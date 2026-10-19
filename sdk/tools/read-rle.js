// Reads Imperivm .rle files (with the pixels inside) and converts them to PNG, for debugging (Imperivm Resurrected SDK). Read-only.
//
//   node read-rle.js <file.rle | folder> [--output <folder>] [--frames] [--color RRGGBB] [--verify]
//
//   (no options)  writes next to the .rle (or in --output) a SHEET <name>.png with all the frames: rows = frames,
//                   columns = directions, each one in its place within the cell (as the game would place them).
//   --frames    also one PNG per frame: <name>_dDD_fFFF.png (DD = direction, FFF = frame, from 0)
//   --color         player color used to paint indices 0..63 of mode 2 (default 3050E0, blue)
//   --verify     writes no PNG: decodes each file, encodes it again and compares byte by byte with the original
//                   (and, if the bytes differ, compares the image pixel by pixel). Exits with code 1 if any file is not identical.
//   --convention      writes the frames as convert-sprite.js expects them (Name_d3_f05.png at full canvas, player tint in
//                   shades of magenta, shadow in Name_d3_f05_sombra.png); --canvas WidthxHeight (default 150x150).
//                   Useful to check the round trip, or to start from a game sprite as a drawing template.
//   --rle-mmp     path to the game's rle.mmp (read-only): lets you read the .rle.mmp files of the paks, whose pixels are not inside
//                   them but in that 410 MB file. Without this option they are skipped (only the .rle files with the pixels inside are read).
// Useful to see what the game does and to check the converter; it copies nothing from the game.
const fs = require('fs'), path = require('path');
const png = require('./lib/png');
const rle = require('./lib/rle');

const args = process.argv.slice(2);
const opt = n => { const i = args.indexOf(n); return i >= 0 ? args[i + 1] : null; };
const flag = n => args.includes(n);
const valued = ['--output', '--color', '--rle-mmp', '--canvas'];
const mmpPath = opt('--rle-mmp');
const mmpFd = mmpPath ? fs.openSync(mmpPath, 'r') : null;
const input = args.find((a, i) => !a.startsWith('--') && !valued.includes(args[i - 1]));
if (!input || !fs.existsSync(input)) {
	console.error('usage: node read-rle.js <file.rle | folder> [--output <folder>] [--frames] [--color RRGGBB] [--verify]');
	process.exit(2);
}
const outDir = opt('--output');
const tint = parseInt(opt('--color') || '3050E0', 16);

function listRle(p) {
	if (fs.statSync(p).isFile()) return [p];
	const out = [];
	for (const e of fs.readdirSync(p, { withFileTypes: true })) {
		const f = path.join(p, e.name);
		if (e.isDirectory()) out.push(...listRle(f)); else if (/\.rle(\.mmp)?$/i.test(e.name)) out.push(f);
	}
	return out;
}

// RGBA color of a decoded pixel
function colorOf(sheet, v, magenta) {
	if (v < 0) return [0, 0, 0, 0];
	const c0 = sheet.cells.find(c => !c.empty);
	if (sheet.mode === rle.MODE_SHADOW || (c0 && c0.fmt === rle.FMT_MASK)) return [0, 0, 0, 128];
	if (c0 && c0.fmt === rle.FMT_RGB555) return [((v >> 10) & 31) * 255 / 31 | 0, ((v >> 5) & 31) * 255 / 31 | 0, (v & 31) * 255 / 31 | 0, 255];
	if (sheet.mode === rle.MODE_PLAYER && v < 64 && magenta) { const k = Math.round((v >> 3) * 255 / 7); return v >> 3 ? [k, 0, k, 255] : [0, 0, 0, 255]; }
	if (sheet.mode === rle.MODE_PLAYER && v < 64) {
		// tint approximation: value v*8+s -> brightness and saturation of the player color (the game computes it in HSV)
		const val = (v >> 3) / 7, sat = (v & 7) / 7;
		const base = [(tint >> 16) & 255, (tint >> 8) & 255, tint & 255];
		const k = 0.25 + 0.75 * val;
		return base.map(b => Math.round((b * sat + 255 * (1 - sat) * 0.5 + b * (1 - sat) * 0.5) * k)).concat(255);
	}
	const o = v * 4;
	if (o + 2 >= sheet.palette.length) return [255, 0, 255, 255];
	return [sheet.palette[o + 2], sheet.palette[o + 1], sheet.palette[o], 255];
}

function frameImage(sheet, cell, magenta) {
	const px = rle.decodeFrame(cell), data = Buffer.alloc(cell.w * cell.h * 4);
	for (let i = 0; i < px.length; i++) colorOf(sheet, px[i], magenta).forEach((b, k) => { data[i * 4 + k] = b; });
	return { width: cell.w, height: cell.h, data };
}

function toSheetPng(sheet) {
	const cells = sheet.cells.filter(c => !c.empty);
	if (!cells.length) return null;
	const minx = Math.min(...cells.map(c => c.x0)), miny = Math.min(...cells.map(c => c.y0));
	const cw = Math.max(...cells.map(c => c.x1)) - minx + 1, ch = Math.max(...cells.map(c => c.y1)) - miny + 1;
	const W = cw * sheet.cols, H = ch * sheet.rows;
	if (W * H > 64e6) throw new Error(('the sheet would be too large (' + W + 'x' + H + '); use --frames'));
	const data = Buffer.alloc(W * H * 4);
	sheet.cells.forEach((c, i) => {
		if (c.empty) return;
		const col = i % sheet.cols, row = (i / sheet.cols) | 0, img = frameImage(sheet, c);
		for (let y = 0; y < c.h; y++) img.data.copy(data, ((row * ch + (c.y0 - miny) + y) * W + col * cw + (c.x0 - minx)) * 4, y * c.w * 4, (y + 1) * c.w * 4);
	});
	return png.encode({ width: W, height: H, data });
}

function describe(sheet) {
	const used = sheet.cells.filter(c => !c.empty);
	const fmts = [...new Set(used.map(c => c.fmt))].join('/');
	return `mode ${sheet.mode}, ${sheet.cols} columns x ${sheet.rows} rows, ${used.length}/${sheet.cells.length} frames with image, ` +
		`palette ${sheet.paletteCount}, fmt ${fmts}`;
}

const ERR = 'ERROR  ';
let failures = 0;
for (const file of listRle(input)) {
	let buf = fs.readFileSync(file);
	const name = path.basename(file).replace(/\.rle(\.mmp)?$/i, '');
	let sheet;
	try { sheet = rle.parse(buf); } catch (e) { console.log(`${ERR}${file}: ${e.message}`); failures++; continue; }
	if (sheet.cells.some(c => !c.empty && !c.data)) {
		if (mmpFd === null) {
			console.log(`SKIPPED  ${file}: its pixels are outside (rle.mmp); use --rle-mmp <path to the game's rle.mmp>`);
			continue;
		}
		try { rle.resolveMmp(sheet, mmpFd); } catch (e) { console.log(`${ERR}${file}: ${e.message}`); failures++; continue; }
		buf = rle.write(sheet); // the "pixels inside" version of that .rle.mmp: it is what gets compared against
	}
	if (flag('--verify')) {
		const again = rle.write(rle.reencode(sheet)), same = again.equals(buf) && sheet.trailing === 0;
		if (same) { console.log(`${'SAME  '}${path.basename(file)}  ${describe(sheet)}`); continue; }
		// at least the same image?
		const back = rle.parse(again);
		let samePixels = back.cells.length === sheet.cells.length;
		sheet.cells.forEach((c, i) => {
			const d = back.cells[i];
			if (c.empty !== d.empty) samePixels = false;
			else if (!c.empty && !Buffer.from(rle.decodeFrame(c).buffer).equals(Buffer.from(rle.decodeFrame(d).buffer))) samePixels = false;
		});
		let firstDiff = 0; while (firstDiff < Math.min(again.length, buf.length) && again[firstDiff] === buf[firstDiff]) firstDiff++;
		console.log(`DIFFERENT  ${path.basename(file)}  image ${samePixels ? 'identical' : 'DIFFERENT'}; ${buf.length} vs ${again.length} bytes; first differing byte at ${firstDiff}; ${sheet.trailing} extra bytes at the end`);
		failures++;
		continue;
	}
	const dir = outDir || path.dirname(file);
	fs.mkdirSync(dir, { recursive: true });
	if (flag('--convention')) {
		// Writes what convert-sprite.js expects: one PNG per frame and direction, at full canvas, tint in magenta, shadow separate
		const m = /^(\d+)x(\d+)$/i.exec(opt('--canvas') || '150x150');
		if (!m) { console.log(ERR + '--canvas must be WidthxHeight'); failures++; continue; }
		const LW = +m[1], LH = +m[2], isShadow = sheet.mode === rle.MODE_SHADOW, base = name.replace(/_shadow$/i, ''), suffix = isShadow ? '_sombra' : '';
		let bad = false;
		sheet.cells.forEach((c, i) => {
			if (c.empty) return;
			if (c.x0 < 0 || c.y0 < 0 || c.x1 >= LW || c.y1 >= LH) { bad = true; return; }
			const img = frameImage(sheet, c, true), data = Buffer.alloc(LW * LH * 4);
			for (let y = 0; y < c.h; y++) img.data.copy(data, ((c.y0 + y) * LW + c.x0) * 4, y * c.w * 4, (y + 1) * c.w * 4);
			fs.writeFileSync(path.join(dir, `${base}_d${i % sheet.cols}_f${String((i / sheet.cols) | 0).padStart(2, '0')}${suffix}.png`), png.encode({ width: LW, height: LH, data }));
		});
		console.log(bad ? ERR + `${name}: there are frames outside the ${LW}x${LH} canvas`
			: `${name}: ${sheet.cols} directions x ${sheet.rows} frames -> ${base}_d*_f*${suffix}.png (canvas ${LW}x${LH})`);
		if (bad) failures++;
		continue;
	}
	const sheetPng = toSheetPng(sheet);
	if (sheetPng) fs.writeFileSync(path.join(dir, name + '.png'), sheetPng);
	if (flag('--frames')) {
		sheet.cells.forEach((c, i) => {
			if (c.empty) return;
			const d = String(i % sheet.cols).padStart(2, '0'), f = String((i / sheet.cols) | 0).padStart(3, '0');
			fs.writeFileSync(path.join(dir, `${name}_d${d}_f${f}.png`), png.encode(frameImage(sheet, c)));
		});
	}
	console.log(`${name}: ${describe(sheet)}${sheetPng ? ' -> ' + path.join(dir, name + '.png') : ' (all empty)'}`);
}
process.exit(failures ? 1 : 0);
