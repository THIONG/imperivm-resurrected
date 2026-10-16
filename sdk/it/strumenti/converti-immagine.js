// Converte un PNG in un'immagine del gioco (BMP a 16 bit) per i pulsanti degli ordini e per le icone (SDK).
//
//   node converti-immagine.js <input.png> <output.bmp> --tipo pulsante|icona [--sfondo #RRGGBB]
//
//   --tipo pulsante   Il pulsante di un ordine (l'attributo button="..." di <cmd>). 153 x 51: tre riquadri da 51 x 51 (normale, con il
//                  mouse sopra e disattivato). Accetta un PNG da 51 x 51 (viene scalato se ha un'altra dimensione, mantenendo le proporzioni) e
//                  gli altri due riquadri si creano da soli. Un PNG da 153 x 51 si usa così com'è (ha già i tre riquadri).
//   --tipo icona   Il ritratto di un'unità o di una coda (icon="..." o queueicon="..."). 65 x 76. Il trasparente del PNG viene
//                  reso con il verde puro che il gioco considera trasparente.
//
// Il file risultante va nella cartella della mod con il percorso che usa il gioco:
//   pulsante gameres/cmdbar/<ciò che metti in button=>     (button="Actions/Respiro.bmp" -> gameres/cmdbar/Actions/Respiro.bmp)
//   icona    gameres/icons/<nome>.bmp                     (icon="gameres/icons/MiUnidad.bmp")
const fs = require('fs');
const png = require('./lib/png.js');
const { T } = require('./lib/lang');

const args = process.argv.slice(2);
const opt = n => { const i = args.indexOf(n); return i >= 0 ? args[i + 1] : null; };
const pos = args.filter((a, i) => !a.startsWith('--') && !(i > 0 && args[i - 1].startsWith('--')));
const [input, output] = pos;
const type = opt('--tipo');
const bgHex = (opt('--fondo') || '#281e14').replace('#', '');
if (!input || !output || !['boton', 'icono'].includes(type)) {
	console.error(T('uso: node converti-immagine.js <entrada.png> <salida.bmp> --tipo boton|icono [--fondo #RRGGBB]',
		'usage: node converti-immagine.js <input.png> <output.bmp> --tipo boton|icono [--fondo #RRGGBB]',
		'uso: node converti-immagine.js <ingresso.png> <uscita.bmp> --tipo boton|icono [--fondo #RRGGBB]'));
	process.exit(2);
}
const BG = [parseInt(bgHex.slice(0, 2), 16), parseInt(bgHex.slice(2, 4), 16), parseInt(bgHex.slice(4, 6), 16)];

const src = png.decode(fs.readFileSync(input));

// Scala (mantenendo le proporzioni) a w x h, centrata, con interpolazione bilineare; ciò che avanza resta trasparente
function fit(img, w, h) {
	if (img.width === w && img.height === h) return img;
	const k = Math.min(w / img.width, h / img.height), nw = Math.max(1, Math.round(img.width * k)), nh = Math.max(1, Math.round(img.height * k));
	const ox = Math.floor((w - nw) / 2), oy = Math.floor((h - nh) / 2);
	const out = Buffer.alloc(w * h * 4);
	for (let y = 0; y < nh; y++) {
		for (let x = 0; x < nw; x++) {
			const fx = (x + 0.5) / k - 0.5, fy = (y + 0.5) / k - 0.5;
			const x0 = Math.max(0, Math.min(img.width - 1, Math.floor(fx))), y0 = Math.max(0, Math.min(img.height - 1, Math.floor(fy)));
			const x1 = Math.min(img.width - 1, x0 + 1), y1 = Math.min(img.height - 1, y0 + 1);
			const tx = Math.max(0, Math.min(1, fx - x0)), ty = Math.max(0, Math.min(1, fy - y0));
			const o = ((y + oy) * w + x + ox) * 4;
			// con alfa premoltiplicato perché il bordo trasparente non sporchi i colori
			let a = 0, r = 0, g = 0, b = 0;
			for (const [px, py, wt] of [[x0, y0, (1 - tx) * (1 - ty)], [x1, y0, tx * (1 - ty)], [x0, y1, (1 - tx) * ty], [x1, y1, tx * ty]]) {
				const i = (py * img.width + px) * 4, al = img.data[i + 3] / 255 * wt;
				r += img.data[i] * al; g += img.data[i + 1] * al; b += img.data[i + 2] * al; a += al;
			}
			out[o] = a ? r / a : 0; out[o + 1] = a ? g / a : 0; out[o + 2] = a ? b / a : 0; out[o + 3] = Math.round(a * 255);
		}
	}
	return { width: w, height: h, data: out };
}

// BMP a 16 bit (5-5-5), dal basso verso l'alto, come quelli del gioco. pixel(x, y) -> valore a 16 bit
function writeBmp(w, h, pixel) {
	const stride = ((w * 2 + 3) >> 2) << 2, size = stride * h, b = Buffer.alloc(54 + size);
	b.write('BM', 0);
	b.writeUInt32LE(54 + size, 2); b.writeUInt32LE(54, 10); b.writeUInt32LE(40, 14);
	b.writeInt32LE(w, 18); b.writeInt32LE(h, 22); b.writeUInt16LE(1, 26); b.writeUInt16LE(16, 28); b.writeUInt32LE(0, 30);
	b.writeUInt32LE(size, 34); b.writeInt32LE(2834, 38); b.writeInt32LE(2834, 42);
	for (let y = 0; y < h; y++) for (let x = 0; x < w; x++) b.writeUInt16LE(pixel(x, h - 1 - y), 54 + y * stride + x * 2);
	return b;
}
const to555 = (r, g, b) => ((r >> 3) << 10) | ((g >> 3) << 5) | (b >> 3);

if (type === 'icono') {
	const W = 65, H = 76, img = fit(src, W, H);
	const KEY = 0x03e0;  // verde puro: il colore che il gioco considera trasparente
	fs.writeFileSync(output, writeBmp(W, H, (x, y) => {
		const i = (y * W + x) * 4, a = img.data[i + 3];
		if (a < 128) return KEY;
		const v = to555(img.data[i], img.data[i + 1], img.data[i + 2]);
		return v === KEY ? KEY ^ 1 : v;  // perché nessun colore vero venga scambiato per la trasparenza
	}));
	console.log(T(`icono ${W}x${H} escrito en ${output}`, `icon ${W}x${H} written to ${output}`, `icona ${W}x${H} scritta in ${output}`));
}
else {
	const S = 51;
	let frames;
	if (src.width === 153 && src.height === 51) {
		frames = [0, 1, 2].map(f => ({ width: S, height: S, data: Buffer.from(Array.from({ length: S * S * 4 }, (_, k) => src.data[((Math.floor(k / 4 / S)) * 153 + f * S + (Math.floor(k / 4) % S)) * 4 + (k % 4)])) }));
	}
	else {
		const base = fit(src, S, S);
		// il trasparente si mette sopra lo sfondo del pulsante
		const flat = Buffer.alloc(S * S * 4);
		for (let i = 0; i < S * S; i++) {
			const a = base.data[i * 4 + 3] / 255;
			for (let c = 0; c < 3; c++) flat[i * 4 + c] = Math.round(base.data[i * 4 + c] * a + BG[c] * (1 - a));
			flat[i * 4 + 3] = 255;
		}
		const hover = Buffer.from(flat), off = Buffer.from(flat);
		for (let i = 0; i < S * S; i++) {
			for (let c = 0; c < 3; c++) hover[i * 4 + c] = Math.min(255, Math.round(flat[i * 4 + c] * 1.18 + 6));
			const gray = Math.round(0.3 * flat[i * 4] + 0.59 * flat[i * 4 + 1] + 0.11 * flat[i * 4 + 2]);
			for (let c = 0; c < 3; c++) off[i * 4 + c] = Math.round(gray * 0.7);
		}
		frames = [flat, hover, off].map(data => ({ width: S, height: S, data }));
	}
	fs.writeFileSync(output, writeBmp(153, 51, (x, y) => {
		const f = Math.floor(x / S), i = (y * S + (x % S)) * 4, d = frames[f].data;
		return to555(d[i], d[i + 1], d[i + 2]);
	}));
	console.log(T(`botón 153x51 (3 cuadros) escrito en ${output}`, `button 153x51 (3 frames) written to ${output}`, `pulsante 153x51 (3 fotogrammi) scritto in ${output}`));
}
