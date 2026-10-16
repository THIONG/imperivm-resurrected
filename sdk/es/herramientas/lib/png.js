// PNG mínimo para las herramientas del SDK (sin dependencias): lee PNG de 8 bits (grises, RGB, paleta, con o sin alfa, sin entrelazado)
// y escribe PNG RGBA de 8 bits.
const zlib = require('zlib');
const { T } = require('./lang');

function crc32(buf) {
	let c, crc = 0xffffffff;
	for (let n = 0; n < buf.length; n++) {
		c = (crc ^ buf[n]) & 0xff;
		for (let k = 0; k < 8; k++) c = c & 1 ? 0xedb88320 ^ (c >>> 1) : c >>> 1;
		crc = (crc >>> 8) ^ c;
	}
	return (crc ^ 0xffffffff) >>> 0;
}

// Devuelve {width, height, data} con data = Buffer RGBA (4 bytes por píxel)
function decode(file) {
	if (file.readUInt32BE(0) !== 0x89504e47) throw new Error(T('no es un PNG', 'not a PNG', 'non è un PNG'));
	let pos = 8, w = 0, h = 0, depth = 0, type = 0, interlace = 0, palette = null, trns = null;
	const idat = [];
	while (pos < file.length) {
		const len = file.readUInt32BE(pos), name = file.toString('latin1', pos + 4, pos + 8), body = file.slice(pos + 8, pos + 8 + len);
		pos += 12 + len;
		if (name === 'IHDR') { w = body.readUInt32BE(0); h = body.readUInt32BE(4); depth = body[8]; type = body[9]; interlace = body[12]; }
		else if (name === 'PLTE') palette = body;
		else if (name === 'tRNS') trns = body;
		else if (name === 'IDAT') idat.push(body);
		else if (name === 'IEND') break;
	}
	if (depth !== 8) throw new Error(T('solo se admiten PNG de 8 bits por canal (este tiene ' + depth + ')', 'only PNG with 8 bits per channel are supported (this one has ' + depth + ')', 'sono ammessi solo PNG a 8 bit per canale (questo ne ha ' + depth + ')'));
	if (interlace) throw new Error(T('los PNG entrelazados no se admiten: guárdalo sin entrelazar', 'interlaced PNG are not supported: save it without interlacing', 'i PNG interlacciati non sono ammessi: salvalo senza interlacciamento'));
	const channels = { 0: 1, 2: 3, 3: 1, 4: 2, 6: 4 }[type];
	if (!channels) throw new Error(T('tipo de PNG no admitido: ' + type, 'unsupported PNG type: ' + type, 'tipo di PNG non ammesso: ' + type));
	const raw = zlib.inflateSync(Buffer.concat(idat));
	const stride = w * channels, px = Buffer.alloc(h * stride);
	let prev = Buffer.alloc(stride);
	for (let y = 0; y < h; y++) {
		const f = raw[y * (stride + 1)], line = raw.slice(y * (stride + 1) + 1, (y + 1) * (stride + 1)), out = px.slice(y * stride, (y + 1) * stride);
		for (let i = 0; i < stride; i++) {
			const a = i >= channels ? out[i - channels] : 0, b = prev[i], c = i >= channels ? prev[i - channels] : 0;
			let v = line[i];
			if (f === 1) v += a;
			else if (f === 2) v += b;
			else if (f === 3) v += (a + b) >> 1;
			else if (f === 4) { const p = a + b - c, pa = Math.abs(p - a), pb = Math.abs(p - b), pc = Math.abs(p - c); v += pa <= pb && pa <= pc ? a : pb <= pc ? b : c; }
			out[i] = v & 255;
		}
		prev = out;
	}
	const data = Buffer.alloc(w * h * 4);
	for (let i = 0; i < w * h; i++) {
		let r, g, b, a = 255;
		if (type === 6) { r = px[i * 4]; g = px[i * 4 + 1]; b = px[i * 4 + 2]; a = px[i * 4 + 3]; }
		else if (type === 2) { r = px[i * 3]; g = px[i * 3 + 1]; b = px[i * 3 + 2]; }
		else if (type === 0) { r = g = b = px[i]; }
		else if (type === 4) { r = g = b = px[i * 2]; a = px[i * 2 + 1]; }
		else { const k = px[i]; r = palette[k * 3]; g = palette[k * 3 + 1]; b = palette[k * 3 + 2]; if (trns && k < trns.length) a = trns[k]; }
		data[i * 4] = r; data[i * 4 + 1] = g; data[i * 4 + 2] = b; data[i * 4 + 3] = a;
	}
	return { width: w, height: h, data };
}

function chunk(name, body) {
	const out = Buffer.alloc(12 + body.length);
	out.writeUInt32BE(body.length, 0);
	out.write(name, 4, 'latin1');
	body.copy(out, 8);
	out.writeUInt32BE(crc32(out.slice(4, 8 + body.length)), 8 + body.length);
	return out;
}

// image: {width, height, data (RGBA)}
function encode(image) {
	const { width: w, height: h, data } = image;
	const raw = Buffer.alloc(h * (w * 4 + 1));
	for (let y = 0; y < h; y++) {
		raw[y * (w * 4 + 1)] = 0;
		data.copy(raw, y * (w * 4 + 1) + 1, y * w * 4, (y + 1) * w * 4);
	}
	const ihdr = Buffer.alloc(13);
	ihdr.writeUInt32BE(w, 0); ihdr.writeUInt32BE(h, 4); ihdr[8] = 8; ihdr[9] = 6;
	return Buffer.concat([Buffer.from([0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a]), chunk('IHDR', ihdr), chunk('IDAT', zlib.deflateSync(raw)), chunk('IEND', Buffer.alloc(0))]);
}

module.exports = { decode, encode };
