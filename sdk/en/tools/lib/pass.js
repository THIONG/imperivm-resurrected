// Imperivm's .pass format (a building's footprint: where you cannot walk) (Imperivm Resurrected SDK).
// Description: graphics.md.
//
//   20-byte header: "DIRG", u32 0x10 (one grid pixel = 16 map units), u32 1 (bits per cell),
//   u32 w and u32 h in map units (0x800 x 0x800 in all of the game's). Then, a 1-bit grid of (w/16) x (h/16) = 128 x 128
//   cells (2048 bytes): row b, column a = bit number (b*128 + a) of a strip of little endian dwords (bit 0 = the least significant).
//   Whole file: 2068 bytes.
// Orientation (deduced from StampFootprintToGrid1 and VERIFIED by drawing it over the Roman barracks sprite): column a grows toward
// the right of the screen and row b grows UPWARD (toward the north). The building's origin (the point where it is "pinned",
// the one the .ent.xml layer offsets use) falls in the center of cell (63, 64).
const { T } = require('./lang');

const CELLS = 128, MAGIC = 'DIRG', OX = 63, OY = 64;

function decode(buf) {
	if (buf.toString('latin1', 0, 4) !== MAGIC) throw new Error(T('no es un .pass (falta DIRG)', 'not a .pass (DIRG missing)', 'non è un .pass (manca DIRG)'));
	const unit = buf.readUInt32LE(4), bits = buf.readUInt32LE(8), W = buf.readUInt32LE(12), H = buf.readUInt32LE(16);
	if (bits !== 1) throw new Error(T('bits por celda distinto de 1: ' + bits, 'bits per cell other than 1: ' + bits, 'bit per cella diversi da 1: ' + bits));
	const w = W / unit, h = H / unit;
	if (buf.length !== 20 + w * h / 8) throw new Error(T(`tamaño inesperado: ${buf.length} bytes para ${w}x${h} celdas`, `unexpected size: ${buf.length} bytes for ${w}x${h} cells`, `dimensione inattesa: ${buf.length} byte per ${w}x${h} celle`));
	const cells = new Uint8Array(w * h);
	for (let n = 0; n < w * h; n++) cells[n] = (buf[20 + (n >> 3)] >> (n & 7)) & 1;
	return { unit, W, H, w, h, cells };
}

// cells: Uint8Array CELLS*CELLS (row b * 128 + column a)
function encode(cells, w = CELLS, h = CELLS) {
	const out = Buffer.alloc(20 + w * h / 8);
	out.write(MAGIC, 0, 'latin1');
	out.writeUInt32LE(0x10, 4); out.writeUInt32LE(1, 8); out.writeUInt32LE(w * 16, 12); out.writeUInt32LE(h * 16, 16);
	for (let n = 0; n < w * h; n++) if (cells[n]) out[20 + (n >> 3)] |= 1 << (n & 7);
	return out;
}

// Rectangle of width x height cells centered on the building's origin
function rectangle(width, height) {
	const cells = new Uint8Array(CELLS * CELLS);
	const a0 = OX - (width >> 1), b0 = OY - (height >> 1);
	if (a0 < 0 || b0 < 0 || a0 + width > CELLS || b0 + height > CELLS) throw new Error(T(`un rectángulo de ${width}x${height} celdas no cabe (máximo unas ${2 * Math.min(OX, OY)})`, `a rectangle of ${width}x${height} cells does not fit (about ${2 * Math.min(OX, OY)} at most)`, `un rettangolo di ${width}x${height} celle non ci sta (al massimo circa ${2 * Math.min(OX, OY)})`));
	for (let b = b0; b < b0 + height; b++) for (let a = a0; a < a0 + width; a++) cells[b * CELLS + a] = 1;
	return cells;
}

// Mask drawn as an image (north up): each pixel = one cell; mask[y*w+x] != 0 = blocked. Centered on the origin.
function fromMask(mask, width, height) {
	const cells = new Uint8Array(CELLS * CELLS);
	const a0 = OX - (width >> 1), b0 = OY + (height >> 1); // row 0 of the image (top) is the highest b
	for (let y = 0; y < height; y++) for (let x = 0; x < width; x++) {
		if (!mask[y * width + x]) continue;
		const a = a0 + x, b = b0 - y;
		if (a < 0 || a >= CELLS || b < 0 || b >= CELLS) throw new Error(T('la máscara se sale de la rejilla de 128x128 celdas: reduce su tamaño', 'the mask goes outside the 128x128 cell grid: reduce its size', 'la maschera esce dalla griglia di 128x128 celle: riduci le sue dimensioni'));
		cells[b * CELLS + a] = 1;
	}
	return cells;
}

// The inverse of fromMask, to view a footprint as an image: crops to its box and returns {width,height,mask,count}
function toMask(cells) {
	let a0 = CELLS, a1 = -1, b0 = CELLS, b1 = -1, count = 0;
	for (let b = 0; b < CELLS; b++) for (let a = 0; a < CELLS; a++) if (cells[b * CELLS + a]) { count++; if (a < a0) a0 = a; if (a > a1) a1 = a; if (b < b0) b0 = b; if (b > b1) b1 = b; }
	if (!count) return { width: 0, height: 0, mask: new Uint8Array(0), count: 0 };
	const width = a1 - a0 + 1, height = b1 - b0 + 1, mask = new Uint8Array(width * height);
	for (let y = 0; y < height; y++) for (let x = 0; x < width; x++) mask[y * width + x] = cells[(b1 - y) * CELLS + a0 + x];
	return { width, height, mask, count };
}

module.exports = { CELLS, OX, OY, decode, encode, rectangle, fromMask, toMask };
