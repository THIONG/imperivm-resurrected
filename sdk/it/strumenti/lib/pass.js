// Formato .pass (impronta di un edificio: dove non si può passare) di Imperivm (SDK di Imperivm Resurrected).
// Descrizione: grafica.md.
//
//   Intestazione di 20 byte: "DIRG", u32 0x10 (un pixel della griglia = 16 unità di mappa), u32 1 (bit per cella),
//   u32 w e u32 h in unità di mappa (0x800 x 0x800 in tutti quelli del gioco). Poi, una griglia a 1 bit di (w/16) x (h/16) = 128 x 128
//   celle (2048 byte): riga b, colonna a = bit numero (b*128 + a) di una striscia di dword little endian (bit 0 = il meno significativo).
//   File completo: 2068 byte.
// Orientamento (dedotto da StampFootprintToGrid1 e VERIFICATO disegnandolo sullo sprite della caserma romana): la colonna a cresce verso
// destra sullo schermo e la riga b cresce VERSO L'ALTO (verso nord). L'origine dell'edificio (il punto in cui è «inchiodato»,
// quello degli spostamenti di livello dell'.ent.xml) cade al centro della cella (63, 64).
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

// cells: Uint8Array CELLS*CELLS (riga b * 128 + colonna a)
function encode(cells, w = CELLS, h = CELLS) {
	const out = Buffer.alloc(20 + w * h / 8);
	out.write(MAGIC, 0, 'latin1');
	out.writeUInt32LE(0x10, 4); out.writeUInt32LE(1, 8); out.writeUInt32LE(w * 16, 12); out.writeUInt32LE(h * 16, 16);
	for (let n = 0; n < w * h; n++) if (cells[n]) out[20 + (n >> 3)] |= 1 << (n & 7);
	return out;
}

// Rettangolo di larghezza x altezza celle centrato sull'origine dell'edificio
function rectangle(width, height) {
	const cells = new Uint8Array(CELLS * CELLS);
	const a0 = OX - (width >> 1), b0 = OY - (height >> 1);
	if (a0 < 0 || b0 < 0 || a0 + width > CELLS || b0 + height > CELLS) throw new Error(T(`un rectángulo de ${width}x${height} celdas no cabe (máximo unas ${2 * Math.min(OX, OY)})`, `a rectangle of ${width}x${height} cells does not fit (about ${2 * Math.min(OX, OY)} at most)`, `un rettangolo di ${width}x${height} celle non ci sta (al massimo circa ${2 * Math.min(OX, OY)})`));
	for (let b = b0; b < b0 + height; b++) for (let a = a0; a < a0 + width; a++) cells[b * CELLS + a] = 1;
	return cells;
}

// Maschera disegnata come immagine (nord in alto): ogni pixel = una cella; mask[y*w+x] != 0 = bloccato. Si centra sull'origine.
function fromMask(mask, width, height) {
	const cells = new Uint8Array(CELLS * CELLS);
	const a0 = OX - (width >> 1), b0 = OY + (height >> 1); // la riga 0 dell'immagine (in alto) è la b più alta
	for (let y = 0; y < height; y++) for (let x = 0; x < width; x++) {
		if (!mask[y * width + x]) continue;
		const a = a0 + x, b = b0 - y;
		if (a < 0 || a >= CELLS || b < 0 || b >= CELLS) throw new Error(T('la máscara se sale de la rejilla de 128x128 celdas: reduce su tamaño', 'the mask goes outside the 128x128 cell grid: reduce its size', 'la maschera esce dalla griglia di 128x128 celle: riduci le sue dimensioni'));
		cells[b * CELLS + a] = 1;
	}
	return cells;
}

// L'inverso di fromMask, per vedere un'impronta come immagine: la ritaglia al suo riquadro e restituisce {width,height,mask,count}
function toMask(cells) {
	let a0 = CELLS, a1 = -1, b0 = CELLS, b1 = -1, count = 0;
	for (let b = 0; b < CELLS; b++) for (let a = 0; a < CELLS; a++) if (cells[b * CELLS + a]) { count++; if (a < a0) a0 = a; if (a > a1) a1 = a; if (b < b0) b0 = b; if (b > b1) b1 = b; }
	if (!count) return { width: 0, height: 0, mask: new Uint8Array(0), count: 0 };
	const width = a1 - a0 + 1, height = b1 - b0 + 1, mask = new Uint8Array(width * height);
	for (let y = 0; y < height; y++) for (let x = 0; x < width; x++) mask[y * width + x] = cells[(b1 - y) * CELLS + a0 + x];
	return { width, height, mask, count };
}

module.exports = { CELLS, OX, OY, decode, encode, rectangle, fromMask, toMask };
