// Converte i tuoi disegni (PNG) nel formato .rle del gioco, con i pixel dentro (SDK di Imperivm Resurrected).
// Il gioco legge solo .rle e nessun programma di disegno lo esporta: questo strumento è il ponte. Non disegna nulla.
//
//   node converti-sprite.js <input> --output <cartella> [opzioni]
//
// <input> è una CARTELLA con un PNG per fotogramma e direzione, oppure un file PNG con il foglio già montato (vedi sotto).
//
// CONVENZIONE DI INPUT (una cartella; tutti i PNG della stessa dimensione, la «tela», 150x150 di default per le unità):
//   Walk_d0_f00.png  Walk_d0_f01.png ... Walk_d7_f14.png     <Animazione>_d<direzione>_f<fotogramma>.png, entrambi contando da 0
//   Walk_d0_f00_sombra.png                                    (opzionale) l'ombra di quel fotogramma
//   Idle_..., Fight_..., Death_..., Transition_..., Taunt_...  i nomi che usa il gioco (vedi grafica.md)
//   Ogni animazione deve avere TUTTE le combinazioni direzione x fotogramma. Tutte le animazioni, le stesse direzioni.
//   Sfondo trasparente (alfa). I pixel con alfa < 128 sono considerati trasparenti (nel gioco non esiste la semitrasparenza).
//   I piedi del personaggio stanno nello stesso punto di tutte le tele (di default il pixel (75,100) di 150x150).
//
// FOGLIO MONTATO (alternativa): un PNG <Animazione>.png le cui COLONNE sono le direzioni e le cui RIGHE sono i fotogrammi,
//   ogni cella della dimensione della tela (--tela AnchoxAlto). L'ombra, se c'è, in <Animazione>_sombra.png con la stessa disposizione.
//   Si possono mescolare: la cartella può contenere fogli e fotogrammi singoli.
//
// OPZIONI
//   --output <cartella>      dove si scrivono <Animazione>.rle e <Animazione>_shadow.rle (obbligatorio)
//   --tela AnchoxAlto     dimensione della tela; di default quella del primo PNG (150x150 se è un foglio)
//   --giocatore               colore del giocatore (modo 2 del gioco): ciò che è dipinto di MAGENTA diventa il colore della squadra.
//                           Magenta = rosso e blu alti e uguali, verde zero: (255,0,255) è il colore del giocatore quasi puro e
//                           i toni più scuri, (128,0,128), ecc., sono le sue ombre. Senza questa opzione, il modo 1 (palette libera di
//                           256 colori, l'unità non cambia colore in base al giocatore). Con essa restano 192 colori normali.
//   --ombra <auto|file|nessuna>
//                           auto (predefinito): usa i *_sombra.png se esistono; altrimenti, disegna un'ellisse sotto i piedi;
//                           archivos: solo quelli che hanno un PNG d'ombra (errore se ne manca qualcuno); ninguna: non scrive ombre.
//   EDIFICI: un'animazione chiamata «shadow» (shadow_d0_f00.png…) viene presa come maschera d'ombra e scritta come shadow.rle;
//                gli altri livelli (layer1_d0_f00.png = fase di costruzione 0 del livello 1…) con --ombra nessuna. 1 direzione (d0).
//   --piedi X,Y              dove sono i piedi sulla tela (solo per l'ombra automatica; di default 75,100)
//   --tavolozza-per-animazione  una palette diversa in ogni .rle (di default una sola per tutte le animazioni, così i colori
//                           non cambiano da un'animazione all'altra)
//   --soglia-alfa N         alfa minimo perché un pixel conti (di default 128)
//
// COSA FA: ritaglia ogni fotogramma al suo riquadro di pixel, riduce i colori a una palette di 256 (192 + 64 di tinta con --giocatore)
// con il «taglio mediano» se ce ne sono di più (senza dithering: pixel art), codifica per righe con il formato RLE2 del gioco e rilegge il
// file che ha appena scritto per verificare che contenga esattamente ciò che si voleva.
const fs = require('fs'), path = require('path');
const png = require('./lib/png');
const rle = require('./lib/rle');
const { T } = require('./lib/lang');

// ---------- argomenti -----------
const args = process.argv.slice(2);
const valued = ['--salida', '--lienzo', '--sombra', '--pies', '--umbral-alfa'];
const opt = (n, d) => { const i = args.indexOf(n); return i >= 0 ? args[i + 1] : d; };
const flag = n => args.includes(n);
const input = args.find((a, i) => !a.startsWith('--') && !valued.includes(args[i - 1]));

function fail(msg) { console.error(T('ERROR: ', 'ERROR: ', 'ERRORE: ') + msg); process.exit(1); }
if (!input || !opt('--salida')) {
	console.error(T('uso: node converti-sprite.js <carpeta con PNG | hoja.png> --salida <carpeta> [--jugador] [--sombra auto|archivos|ninguna] [--lienzo 150x150] [--pies 75,100]',
		'usage: node converti-sprite.js <folder with PNG | sheet.png> --salida <folder> [--jugador] [--sombra auto|archivos|ninguna] [--lienzo 150x150] [--pies 75,100]',
		'uso: node converti-sprite.js <cartella con PNG | foglio.png> --salida <cartella> [--jugador] [--sombra auto|archivos|ninguna] [--lienzo 150x150] [--pies 75,100]'));
	console.error(T('     (el convenio de nombres está al principio de este archivo y en grafica.md)',
		'     (the naming convention is at the top of this file and in grafica.md)',
		'     (la convenzione dei nomi è all\'inizio di questo file e in grafica.md)'));
	process.exit(2);
}
if (!fs.existsSync(input)) fail(T('no existe ' + input, 'does not exist: ' + input, 'non esiste ' + input));
const outDir = opt('--salida');
const player = flag('--jugador');
const shadowMode = opt('--sombra', 'auto');
if (!['auto', 'archivos', 'ninguna'].includes(shadowMode)) fail(T('--sombra debe ser auto, archivos o ninguna', '--sombra must be auto, files or none', '--sombra deve essere auto, file o nessuna'));
const alphaMin = parseInt(opt('--umbral-alfa', '128'), 10);
const feet = (opt('--pies', '75,100')).split(',').map(Number);
if (feet.length !== 2 || feet.some(isNaN)) fail(T('--pies debe ser X,Y', '--pies must be X,Y', '--pies deve essere X,Y'));
let canvas = null;
if (opt('--lienzo')) {
	const m = /^(\d+)x(\d+)$/i.exec(opt('--lienzo'));
	if (!m) fail(T('--lienzo debe ser AnchoxAlto, p. ej. 150x150', '--lienzo must be WidthxHeight, e.g. 150x150', '--lienzo deve essere LarghezzaxAltezza, p. es. 150x150'));
	canvas = { w: +m[1], h: +m[2] };
}

// ---------- 1. individuare i PNG ----------
const files = fs.statSync(input).isDirectory() ? fs.readdirSync(input).filter(f => /\.png$/i.test(f)).map(f => path.join(input, f)) : [input];
const anims = new Map(); // nome -> { frames: Map("d,f" -> file), shadows: Map("d,f" -> file), sheet, sheetShadow }
const getAnim = n => { if (!anims.has(n)) anims.set(n, { frames: new Map(), shadows: new Map(), sheet: null, sheetShadow: null }); return anims.get(n); };
for (const f of files) {
	const b = path.basename(f);
	let m;
	if ((m = /^(.+?)_d(\d+)_f(\d+)(_sombra)?\.png$/i.exec(b))) {
		const a = getAnim(m[1]);
		(m[4] ? a.shadows : a.frames).set(`${+m[2]},${+m[3]}`, f);
	} else if ((m = /^(.+?)_sombra\.png$/i.exec(b))) getAnim(m[1]).sheetShadow = f;
	else getAnim(b.replace(/\.png$/i, '')).sheet = f;
}
if (!anims.size) fail(T('no hay PNG en ' + input, 'no PNG in ' + input, 'nessun PNG in ' + input));

// ---------- 2. caricare i fotogrammi: ognuno è {px: Int32Array canvas.w*canvas.h} ---------
// px: -1 trasparente; 0..0xFFFFFF colore RGB; 0x1000000 + indice di tinta (solo con --giocatore)
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
function colorPixels(img, x0, y0, w, h) { // ritaglia una cella dell'immagine
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
function shadowPixels(img, x0, y0, w, h) { // 1 = ombra, -1 = niente
	const out = new Int32Array(w * h).fill(-1);
	for (let y = 0; y < h; y++) for (let x = 0; x < w; x++) {
		const o = ((y0 + y) * img.width + x0 + x) * 4;
		if (img.data[o + 3] >= alphaMin && (img.data[o] + img.data[o + 1] + img.data[o + 2]) / 3 < 200) out[y * w + x] = 1;
	}
	return out;
}

// Ordina ogni animazione in una griglia cols (direzioni) x rows (fotogrammi)
const sheets = []; // { name, cols, rows, color: [Int32Array], shadow: [Int32Array|null] }
function needCanvas(img, f) {
	if (!canvas) canvas = { w: img.width, h: img.height };
	if (img.width !== canvas.w || img.height !== canvas.h) {
		fail(T(`${f}: mide ${img.width}x${img.height} y el lienzo es ${canvas.w}x${canvas.h}. Todos los PNG de fotogramas deben medir lo mismo`,
			`${f}: is ${img.width}x${img.height} and the canvas is ${canvas.w}x${canvas.h}. All frame PNGs must be the same size`,
			`${f}: misura ${img.width}x${img.height} e la tela è ${canvas.w}x${canvas.h}. Tutti i PNG dei fotogrammi devono avere le stesse dimensioni`));
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
			fail(T(`a la animación ${name} le faltan fotogramas (${cols} direcciones x ${rows} fotogramas): ${list}${missing.length > 6 ? ' … (' + missing.length + ' en total)' : ''}`,
				`animation ${name} is missing frames (${cols} directions x ${rows} frames): ${list}${missing.length > 6 ? ' … (' + missing.length + ' in total)' : ''}`,
				`all'animazione ${name} mancano dei fotogrammi (${cols} direzioni x ${rows} fotogrammi): ${list}${missing.length > 6 ? ' … (' + missing.length + ' in totale)' : ''}`));
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
			fail(T(`${a.sheet}: la hoja mide ${img.width}x${img.height}, que no es múltiplo del lienzo ${canvas.w}x${canvas.h} (usa --lienzo)`,
				`${a.sheet}: the sheet is ${img.width}x${img.height}, which is not a multiple of the canvas ${canvas.w}x${canvas.h} (use --lienzo)`,
				`${a.sheet}: il foglio misura ${img.width}x${img.height}, che non è un multiplo della tela ${canvas.w}x${canvas.h} (usa --lienzo)`));
		}
		cols = img.width / canvas.w; rows = img.height / canvas.h;
		const simg = a.sheetShadow ? loadPng(a.sheetShadow) : null;
		if (simg && (simg.width !== img.width || simg.height !== img.height)) {
			fail(T(`${a.sheetShadow} debe medir lo mismo que ${a.sheet}`, `${a.sheetShadow} must be the same size as ${a.sheet}`, `${a.sheetShadow} deve avere le stesse dimensioni di ${a.sheet}`));
		}
		for (let r = 0; r < rows; r++) for (let c = 0; c < cols; c++) {
			color[r * cols + c] = colorPixels(img, c * canvas.w, r * canvas.h, canvas.w, canvas.h);
			shadow[r * cols + c] = simg ? shadowPixels(simg, c * canvas.w, r * canvas.h, canvas.w, canvas.h) : null;
		}
	}
	if (rows > 62) fail(T(`${name} tiene ${rows} fotogramas; el juego admite 62 como máximo por animación`, `${name} has ${rows} frames; the game allows 62 at most per animation`, `${name} ha ${rows} fotogrammi; il gioco ne ammette al massimo 62 per animazione`));
	sheets.push({ name, cols, rows, color, shadow, isShadow: name.toLowerCase() === "shadow" });
}
const dirCounts = [...new Set(sheets.map(s => s.cols))];
if (dirCounts.length > 1) {
	const dl = sheets.map(s => `${s.name}: ${s.cols}`).join(', ');
	fail(T('las animaciones tienen distinto número de direcciones (' + dl + '). Todas las hojas de una unidad llevan las mismas',
		'the animations have different numbers of directions (' + dl + '). All sheets of a unit must have the same',
		'le animazioni hanno un numero diverso di direzioni (' + dl + '). Tutti i fogli di un\'unità devono avere le stesse'));
}
if (![1, 4, 8, 12, 16, 32].includes(dirCounts[0])) {
	console.warn(T(`AVISO: ${dirCounts[0]} direcciones. El juego usa 8 (soldados), 12 (héroes y animales), 4 (barcos), 1 (edificios)`,
		`WARNING: ${dirCounts[0]} directions. The game uses 8 (soldiers), 12 (heroes and animals), 4 (ships), 1 (buildings)`,
		`AVVISO: ${dirCounts[0]} direzioni. Il gioco usa 8 (soldati), 12 (eroi e animali), 4 (navi), 1 (edifici)`));
}

// ---------- 3. palette ----------
// Taglio mediano sui colori distinti (con il loro numero di occorrenze). Restituisce la lista di colori [r,g,b].
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
function buildPalette(group) { // group: lista di fogli; restituisce {list: [[r,g,b]], map: Map(rgb -> indice di palette), exact}
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
const groups = flag("--paleta-por-animacion") ? colorSheets.map(s => [s]) : [colorSheets];

// ---------- 4. scrittura ----------
function ellipseShadow(w, h, colorPx) { // ombra automatica: ellisse sotto i piedi, larga quanto la figura
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
function cropCell(px, w, h, value) { // -> {x0,y0,x1,y1,w,h,pixels:Int16Array} o {empty:true}
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
		console.log(T(`  paleta (${gn}): ${pal.total} colores distintos reducidos a ${maxColors}`,
			`  palette (${gn}): ${pal.total} distinct colors reduced to ${maxColors}`,
			`  tavolozza (${gn}): ${pal.total} colori distinti ridotti a ${maxColors}`));
	}
	for (const s of group) {
		// --- colore
		const cells = s.color.map(px => cropCell(px, W, H, v => v >= 0x1000000 ? v - 0x1000000 : pal.map.get(v)));
		const empties = cells.filter(c => c.empty).length;
		if (empties) {
			console.warn(T(`AVISO: ${s.name}: ${empties} fotogramas están vacíos (todo transparente)`,
				`WARNING: ${s.name}: ${empties} frames are empty (fully transparent)`,
				`AVVISO: ${s.name}: ${empties} fotogrammi sono vuoti (tutto trasparente)`));
			warnings++;
		}
		const palette = Buffer.alloc(1024), rgb555 = Buffer.alloc(512);
		const entries = pal.list.map(c => c);
		entries.forEach((c, i) => {
			const o = (firstIndex + i) * 4;
			palette[o] = c[2]; palette[o + 1] = c[1]; palette[o + 2] = c[0];
			rgb555.writeUInt16LE(((c[0] >> 3) << 10) | ((c[1] >> 3) << 5) | (c[2] >> 3), (firstIndex + i) * 2);
		});
		if (player) { // come il gioco: i primi 64 colori copiano i 64 successivi e la loro tabella RGB555 ha il riempimento 0x6666
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
		console.log(T(`${s.name}.rle: ${s.cols} direcciones x ${s.rows} fotogramas, modo ${sheet.mode}, ${pal.list.length} colores, ${bytes.length} bytes`,
			`${s.name}.rle: ${s.cols} directions x ${s.rows} frames, mode ${sheet.mode}, ${pal.list.length} colors, ${bytes.length} bytes`,
			`${s.name}.rle: ${s.cols} direzioni x ${s.rows} fotogrammi, modo ${sheet.mode}, ${pal.list.length} colori, ${bytes.length} byte`));
		written++;

		// --- ombra
		if (shadowMode === 'ninguna') continue;
		const missingShadow = s.shadow.filter(x => !x).length;
		if (shadowMode === 'archivos' && missingShadow) {
			fail(T(`--sombra archivos: a ${s.name} le faltan ${missingShadow} PNG de sombra (*_sombra.png)`,
				`--sombra archivos: ${s.name} is missing ${missingShadow} shadow PNG (*_sombra.png)`,
				`--sombra archivos: a ${s.name} mancano ${missingShadow} PNG di ombra (*_sombra.png)`));
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
		const shDesc = missingShadow === s.shadow.length ? T('sombra automática (elipse)', 'automatic shadow (ellipse)', 'ombra automatica (ellisse)')
			: missingShadow ? T(`${s.shadow.length - missingShadow} de archivo, ${missingShadow} automáticas`, `${s.shadow.length - missingShadow} from files, ${missingShadow} automatic`, `${s.shadow.length - missingShadow} da file, ${missingShadow} automatiche`)
			: T('sombra de tus PNG', 'shadow from your PNG', 'ombra dai tuoi PNG');
		console.log(T(`${s.name}_shadow.rle: ${shDesc}, ${sb.length} bytes`, `${s.name}_shadow.rle: ${shDesc}, ${sb.length} bytes`, `${s.name}_shadow.rle: ${shDesc}, ${sb.length} byte`));
		written++;
	}
}

// Un'animazione chiamata «shadow» (edifici): i suoi PNG sono direttamente la maschera d'ombra -> shadow.rle (modo 4)
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
	console.log(T(`${s.name}.rle: sombra de tus PNG (cualquier píxel opaco cuenta), ${s.cols} x ${s.rows}, ${sb.length} bytes`,
		`${s.name}.rle: shadow from your PNG (any opaque pixel counts), ${s.cols} x ${s.rows}, ${sb.length} bytes`,
		`${s.name}.rle: ombra dai tuoi PNG (conta qualsiasi pixel opaco), ${s.cols} x ${s.rows}, ${sb.length} byte`));
	written++;
}

// Rilegge quanto scritto e verifica che ogni fotogramma decodifichi i pixel che sono stati codificati
function verify(file, bytes, sheet, label) {
	const back = rle.parse(bytes);
	if (back.trailing !== 0) fail(label + T(': sobran bytes al leer lo escrito (error interno)', ': extra bytes when reading back what was written (internal error)', ': byte in eccesso rileggendo quanto scritto (errore interno)'));
	if (back.cols !== sheet.cols || back.rows !== sheet.rows || back.mode !== sheet.mode) fail(label + T(': la cabecera no coincide (error interno)', ': the header does not match (internal error)', ': l\'intestazione non coincide (errore interno)'));
	sheet.cells.forEach((c, i) => {
		const d = back.cells[i];
		if (!!c.empty !== !!d.empty) fail(T(`${label}: celda ${i} vacía/llena distinta (error interno)`, `${label}: cell ${i} empty/full mismatch (internal error)`, `${label}: cella ${i} vuota/piena diversa (errore interno)`));
		if (c.empty) return;
		const got = rle.decodeFrame(d);
		for (let k = 0; k < got.length; k++) if (got[k] !== c.pixels[k]) fail(T(`${label}: el fotograma ${i} no se relee igual (error interno)`, `${label}: frame ${i} does not read back the same (internal error)`, `${label}: il fotogramma ${i} non si rilegge uguale (errore interno)`));
	});
}
console.log(T(`\nListo: ${written} archivos .rle en ${outDir}${warnings ? ` (${warnings} avisos)` : ''}. Siguiente paso: node crea-entita.js unidad ${outDir} --nombre <MiUnidad>`,
	`\nDone: ${written} .rle files in ${outDir}${warnings ? ` (${warnings} warnings)` : ''}. Next step: node crea-entita.js unidad ${outDir} --nombre <MiUnidad>`,
	`\nFatto: ${written} file .rle in ${outDir}${warnings ? ` (${warnings} avvisi)` : ''}. Passo successivo: node crea-entita.js unidad ${outDir} --nombre <MiUnidad>`));
