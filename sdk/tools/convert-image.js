// Converts a PNG into a game image (16-bit BMP) for order buttons and icons (SDK).
//
//   node convert-image.js <input.png> <output.bmp> --type button|icon [--background #RRGGBB]
//
//   --type button   An order's button (the button="..." attribute of <cmd>). 153 x 51: three 51 x 51 cells (normal, mouse over
//                  and disabled). The input is a 51 x 51 PNG (scaled, keeping its proportions, if it has another size) and the
//                  other two cells are generated automatically. A 153 x 51 PNG is used as is (it already has the three cells).
//   --type icon   The portrait of a unit or a queue entry (icon="..." or queueicon="..."). 65 x 76. The transparent parts of the PNG are
//                  filled with the pure green that the game takes as transparent.
//
// The resulting file goes in the mod folder with the path the game uses:
//   button  gameres/cmdbar/<whatever you put in button=>     (button="Actions/Respiro.bmp" -> gameres/cmdbar/Actions/Respiro.bmp)
//   icon    gameres/icons/<name>.bmp                         (icon="gameres/icons/MyUnit.bmp")
const fs = require('fs');
const png = require('./lib/png.js');

const args = process.argv.slice(2);
const opt = n => { const i = args.indexOf(n); return i >= 0 ? args[i + 1] : null; };
const pos = args.filter((a, i) => !a.startsWith('--') && !(i > 0 && args[i - 1].startsWith('--')));
const [input, output] = pos;
const type = opt('--type');
const bgHex = (opt('--background') || '#281e14').replace('#', '');
if (!input || !output || !['button', 'icon'].includes(type)) {
	console.error('usage: node convert-image.js <input.png> <output.bmp> --type button|icon [--background #RRGGBB]');
	process.exit(2);
}
const BG = [parseInt(bgHex.slice(0, 2), 16), parseInt(bgHex.slice(2, 4), 16), parseInt(bgHex.slice(4, 6), 16)];

const src = png.decode(fs.readFileSync(input));

// Scales (keeping its proportions) to w x h, centered, with bilinear interpolation; any leftover area stays transparent
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
			// with premultiplied alpha so that the transparent edge does not dirty the colors
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

// 16-bit BMP (5-5-5), bottom-up, like the game's. pixel(x, y) -> 16-bit value
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

if (type === 'icon') {
	const W = 65, H = 76, img = fit(src, W, H);
	const KEY = 0x03e0;  // pure green: the color the game takes as transparent
	fs.writeFileSync(output, writeBmp(W, H, (x, y) => {
		const i = (y * W + x) * 4, a = img.data[i + 3];
		if (a < 128) return KEY;
		const v = to555(img.data[i], img.data[i + 1], img.data[i + 2]);
		return v === KEY ? KEY ^ 1 : v;  // so that no real color is mistaken for transparency
	}));
	console.log(`icon ${W}x${H} written to ${output}`);
}
else {
	const S = 51;
	let frames;
	if (src.width === 153 && src.height === 51) {
		frames = [0, 1, 2].map(f => ({ width: S, height: S, data: Buffer.from(Array.from({ length: S * S * 4 }, (_, k) => src.data[((Math.floor(k / 4 / S)) * 153 + f * S + (Math.floor(k / 4) % S)) * 4 + (k % 4)])) }));
	}
	else {
		const base = fit(src, S, S);
		// transparent parts are placed over the button background
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
	console.log(`button 153x51 (3 frames) written to ${output}`);
}
