// Formato .rle (IMGRLE / RLE2) di Imperivm, con i pixel DENTRO il file (SDK di Imperivm Resurrected).
// Lettore e scrittore senza dipendenze. Descrizione del formato: grafica.md.
//
//   const rle = require('./lib/rle');
//   const hoja = rle.parse(fs.readFileSync('Idle.rle'));       // legge
//   const bytes = rle.write(hoja);                              // scrive (se non è stato toccato nulla, restituisce gli stessi byte)
//   const px = rle.decodeFrame(hoja.cells[0]);                  // Int16Array w*h con l'indice di palette (-1 = trasparente)
//
// Tutto in little endian. Un file è:
//   "IMGRLE", modo u32, cella corrente (2 x u32 = 0,0), colonne u32, righe u32,
//   colonne*righe voci per righe (indice = riga*colonne + colonna): rect i32 x4 (x0,y0,x1,y1 inclusivo) e, solo se il rect non è
//   invertito, un blocco RLE2 (intestazione di 30 byte + dati),
//   n u32 colori di palette, n*4 byte (B,G,R,0), e solo nel modo 2 n*2 byte di tabella RGB555.
// Modi: 0 RGB555 diretto, 1 palette, 2 palette + colore del giocatore, 4 ombra, 5 nuvole, 6 alfa.
// Formato del fotogramma (fmt): 1 = 8 bpp con indici, 3 = 16 bpp RGB555, 7 = maschera a 1 bit (senza pixel).

const { T } = require('./lang');

const MODE_RGB = 0, MODE_PAL = 1, MODE_PLAYER = 2, MODE_SHADOW = 4;
const FMT_INDEX = 1, FMT_RGB555 = 3, FMT_MASK = 7;

// --- lettura ----------------------------------------------------------------------------------------------------------
function parse(buf) {
	if (buf.toString('latin1', 0, 6) !== 'IMGRLE') throw new Error(T('no es un .rle (falta la firma IMGRLE)', 'not a .rle (IMGRLE signature missing)', 'non è un .rle (manca la firma IMGRLE)'));
	let p = 6;
	const u32 = () => { const v = buf.readUInt32LE(p); p += 4; return v; };
	const i32 = () => { const v = buf.readInt32LE(p); p += 4; return v; };
	const mode = u32();
	const curCol = u32(), curRow = u32();
	const cols = u32(), rows = u32();
	if (cols > 4096 || rows > 4096) throw new Error(T('cabecera absurda: ' + cols + ' columnas x ' + rows + ' filas', 'absurd header: ' + cols + ' columns x ' + rows + ' rows', 'intestazione assurda: ' + cols + ' colonne x ' + rows + ' righe'));
	const cells = [];
	for (let i = 0; i < cols * rows; i++) {
		const cell = { x0: i32(), y0: i32(), x1: i32(), y1: i32(), empty: true };
		if (cell.x0 <= cell.x1 && cell.y0 <= cell.y1) {
			if (buf.toString('latin1', p, p + 4) !== 'RLE2') throw new Error(T('falta RLE2 en la celda ' + i + ' (offset ' + p + ')', 'RLE2 missing in cell ' + i + ' (offset ' + p + ')', 'manca RLE2 nella cella ' + i + ' (offset ' + p + ')'));
			cell.empty = false;
			cell.w = buf.readInt32LE(p + 4);
			cell.h = buf.readInt32LE(p + 8);
			cell.fmt = buf.readUInt16LE(p + 12);
			const dataSize = buf.readUInt32LE(p + 14);
			cell.ownPalette = buf.readUInt32LE(p + 18);
			cell.flag = buf.readUInt32LE(p + 22);
			cell.key = buf.readUInt32LE(p + 26);
			if (buf.toString('latin1', p + 30, p + 34) === 'pamm') {
				// .rle.mmp del gioco: al posto dei pixel c'è «pamm» + la posizione dentro il rle.mmp (vedi resolveMmp)
				cell.mmpOffset = buf.readUInt32LE(p + 34);
				cell.dataSize = dataSize;
				cell.data = null;
				p += 38;
			} else {
				cell.data = buf.slice(p + 30, p + 30 + dataSize);
				if (cell.data.length !== dataSize) throw new Error(T('archivo cortado en la celda ' + i, 'file cut short in cell ' + i, 'file troncato nella cella ' + i));
				p += 30 + dataSize;
			}
		}
		cells.push(cell);
	}
	const n = u32();
	const palette = Buffer.from(buf.slice(p, p + n * 4)); p += n * 4;
	let rgb555 = null;
	if (mode === MODE_PLAYER) { rgb555 = Buffer.from(buf.slice(p, p + n * 2)); p += n * 2; }
	return { mode, curCol, curRow, cols, rows, cells, paletteCount: n, palette, rgb555, trailing: buf.length - p };
}

// Per il debug con i file del gioco: un .rle.mmp tiene i pixel nel rle.mmp della cartella del gioco. Questo li porta nel file
// (leggendo il rle.mmp, in sola lettura) e lascia il foglio come un normale .rle con i pixel dentro.
function resolveMmp(sheet, fd) {
	const fs = require('fs');
	for (const c of sheet.cells) {
		if (c.empty || c.data) continue;
		c.data = Buffer.alloc(c.dataSize);
		if (fs.readSync(fd, c.data, 0, c.dataSize, c.mmpOffset) !== c.dataSize) throw new Error(T('rle.mmp más corto de lo esperado', 'rle.mmp is shorter than expected', 'rle.mmp è più corto del previsto'));
		delete c.mmpOffset; delete c.dataSize;
	}
	return sheet;
}

// Restituisce Int16Array w*h: indice di palette (fmt 1), valore RGB555 (fmt 3), 1 = ombra (fmt 7); -1 = trasparente.
function decodeFrame(cell) {
	const { w, h, fmt, data, flag } = cell;
	const out = new Int16Array(w * h).fill(-1);
	const step = flag ? 4 : 2;
	for (let y = 0; y < h; y++) {
		let p = flag ? data.readUInt32LE(y * 4) : data.readUInt16LE(y * 2);
		let x = 0;
		while (x < w) {
			const skip = data[p], n = data[p + 1]; p += 2;
			x += skip;
			for (let k = 0; k < n; k++) {
				let v;
				if (fmt === FMT_INDEX) v = data[p++];
				else if (fmt === FMT_RGB555) { v = data.readUInt16LE(p); p += 2; if (v > 32767) v -= 65536; }
				else v = 1;
				if (x < w) out[y * w + x] = v;
				x++;
			}
		}
	}
	return out;
}

// --- scrittura --------------------------------------------------------------------------------------------------------
// pixels: Int16Array w*h (-1 = trasparente). Restituisce {flag, data}
function encodeFrame(pixels, w, h, fmt) {
	const bodies = [], offs = [];
	let pos = 0;
	for (let y = 0; y < h; y++) {
		offs.push(pos);
		const out = [];
		let x = 0;
		const row = y * w;
		while (x < w) {
			let s = x;
			while (s < w && pixels[row + s] < 0) s++;
			let skip = s - x;
			if (s >= w) { while (skip > 255) { out.push(255, 0); skip -= 255; } out.push(skip, 0); break; }
			while (skip > 255) { out.push(255, 0); skip -= 255; }
			let e = s;
			while (e < w && pixels[row + e] >= 0 && e - s < 255) e++;
			out.push(skip, e - s);
			for (let k = s; k < e; k++) {
				const v = pixels[row + k];
				if (fmt === FMT_INDEX) out.push(v);
				else if (fmt === FMT_RGB555) out.push(v & 255, v >> 8);
			}
			x = e;
		}
		bodies.push(Buffer.from(out));
		pos += out.length;
	}
	// Regola verificata sui 200 939 fotogrammi del gioco: offset a 32 bit se la tabella da 4 byte per riga + le righe non sta in 16 bit
	const big = h * 4 + pos > 65535;
	const tbl = Buffer.alloc(h * (big ? 4 : 2));
	offs.forEach((o, i) => big ? tbl.writeUInt32LE(o + tbl.length, i * 4) : tbl.writeUInt16LE(o + tbl.length, i * 2));
	return { flag: big ? 1 : 0, data: Buffer.concat([tbl, ...bodies]) };
}

// Se la cella ha `pixels` (Int16Array) viene codificata; altrimenti se ne copiano i byte così come sono (per il viaggio di andata e ritorno).
function write(sheet) {
	const parts = [];
	const head = Buffer.alloc(26);
	head.write('IMGRLE', 0, 'latin1');
	head.writeUInt32LE(sheet.mode, 6);
	head.writeUInt32LE(sheet.curCol || 0, 10);
	head.writeUInt32LE(sheet.curRow || 0, 14);
	head.writeUInt32LE(sheet.cols, 18);
	head.writeUInt32LE(sheet.rows, 22);
	parts.push(head);
	if (sheet.cells.length !== sheet.cols * sheet.rows) throw new Error(T('hay ' + sheet.cells.length + ' celdas y la hoja es de ' + sheet.cols + 'x' + sheet.rows, 'there are ' + sheet.cells.length + ' cells and the sheet is ' + sheet.cols + 'x' + sheet.rows, 'ci sono ' + sheet.cells.length + ' celle e il foglio è di ' + sheet.cols + 'x' + sheet.rows));
	for (const c of sheet.cells) {
		const r = Buffer.alloc(16);
		if (c.empty) { // cella vuota: rect invertito; se viene da un file si conserva quello che aveva
			const has = c.x0 !== undefined;
			r.writeInt32LE(has ? c.x0 : 0, 0); r.writeInt32LE(has ? c.y0 : 0, 4); r.writeInt32LE(has ? c.x1 : -1, 8); r.writeInt32LE(has ? c.y1 : -1, 12);
			parts.push(r); continue;
		}
		r.writeInt32LE(c.x0, 0); r.writeInt32LE(c.y0, 4); r.writeInt32LE(c.x1, 8); r.writeInt32LE(c.y1, 12);
		parts.push(r);
		let flag = c.flag, data = c.data;
		if (c.pixels) ({ flag, data } = encodeFrame(c.pixels, c.w, c.h, c.fmt));
		const b = Buffer.alloc(30);
		b.write('RLE2', 0, 'latin1');
		b.writeInt32LE(c.w, 4); b.writeInt32LE(c.h, 8);
		b.writeUInt16LE(c.fmt, 12); b.writeUInt32LE(data.length, 14);
		b.writeUInt32LE(c.ownPalette || 0, 18); b.writeUInt32LE(flag, 22); b.writeUInt32LE(c.key >>> 0, 26);
		parts.push(b, data);
	}
	const n = Buffer.alloc(4);
	n.writeUInt32LE(sheet.paletteCount, 0);
	parts.push(n, sheet.palette);
	if (sheet.mode === MODE_PLAYER) parts.push(sheet.rgb555);
	return Buffer.concat(parts);
}

// Rifà i byte di tutti i fotogrammi a partire dai loro pixel (per verificare che lo scrittore riproduca quello del gioco)
function reencode(sheet) {
	return Object.assign({}, sheet, {
		cells: sheet.cells.map(c => c.empty ? c : Object.assign({}, c, { pixels: decodeFrame(c) })),
	});
}

module.exports = { parse, resolveMmp, write, decodeFrame, encodeFrame, reencode, MODE_RGB, MODE_PAL, MODE_PLAYER, MODE_SHADOW, FMT_INDEX, FMT_RGB555, FMT_MASK };
