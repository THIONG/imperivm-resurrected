// Converts your drawings (PNG) to the game's .rle format, with the pixels inside (Imperivm Resurrected SDK).
// The game only reads .rle and no drawing program exports it: this tool is the bridge. It draws nothing.
//
//   node convert-sprite.js <input> --output <folder> [options]
//
// <input> is a FOLDER with one PNG per frame and direction, or a PNG file with the sheet already assembled (see below).
//
// INPUT CONVENTION (one folder; all PNGs the same size, the "canvas", 150x150 by default for units):
//   Walk_d0_f00.png  Walk_d0_f01.png ... Walk_d7_f14.png     <Animation>_d<direction>_f<frame>.png, both counting from 0
//   Walk_d0_f00_shadow.png                                    (optional) the shadow of that frame (_sombra also works)
//   Idle_..., Fight_..., Death_..., Transition_..., Taunt_...  the names the game uses (see graphics.md)
//   Each animation must have ALL the direction x frame combinations. All animations, the same directions.
//   Transparent background (alpha). Pixels with alpha < 128 are taken as transparent (the game has no semi-transparency).
//   The character's feet go at the same point in every canvas (by default pixel (75,100) of 150x150).
//
// ASSEMBLED SHEET (alternative): a PNG <Animation>.png whose COLUMNS are the directions and whose ROWS are the frames,
//   each cell the size of the canvas (--canvas WidthxHeight). The shadow, if any, in <Animation>_shadow.png with the same layout.
//   The two can be mixed: the folder can hold sheets and individual frames.
//
// OPTIONS
//   --output <folder>      where <Animation>.rle and <Animation>_shadow.rle are written (required)
//   --canvas WidthxHeight  canvas size; by default that of the first PNG (150x150 if it is a sheet)
//   --player               player color (the game's mode 2): whatever is painted in MAGENTA becomes the team color.
//                           Magenta = high and equal red and blue, zero green: (255,0,255) is almost pure player color and
//                           the darker shades, (128,0,128), etc., are its shades. Without this option, mode 1 (free palette of
//                           256 colors, the unit does not change color with the player). With it, 192 normal colors remain.
//   --shadow <auto|files|none>
//                           auto (default): uses the *_shadow.png files if they exist; otherwise, draws an ellipse under the feet;
//                           files: only those that come with a shadow PNG (error if any is missing); none: writes no shadows.
//   BUILDINGS: an animation named "shadow" (shadow_d0_f00.png...) is taken as the shadow mask and written as shadow.rle;
//                the other layers (layer1_d0_f00.png = construction stage 0 of layer 1...) with --shadow none. 1 direction (d0).
//   --feet X,Y              where the feet are on the canvas (only for the automatic shadow; default 75,100)
//   --palette-per-animation  a different palette in each .rle (by default a single one for all animations, so the colors
//                           do not change from one animation to another)
//   --alpha-threshold N         minimum alpha for a pixel to count (default 128)
//
// WHAT IT DOES: crops each frame to its pixel box, reduces the colors to a 256-color palette (192 + 64 of tint with --player)
// by "median cut" if there are more (no dithering: pixel art), encodes by rows with the game's RLE2 format and re-reads the
// file it has just written to check that it contains exactly what was intended.
const fs = require('fs'), path = require('path');
const png = require('./lib/png');
const rle = require('./lib/rle');

// ---------- arguments ----------
const args = process.argv.slice(2);
const valued = ['--output', '--canvas', '--shadow', '--feet', '--alpha-threshold'];
const opt = (n, d) => { const i = args.indexOf(n); return i >= 0 ? args[i + 1] : d; };
const flag = n => args.includes(n);
const input = args.find((a, i) => !a.startsWith('--') && !valued.includes(args[i - 1]));

function fail(msg) { console.error('ERROR: ' + msg); process.exit(1); }
if (!input || !opt('--output')) {
	console.error('usage: node convert-sprite.js <folder with PNG | sheet.png> --output <folder> [--player] [--shadow auto|files|none] [--canvas 150x150] [--feet 75,100]');
	console.error('     (the naming convention is at the top of this file and in graphics.md)');
	process.exit(2);
}
if (!fs.existsSync(input)) fail('does not exist: ' + input);
const outDir = opt('--output');
const player = flag('--player');
const shadowMode = opt('--shadow', 'auto');
if (!['auto', 'files', 'none'].includes(shadowMode)) fail('--shadow must be auto, files or none');
const alphaMin = parseInt(opt('--alpha-threshold', '128'), 10);
const feet = (opt('--feet', '75,100')).split(',').map(Number);
if (feet.length !== 2 || feet.some(isNaN)) fail('--feet must be X,Y');
let canvas = null;
if (opt('--canvas')) {
	const m = /^(\d+)x(\d+)$/i.exec(opt('--canvas'));
	if (!m) fail('--canvas must be WidthxHeight, e.g. 150x150');
	canvas = { w: +m[1], h: +m[2] };
}

// ---------- 1. locating the PNGs ----------
const files = fs.statSync(input).isDirectory() ? fs.readdirSync(input).filter(f => /\.png$/i.test(f)).map(f => path.join(input, f)) : [input];
const anims = new Map(); // name -> { frames: Map("d,f" -> file), shadows: Map("d,f" -> file), sheet, sheetShadow }
const getAnim = n => { if (!anims.has(n)) anims.set(n, { frames: new Map(), shadows: new Map(), sheet: null, sheetShadow: null }); return anims.get(n); };
for (const f of files) {
	const b = path.basename(f);
	let m;
	if ((m = /^(.+?)_d(\d+)_f(\d+)(_shadow|_sombra)?\.png$/i.exec(b))) {
		const a = getAnim(m[1]);
		(m[4] ? a.shadows : a.frames).set(`${+m[2]},${+m[3]}`, f);
	} else if ((m = /^(.+?)_(?:shadow|sombra)\.png$/i.exec(b))) getAnim(m[1]).sheetShadow = f;
	else getAnim(b.replace(/\.png$/i, '')).sheet = f;
}
if (!anims.size) fail('no PNG in ' + input);

// ---------- 2. loading frames: each one is {px: Int32Array canvas.w*canvas.h} ----------
// px: -1 transparent; 0..0xFFFFFF RGB color; 0x1000000 + tint index (only with --player)
const cache = new Map();
function loadPng(f) {
	if (!cache.has(f)) {
		try { cache.set(f, png.decode(fs.readFileSync(f))); } catch (e) { fail(`${f}: ${e.message}`); }
	}
	return cache.get(f);
}
function isKey(r, g, b) {
	const m = (r + b) / 2;
	return m >= 30 && g <= m * 0.3 && Math.abs(r - b) <= 0.25 * Math.max(r, b);
}
function colorPixels(img, x0, y0, w, h) { // crops one cell out of the image
	const out = new Int32Array(w * h);
	for (let y = 0; y < h; y++) for (let x = 0; x < w; x++) {
		const o = ((y0 + y) * img.width + x0 + x) * 4, a = img.data[o + 3];
		if (a < alphaMin) { out[y * w + x] = -1; continue; }
		const r = img.data[o], g = img.data[o + 1], b = img.data[o + 2];
		if (player && isKey(r, g, b)) out[y * w + x] = 0x1000000 + Math.min(7, Math.round((r + b) / 2 * 7 / 255)) * 8 + 7;
		else out[y * w + x] = (r << 16) | (g << 8) | b;
	}
	return out;
}
function shadowPixels(img, x0, y0, w, h) { // 1 = shadow, -1 = nothing
	const out = new Int32Array(w * h).fill(-1);
	for (let y = 0; y < h; y++) for (let x = 0; x < w; x++) {
		const o = ((y0 + y) * img.width + x0 + x) * 4;
		if (img.data[o + 3] >= alphaMin && (img.data[o] + img.data[o + 1] + img.data[o + 2]) / 3 < 200) out[y * w + x] = 1;
	}
	return out;
}

// Arranges each animation in a grid of cols (directions) x rows (frames)
const sheets = []; // { name, cols, rows, color: [Int32Array], shadow: [Int32Array|null] }
function needCanvas(img, f) {
	if (!canvas) canvas = { w: img.width, h: img.height };
	if (img.width !== canvas.w || img.height !== canvas.h) {
		fail(`${f}: is ${img.width}x${img.height} and the canvas is ${canvas.w}x${canvas.h}. All frame PNGs must be the same size`);
	}
}
for (const [name, a] of [...anims.entries()].sort()) {
	let cols, rows, color = [], shadow = [];
	if (a.frames.size) {
		const keys = [...a.frames.keys()].map(k => k.split(',').map(Number));
		cols = Math.max(...keys.map(k => k[0])) + 1; rows = Math.max(...keys.map(k => k[1])) + 1;
		const missing = [];
		for (let r = 0; r < rows; r++) for (let c = 0; c < cols; c++) if (!a.frames.has(`${c},${r}`)) missing.push(`${name}_d${c}_f${String(r).padStart(2, '0')}.png`);
		if (missing.length) {
			const list = missing.slice(0, 6).join(', ');
			fail(`animation ${name} is missing frames (${cols} directions x ${rows} frames): ${list}${missing.length > 6 ? ' … (' + missing.length + ' in total)' : ''}`);
		}
		for (let r = 0; r < rows; r++) for (let c = 0; c < cols; c++) {
			const f = a.frames.get(`${c},${r}`), img = loadPng(f);
			needCanvas(img, f);
			color[r * cols + c] = colorPixels(img, 0, 0, canvas.w, canvas.h);
			const sf = a.shadows.get(`${c},${r}`);
			if (sf) { const s = loadPng(sf); needCanvas(s, sf); shadow[r * cols + c] = shadowPixels(s, 0, 0, canvas.w, canvas.h); }
			else shadow[r * cols + c] = null;
		}
	} else {
		if (!canvas) canvas = { w: 150, h: 150 };
		const img = loadPng(a.sheet);
		if (img.width % canvas.w || img.height % canvas.h) {
			fail(`${a.sheet}: the sheet is ${img.width}x${img.height}, which is not a multiple of the canvas ${canvas.w}x${canvas.h} (use --canvas)`);
		}
		cols = img.width / canvas.w; rows = img.height / canvas.h;
		const simg = a.sheetShadow ? loadPng(a.sheetShadow) : null;
		if (simg && (simg.width !== img.width || simg.height !== img.height)) {
			fail(`${a.sheetShadow} must be the same size as ${a.sheet}`);
		}
		for (let r = 0; r < rows; r++) for (let c = 0; c < cols; c++) {
			color[r * cols + c] = colorPixels(img, c * canvas.w, r * canvas.h, canvas.w, canvas.h);
			shadow[r * cols + c] = simg ? shadowPixels(simg, c * canvas.w, r * canvas.h, canvas.w, canvas.h) : null;
		}
	}
	if (rows > 62) fail(`${name} has ${rows} frames; the game allows 62 at most per animation`);
	sheets.push({ name, cols, rows, color, shadow, isShadow: name.toLowerCase() === "shadow" });
}
const dirCounts = [...new Set(sheets.map(s => s.cols))];
if (dirCounts.length > 1) {
	const dl = sheets.map(s => `${s.name}: ${s.cols}`).join(', ');
	fail('the animations have different numbers of directions (' + dl + '). All sheets of a unit must have the same');
}
if (![1, 4, 8, 12, 16, 32].includes(dirCounts[0])) {
	console.warn(`WARNING: ${dirCounts[0]} directions. The game uses 8 (soldiers), 12 (heroes and animals), 4 (ships), 1 (buildings)`);
}

// ---------- 3. palettes ----------
// Median cut over the distinct colors (with their occurrence counts). Returns the list of colors [r,g,b].
function medianCut(counts, k) {
	let boxes = [[...counts.entries()].map(([rgb, n]) => ({ r: rgb >> 16, g: (rgb >> 8) & 255, b: rgb & 255, n }))];
	while (boxes.length < k) {
		let best = -1, bestScore = 0, bestCh = 'r';
		boxes.forEach((bx, i) => {
			if (bx.length < 2) return;
			for (const ch of ['r', 'g', 'b']) {
				let lo = 255, hi = 0;
				for (const c of bx) { if (c[ch] < lo) lo = c[ch]; if (c[ch] > hi) hi = c[ch]; }
				const score = (hi - lo) * (ch === 'g' ? 1.2 : ch === 'r' ? 1 : 0.8);
				if (score > bestScore) { bestScore = score; best = i; bestCh = ch; }
			}
		});
		if (best < 0) break;
		const bx = boxes[best].sort((p, q) => p[bestCh] - q[bestCh]);
		const total = bx.reduce((s, c) => s + c.n, 0);
		let acc = 0, cut = 1;
		for (let i = 0; i < bx.length - 1; i++) { acc += bx[i].n; cut = i + 1; if (acc >= total / 2) break; }
		boxes.splice(best, 1, bx.slice(0, cut), bx.slice(cut));
	}
	return boxes.map(bx => {
		const t = bx.reduce((s, c) => s + c.n, 0);
		return [Math.round(bx.reduce((s, c) => s + c.r * c.n, 0) / t), Math.round(bx.reduce((s, c) => s + c.g * c.n, 0) / t), Math.round(bx.reduce((s, c) => s + c.b * c.n, 0) / t)];
	});
}
const firstIndex = player ? 64 : 0, maxColors = 256 - firstIndex;
function buildPalette(group) { // group: list of sheets; returns {list: [[r,g,b]], map: Map(rgb -> palette index), exact}
	const counts = new Map();
	for (const s of group) for (const px of s.color) for (let i = 0; i < px.length; i++) { const v = px[i]; if (v >= 0 && v < 0x1000000) counts.set(v, (counts.get(v) || 0) + 1); }
	const exact = counts.size <= maxColors;
	const list = exact ? [...counts.keys()].sort((a, b) => a - b).map(v => [v >> 16, (v >> 8) & 255, v & 255]) : medianCut(counts, maxColors);
	const map = new Map();
	for (const rgb of counts.keys()) {
		const r = rgb >> 16, g = (rgb >> 8) & 255, b = rgb & 255;
		let bi = 0, bd = Infinity;
		for (let i = 0; i < list.length; i++) {
			const d = (list[i][0] - r) ** 2 + (list[i][1] - g) ** 2 + (list[i][2] - b) ** 2;
			if (d < bd) { bd = d; bi = i; if (d === 0) break; }
		}
		map.set(rgb, firstIndex + bi);
	}
	return { list, map, exact, total: counts.size };
}
const colorSheets = sheets.filter(s => !s.isShadow);
const groups = flag('--palette-per-animation') ? colorSheets.map(s => [s]) : [colorSheets];

// ---------- 4. writing ----------
function ellipseShadow(w, h, colorPx) { // automatic shadow: an ellipse under the feet, as wide as the figure
	let lo = w, hi = -1;
	for (let y = 0; y < h; y++) for (let x = 0; x < w; x++) if (colorPx[y * w + x] >= 0) { if (x < lo) lo = x; if (x > hi) hi = x; }
	const out = new Int32Array(w * h).fill(-1);
	if (hi < 0) return out;
	const rx = Math.max(8, Math.min(30, Math.round((hi - lo + 1) / 2 * 0.8))), ry = Math.max(4, Math.round(rx * 0.45));
	const cx = feet[0], cy = feet[1] - 3;
	for (let y = Math.max(0, cy - ry); y <= Math.min(h - 1, cy + ry); y++) for (let x = Math.max(0, cx - rx); x <= Math.min(w - 1, cx + rx); x++)
		if (((x - cx) / rx) ** 2 + ((y - cy) / ry) ** 2 <= 1) out[y * w + x] = 1;
	return out;
}
function cropCell(px, w, h, value) { // -> {x0,y0,x1,y1,w,h,pixels:Int16Array} or {empty:true}
	let x0 = w, y0 = h, x1 = -1, y1 = -1;
	for (let y = 0; y < h; y++) for (let x = 0; x < w; x++) if (px[y * w + x] >= 0) { if (x < x0) x0 = x; if (x > x1) x1 = x; if (y < y0) y0 = y; if (y > y1) y1 = y; }
	if (x1 < 0) return { empty: true, x0: 0, y0: 0, x1: -1, y1: -1 };
	const cw = x1 - x0 + 1, ch = y1 - y0 + 1, pixels = new Int16Array(cw * ch);
	for (let y = 0; y < ch; y++) for (let x = 0; x < cw; x++) { const v = px[(y0 + y) * w + x0 + x]; pixels[y * cw + x] = v < 0 ? -1 : value(v); }
	return { empty: false, x0, y0, x1, y1, w: cw, h: ch, pixels };
}
fs.mkdirSync(outDir, { recursive: true });
let warnings = 0, written = 0;
const W = canvas.w, H = canvas.h;
for (const group of groups) {
	const pal = buildPalette(group);
	if (!pal.exact) {
		const gn = group.map(s => s.name).join(', ');
		console.log(`  palette (${gn}): ${pal.total} distinct colors reduced to ${maxColors}`);
	}
	for (const s of group) {
		// --- color
		const cells = s.color.map(px => cropCell(px, W, H, v => v >= 0x1000000 ? v - 0x1000000 : pal.map.get(v)));
		const empties = cells.filter(c => c.empty).length;
		if (empties) {
			console.warn(`WARNING: ${s.name}: ${empties} frames are empty (fully transparent)`);
			warnings++;
		}
		const palette = Buffer.alloc(1024), rgb555 = Buffer.alloc(512);
		const entries = pal.list.map(c => c);
		entries.forEach((c, i) => {
			const o = (firstIndex + i) * 4;
			palette[o] = c[2]; palette[o + 1] = c[1]; palette[o + 2] = c[0];
			rgb555.writeUInt16LE(((c[0] >> 3) << 10) | ((c[1] >> 3) << 5) | (c[2] >> 3), (firstIndex + i) * 2);
		});
		if (player) { // like the game: the first 64 colors copy the next 64 and its RGB555 table carries the 0x6666 fill
			palette.copy(palette, 0, 64 * 4, 128 * 4);
			for (let i = 0; i < 64; i++) rgb555.writeUInt16LE(0x6666, i * 2);
		}
		const sheet = {
			mode: player ? rle.MODE_PLAYER : rle.MODE_PAL, cols: s.cols, rows: s.rows,
			cells: cells.map(c => c.empty ? { empty: true } : Object.assign(c, { fmt: rle.FMT_INDEX, key: player ? 161 : 1, ownPalette: 0 })),
			paletteCount: 256, palette, rgb555: player ? rgb555 : null,
		};
		const bytes = rle.write(sheet);
		const file = path.join(outDir, s.name + '.rle');
		fs.writeFileSync(file, bytes);
		verify(file, bytes, sheet, s.name);
		console.log(`${s.name}.rle: ${s.cols} directions x ${s.rows} frames, mode ${sheet.mode}, ${pal.list.length} colors, ${bytes.length} bytes`);
		written++;

		// --- shadow
		if (shadowMode === 'none') continue;
		const missingShadow = s.shadow.filter(x => !x).length;
		if (shadowMode === 'files' && missingShadow) {
			fail(`--shadow files: ${s.name} is missing ${missingShadow} shadow PNG (*_shadow.png)`);
		}
		const sh = s.shadow.map((x, i) => x || ellipseShadow(W, H, s.color[i]));
		const scells = sh.map(px => cropCell(px, W, H, () => 1));
		const ssheet = {
			mode: rle.MODE_SHADOW, cols: s.cols, rows: s.rows,
			cells: scells.map(c => c.empty ? { empty: true } : Object.assign(c, { fmt: rle.FMT_MASK, key: 65280, ownPalette: 0 })),
			paletteCount: 0, palette: Buffer.alloc(0), rgb555: null,
		};
		const sb = rle.write(ssheet), sfile = path.join(outDir, s.name + '_shadow.rle');
		fs.writeFileSync(sfile, sb);
		verify(sfile, sb, ssheet, s.name + '_shadow');
		const shDesc = missingShadow === s.shadow.length ? 'automatic shadow (ellipse)'
			: missingShadow ? `${s.shadow.length - missingShadow} from files, ${missingShadow} automatic`
			: 'shadow from your PNG';
		console.log(`${s.name}_shadow.rle: ${shDesc}, ${sb.length} bytes`);
		written++;
	}
}

// An animation named "shadow" (buildings): its PNGs are directly the shadow mask -> shadow.rle (mode 4)
for (const s of sheets.filter(x => x.isShadow)) {
	const scells = s.color.map(px => cropCell(px, W, H, () => 1));
	const ssheet = {
		mode: rle.MODE_SHADOW, cols: s.cols, rows: s.rows,
		cells: scells.map(c => c.empty ? { empty: true } : Object.assign(c, { fmt: rle.FMT_MASK, key: 65280, ownPalette: 0 })),
		paletteCount: 0, palette: Buffer.alloc(0), rgb555: null,
	};
	const sb = rle.write(ssheet);
	fs.writeFileSync(path.join(outDir, s.name + '.rle'), sb);
	verify(s.name, sb, ssheet, s.name);
	console.log(`${s.name}.rle: shadow from your PNG (any opaque pixel counts), ${s.cols} x ${s.rows}, ${sb.length} bytes`);
	written++;
}

// Re-reads what was written and checks that each frame decodes to the pixels that were encoded
function verify(file, bytes, sheet, label) {
	const back = rle.parse(bytes);
	if (back.trailing !== 0) fail(label + ': extra bytes when reading back what was written (internal error)');
	if (back.cols !== sheet.cols || back.rows !== sheet.rows || back.mode !== sheet.mode) fail(label + ': the header does not match (internal error)');
	sheet.cells.forEach((c, i) => {
		const d = back.cells[i];
		if (!!c.empty !== !!d.empty) fail(`${label}: cell ${i} empty/full mismatch (internal error)`);
		if (c.empty) return;
		const got = rle.decodeFrame(d);
		for (let k = 0; k < got.length; k++) if (got[k] !== c.pixels[k]) fail(`${label}: frame ${i} does not read back the same (internal error)`);
	});
}
console.log(`\nDone: ${written} .rle files in ${outDir}${warnings ? ` (${warnings} warnings)` : ''}. Next step: node create-entity.js unit ${outDir} --name <MyUnit>`);
