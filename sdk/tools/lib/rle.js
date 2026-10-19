// Imperivm's .rle format (IMGRLE / RLE2), with the pixels INSIDE the file (Imperivm Resurrected SDK).
// Dependency-free reader and writer. Format description: graphics.md.
//
//   const rle = require('./lib/rle');
//   const hoja = rle.parse(fs.readFileSync('Idle.rle'));       // read
//   const bytes = rle.write(hoja);                              // write (if nothing was touched, it returns the same bytes)
//   const px = rle.decodeFrame(hoja.cells[0]);                  // Int16Array w*h with the palette index (-1 = transparent)
//
// Everything is little endian. A file is:
//   "IMGRLE", u32 mode, current cell (2 x u32 = 0,0), u32 columns, u32 rows,
//   columns*rows entries by rows (index = row*columns + column): rect i32 x4 (x0,y0,x1,y1 inclusive) and, only if the rect is not
//   inverted, an RLE2 block (30-byte header + data),
//   n u32 palette colors, n*4 bytes (B,G,R,0), and only in mode 2 n*2 bytes of RGB555 table.
// Modes: 0 direct RGB555, 1 palette, 2 palette + player color, 4 shadow, 5 clouds, 6 alpha.
// Frame format (fmt): 1 = 8 bpp with indices, 3 = 16 bpp RGB555, 7 = 1-bit mask (no pixels).


const MODE_RGB = 0, MODE_PAL = 1, MODE_PLAYER = 2, MODE_SHADOW = 4;
const FMT_INDEX = 1, FMT_RGB555 = 3, FMT_MASK = 7;

// --- reading ----------------------------------------------------------------------------------------------------------
function parse(buf) {
	if (buf.toString('latin1', 0, 6) !== 'IMGRLE') throw new Error('not a .rle (IMGRLE signature missing)');
	let p = 6;
	const u32 = () => { const v = buf.readUInt32LE(p); p += 4; return v; };
	const i32 = () => { const v = buf.readInt32LE(p); p += 4; return v; };
	const mode = u32();
	const curCol = u32(), curRow = u32();
	const cols = u32(), rows = u32();
	if (cols > 4096 || rows > 4096) throw new Error(('absurd header: ' + cols + ' columns x ' + rows + ' rows'));
	const cells = [];
	for (let i = 0; i < cols * rows; i++) {
		const cell = { x0: i32(), y0: i32(), x1: i32(), y1: i32(), empty: true };
		if (cell.x0 <= cell.x1 && cell.y0 <= cell.y1) {
			if (buf.toString('latin1', p, p + 4) !== 'RLE2') throw new Error(('RLE2 missing in cell ' + i + ' (offset ' + p + ')'));
			cell.empty = false;
			cell.w = buf.readInt32LE(p + 4);
			cell.h = buf.readInt32LE(p + 8);
			cell.fmt = buf.readUInt16LE(p + 12);
			const dataSize = buf.readUInt32LE(p + 14);
			cell.ownPalette = buf.readUInt32LE(p + 18);
			cell.flag = buf.readUInt32LE(p + 22);
			cell.key = buf.readUInt32LE(p + 26);
			if (buf.toString('latin1', p + 30, p + 34) === 'pamm') {
				// game .rle.mmp: instead of the pixels there is "pamm" + position inside rle.mmp (see resolveMmp)
				cell.mmpOffset = buf.readUInt32LE(p + 34);
				cell.dataSize = dataSize;
				cell.data = null;
				p += 38;
			} else {
				cell.data = buf.slice(p + 30, p + 30 + dataSize);
				if (cell.data.length !== dataSize) throw new Error(('file cut short in cell ' + i));
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

// For debugging with game files: a .rle.mmp keeps its pixels in the rle.mmp in the game folder. This brings them into the file
// (reading the rle.mmp, read-only) and leaves the sheet as a normal .rle with the pixels inside.
function resolveMmp(sheet, fd) {
	const fs = require('fs');
	for (const c of sheet.cells) {
		if (c.empty || c.data) continue;
		c.data = Buffer.alloc(c.dataSize);
		if (fs.readSync(fd, c.data, 0, c.dataSize, c.mmpOffset) !== c.dataSize) throw new Error('rle.mmp is shorter than expected');
		delete c.mmpOffset; delete c.dataSize;
	}
	return sheet;
}

// Returns Int16Array w*h: palette index (fmt 1), RGB555 value (fmt 3), 1 = shadow (fmt 7); -1 = transparent.
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

// --- writing ----------------------------------------------------------------------------------------------------------
// pixels: Int16Array w*h (-1 = transparent). Returns {flag, data}
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
	// Rule verified on the game's 200,939 frames: 32-bit offsets if the 4-byte-per-row table + rows does not fit in 16 bits
	const big = h * 4 + pos > 65535;
	const tbl = Buffer.alloc(h * (big ? 4 : 2));
	offs.forEach((o, i) => big ? tbl.writeUInt32LE(o + tbl.length, i * 4) : tbl.writeUInt16LE(o + tbl.length, i * 2));
	return { flag: big ? 1 : 0, data: Buffer.concat([tbl, ...bodies]) };
}

// If the cell carries `pixels` (Int16Array) it is encoded; otherwise its bytes are copied as they are (for the round trip).
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
	if (sheet.cells.length !== sheet.cols * sheet.rows) throw new Error(('there are ' + sheet.cells.length + ' cells and the sheet is ' + sheet.cols + 'x' + sheet.rows));
	for (const c of sheet.cells) {
		const r = Buffer.alloc(16);
		if (c.empty) { // empty cell: inverted rect; if it comes from a file, the one it carried is kept
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

// Rebuilds the bytes of all the frames from their pixels (to check that the writer reproduces the game's)
function reencode(sheet) {
	return Object.assign({}, sheet, {
		cells: sheet.cells.map(c => c.empty ? c : Object.assign({}, c, { pixels: decodeFrame(c) })),
	});
}

module.exports = { parse, resolveMmp, write, decodeFrame, encodeFrame, reencode, MODE_RGB, MODE_PAL, MODE_PLAYER, MODE_SHADOW, FMT_INDEX, FMT_RGB555, FMT_MASK };
