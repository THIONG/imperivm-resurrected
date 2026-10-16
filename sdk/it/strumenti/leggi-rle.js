// Legge file .rle di Imperivm (con i pixel dentro) e li converte in PNG, per il debug (SDK di Imperivm Resurrected). Sola lettura.
//
//   node leggi-rle.js <file.rle | cartella> [--output <cartella>] [--fotogrammi] [--colore RRGGBB] [--verifica]
//
//   (senza opzioni) scrive accanto al .rle (o in --output) un FOGLIO <nome>.png con tutti i fotogrammi: righe = fotogrammi,
//                   colonne = direzioni, ciascuno al suo posto dentro la cella (come lo metterebbe il gioco).
//   --fotogrammi    in più un PNG per fotogramma: <nome>_dDD_fFFF.png (DD = direzione, FFF = fotogramma, da 0)
//   --colore         colore del giocatore con cui si dipingono gli indici 0..63 del modo 2 (di default 3050E0, blu)
//   --verifica     non scrive PNG: decodifica ogni file, lo ricodifica e confronta byte per byte con l'originale
//                   (e, se i byte differiscono, confronta l'immagine pixel per pixel). Esce con codice 1 se qualche file non è identico.
//   --convenzione      scrive i fotogrammi come li si aspetta converti-sprite.js (Nombre_d3_f05.png a tela piena, tinta del
//                   giocatore in toni di magenta, ombra in Nombre_d3_f05_sombra.png); --tela AnchoxAlto (di default 150x150).
//                   Serve per verificare il viaggio di andata e ritorno, o per partire da uno sprite del gioco come modello di disegno.
//   --rle-mmp     percorso del rle.mmp del gioco (sola lettura): permette di leggere i .rle.mmp dei pak, i cui pixel non sono dentro
//                   ma in quel file da 410 MB. Senza questa opzione vengono saltati (si leggono solo i .rle con i pixel dentro).
// Serve per vedere cosa fa il gioco e per verificare il convertitore; non copia nulla del gioco.
const fs = require('fs'), path = require('path');
const png = require('./lib/png');
const rle = require('./lib/rle');
const { T } = require('./lib/lang');

const args = process.argv.slice(2);
const opt = n => { const i = args.indexOf(n); return i >= 0 ? args[i + 1] : null; };
const flag = n => args.includes(n);
const valued = ['--salida', '--color', '--rle-mmp', '--lienzo'];
const mmpPath = opt('--rle-mmp');
const mmpFd = mmpPath ? fs.openSync(mmpPath, 'r') : null;
const input = args.find((a, i) => !a.startsWith('--') && !valued.includes(args[i - 1]));
if (!input || !fs.existsSync(input)) {
	console.error(T('uso: node leggi-rle.js <archivo.rle | carpeta> [--salida <carpeta>] [--fotogramas] [--color RRGGBB] [--verificar]',
		'usage: node leggi-rle.js <file.rle | folder> [--salida <folder>] [--fotogramas] [--color RRGGBB] [--verificar]',
		'uso: node leggi-rle.js <file.rle | cartella> [--salida <cartella>] [--fotogramas] [--color RRGGBB] [--verificar]'));
	process.exit(2);
}
const outDir = opt('--salida');
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

// Colore RGBA di un pixel decodificato
function colorOf(sheet, v, magenta) {
	if (v < 0) return [0, 0, 0, 0];
	const c0 = sheet.cells.find(c => !c.empty);
	if (sheet.mode === rle.MODE_SHADOW || (c0 && c0.fmt === rle.FMT_MASK)) return [0, 0, 0, 128];
	if (c0 && c0.fmt === rle.FMT_RGB555) return [((v >> 10) & 31) * 255 / 31 | 0, ((v >> 5) & 31) * 255 / 31 | 0, (v & 31) * 255 / 31 | 0, 255];
	if (sheet.mode === rle.MODE_PLAYER && v < 64 && magenta) { const k = Math.round((v >> 3) * 255 / 7); return v >> 3 ? [k, 0, k, 255] : [0, 0, 0, 255]; }
	if (sheet.mode === rle.MODE_PLAYER && v < 64) {
		// approssimazione della tinta: valore v*8+s -> luminosità e saturazione del colore del giocatore (il gioco lo calcola in HSV)
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
	if (W * H > 64e6) throw new Error(T('la hoja sería demasiado grande (' + W + 'x' + H + '); usa --fotogramas', 'the sheet would be too large (' + W + 'x' + H + '); use --fotogramas', 'il foglio sarebbe troppo grande (' + W + 'x' + H + '); usa --fotogramas'));
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
	return T(`modo ${sheet.mode}, ${sheet.cols} columnas x ${sheet.rows} filas, ${used.length}/${sheet.cells.length} fotogramas con dibujo, ` +
		`paleta ${sheet.paletteCount}, fmt ${fmts}`,
		`mode ${sheet.mode}, ${sheet.cols} columns x ${sheet.rows} rows, ${used.length}/${sheet.cells.length} frames with image, ` +
		`palette ${sheet.paletteCount}, fmt ${fmts}`,
		`modo ${sheet.mode}, ${sheet.cols} colonne x ${sheet.rows} righe, ${used.length}/${sheet.cells.length} fotogrammi con disegno, ` +
		`tavolozza ${sheet.paletteCount}, fmt ${fmts}`);
}

const ERR = T('ERROR  ', 'ERROR  ', 'ERRORE  ');
let failures = 0;
for (const file of listRle(input)) {
	let buf = fs.readFileSync(file);
	const name = path.basename(file).replace(/\.rle(\.mmp)?$/i, '');
	let sheet;
	try { sheet = rle.parse(buf); } catch (e) { console.log(`${ERR}${file}: ${e.message}`); failures++; continue; }
	if (sheet.cells.some(c => !c.empty && !c.data)) {
		if (mmpFd === null) {
			console.log(T(`OMITIDO  ${file}: sus píxeles están fuera (rle.mmp); usa --rle-mmp <ruta del rle.mmp del juego>`,
				`SKIPPED  ${file}: its pixels are outside (rle.mmp); use --rle-mmp <path to the game's rle.mmp>`,
				`SALTATO  ${file}: i suoi pixel sono fuori (rle.mmp); usa --rle-mmp <percorso del rle.mmp del gioco>`));
			continue;
		}
		try { rle.resolveMmp(sheet, mmpFd); } catch (e) { console.log(`${ERR}${file}: ${e.message}`); failures++; continue; }
		buf = rle.write(sheet); // la versione «con i pixel dentro» di quel .rle.mmp: è quella con cui si confronta
	}
	if (flag('--verificar')) {
		const again = rle.write(rle.reencode(sheet)), same = again.equals(buf) && sheet.trailing === 0;
		if (same) { console.log(`${T('IGUAL  ', 'SAME  ', 'UGUALE  ')}${path.basename(file)}  ${describe(sheet)}`); continue; }
		// almeno la stessa immagine?
		const back = rle.parse(again);
		let samePixels = back.cells.length === sheet.cells.length;
		sheet.cells.forEach((c, i) => {
			const d = back.cells[i];
			if (c.empty !== d.empty) samePixels = false;
			else if (!c.empty && !Buffer.from(rle.decodeFrame(c).buffer).equals(Buffer.from(rle.decodeFrame(d).buffer))) samePixels = false;
		});
		let firstDiff = 0; while (firstDiff < Math.min(again.length, buf.length) && again[firstDiff] === buf[firstDiff]) firstDiff++;
		console.log(T(`DIFERENTE  ${path.basename(file)}  imagen ${samePixels ? 'idéntica' : 'DISTINTA'}; ${buf.length} vs ${again.length} bytes; primer byte distinto en ${firstDiff}; sobran ${sheet.trailing} bytes al final`,
			`DIFFERENT  ${path.basename(file)}  image ${samePixels ? 'identical' : 'DIFFERENT'}; ${buf.length} vs ${again.length} bytes; first differing byte at ${firstDiff}; ${sheet.trailing} extra bytes at the end`,
			`DIVERSO  ${path.basename(file)}  immagine ${samePixels ? 'identica' : 'DIVERSA'}; ${buf.length} vs ${again.length} byte; primo byte diverso a ${firstDiff}; ${sheet.trailing} byte in eccesso alla fine`));
		failures++;
		continue;
	}
	const dir = outDir || path.dirname(file);
	fs.mkdirSync(dir, { recursive: true });
	if (flag('--convenio')) {
		// Scrive ciò che si aspetta converti-sprite.js: un PNG per fotogramma e direzione, a tela piena, tinta in magenta, ombra a parte
		const m = /^(\d+)x(\d+)$/i.exec(opt('--lienzo') || '150x150');
		if (!m) { console.log(ERR + T('--lienzo debe ser AnchoxAlto', '--lienzo must be WidthxHeight', '--lienzo deve essere LarghezzaxAltezza')); failures++; continue; }
		const LW = +m[1], LH = +m[2], isShadow = sheet.mode === rle.MODE_SHADOW, base = name.replace(/_shadow$/i, ''), suffix = isShadow ? '_sombra' : '';
		let bad = false;
		sheet.cells.forEach((c, i) => {
			if (c.empty) return;
			if (c.x0 < 0 || c.y0 < 0 || c.x1 >= LW || c.y1 >= LH) { bad = true; return; }
			const img = frameImage(sheet, c, true), data = Buffer.alloc(LW * LH * 4);
			for (let y = 0; y < c.h; y++) img.data.copy(data, ((c.y0 + y) * LW + c.x0) * 4, y * c.w * 4, (y + 1) * c.w * 4);
			fs.writeFileSync(path.join(dir, `${base}_d${i % sheet.cols}_f${String((i / sheet.cols) | 0).padStart(2, '0')}${suffix}.png`), png.encode({ width: LW, height: LH, data }));
		});
		console.log(bad ? ERR + T(`${name}: hay fotogramas fuera del lienzo ${LW}x${LH}`, `${name}: there are frames outside the ${LW}x${LH} canvas`, `${name}: ci sono fotogrammi fuori dalla tela ${LW}x${LH}`)
			: T(`${name}: ${sheet.cols} direcciones x ${sheet.rows} fotogramas -> ${base}_d*_f*${suffix}.png (lienzo ${LW}x${LH})`,
				`${name}: ${sheet.cols} directions x ${sheet.rows} frames -> ${base}_d*_f*${suffix}.png (canvas ${LW}x${LH})`,
				`${name}: ${sheet.cols} direzioni x ${sheet.rows} fotogrammi -> ${base}_d*_f*${suffix}.png (tela ${LW}x${LH})`));
		if (bad) failures++;
		continue;
	}
	const sheetPng = toSheetPng(sheet);
	if (sheetPng) fs.writeFileSync(path.join(dir, name + '.png'), sheetPng);
	if (flag('--fotogramas')) {
		sheet.cells.forEach((c, i) => {
			if (c.empty) return;
			const d = String(i % sheet.cols).padStart(2, '0'), f = String((i / sheet.cols) | 0).padStart(3, '0');
			fs.writeFileSync(path.join(dir, `${name}_d${d}_f${f}.png`), png.encode(frameImage(sheet, c)));
		});
	}
	console.log(`${name}: ${describe(sheet)}${sheetPng ? ' -> ' + path.join(dir, name + '.png') : T(' (todo vacío)', ' (all empty)', ' (tutto vuoto)')}`);
}
process.exit(failures ? 1 : 0);
