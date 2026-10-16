// Draws a simple soldier (made of "voxels", little cubes) in 8 directions and 6 animations, and converts it to the game's format
// with the SDK tools. It's an EXAMPLE to see the whole path: you'll draw your sprites with your own program
// (Photoshop, Aseprite, Blender…) and you'll skip the first step; the other two are the same.
//
//   node draw-soldier.js [--png <folder>]
//
//   1. It draws the PNGs (one file per animation, direction and frame: Walk_d3_f05.png) in a temporary folder
//      (or in --png <folder> if you want to see them). Canvas 150x150, feet at (75,100). The tunic and the crest are MAGENTA:
//      that becomes the player's color.
//   2. node ..\..\tools\convert-sprite.js <png> --player --output ..\unit-graphics-example\units\RSimpleSoldier
//   3. node ..\..\tools\create-entity.js unit ..\unit-graphics-example\units\RSimpleSoldier --name RSimpleSoldier
//
// Output goes to ..\unit-graphics-example\units\RSimpleSoldier\ (about 500 KB). The shadow isn't drawn: convert-sprite generates it as an ellipse under the feet.
const fs = require('fs'), os = require('os'), path = require('path'), cp = require('child_process');
const png = require('../../tools/lib/png');

const W = 150, H = 150, FEET = [75, 100], S = 3; // S = pixels per cube
const DIRS = 8;

// ---------- the model: list of cubes with their part (so it can be moved) ----------
// Local coordinates: x to the soldier's right, y up, z where it faces. Unit = 1 cube (S pixels).
const MAGENTA = [255, 0, 255], SKIN = [226, 178, 138], METAL = [176, 184, 192], DARK = [58, 42, 34], BROWN = [110, 72, 40], RED = [138, 43, 30], GOLD = [214, 170, 60], BLADE = [225, 230, 236];
function box(parts, part, x0, x1, y0, y1, z0, z1, color, shade) {
	for (let x = x0; x <= x1; x++) for (let y = y0; y <= y1; y++) for (let z = z0; z <= z1; z++) parts.push({ part, x, y, z, color, shade: shade || 0 });
}
function model() {
	const v = [];
	box(v, 'legL', -2, -1, 2, 5, -1, 0, BROWN);          box(v, 'legL', -2, -1, 0, 1, -1, 1, DARK);
	box(v, 'legR', 1, 2, 2, 5, -1, 0, BROWN);            box(v, 'legR', 1, 2, 0, 1, -1, 1, DARK);
	box(v, 'body', -2, 2, 6, 11, -1, 1, MAGENTA, 1);     // tunic, with brightness depending on height
	box(v, 'body', -2, 2, 6, 6, -1, 1, DARK);            // belt
	box(v, 'body', -1, 1, 8, 11, 2, 2, METAL);           // breastplate
	box(v, 'head', -1, 1, 12, 14, -1, 1, SKIN);
	box(v, 'head', -1, 1, 13, 13, 2, 2, DARK);           // eyes (one line)
	box(v, 'head', -2, 2, 14, 15, -1, 2, METAL);         // helmet
	box(v, 'head', 0, 0, 16, 17, -2, 2, MAGENTA, 1);     // crest
	box(v, 'armL', -3, -3, 7, 11, 0, 0, SKIN);           box(v, 'armL', -3, -3, 10, 11, -1, 1, MAGENTA, 1);
	box(v, 'shield', -4, -4, 5, 12, -1, 1, RED);         box(v, 'shield', -4, -4, 8, 9, 0, 0, GOLD);
	box(v, 'armR', 3, 3, 7, 11, 0, 0, SKIN);             box(v, 'armR', 3, 3, 10, 11, -1, 1, MAGENTA, 1);
	box(v, 'sword', 3, 3, 7, 7, 1, 2, BROWN);            box(v, 'sword', 3, 3, 7, 7, 3, 10, BLADE);
	return v;
}
const MODEL = model();
const PIVOT = { legL: [0, 6, 0], legR: [0, 6, 0], armL: [0, 11, 0], shield: [0, 11, 0], armR: [0, 11, 0], sword: [0, 11, 0], body: [0, 0, 0], head: [0, 0, 0] };

// rotates a point around the X axis (side to side) centered on p: a positive angle swings forward whatever hangs
function pitch(pt, c, a) {
	const dy = pt[1] - c[1], dz = pt[2] - c[2], ca = Math.cos(a), sa = Math.sin(a);
	return [pt[0], c[1] + dy * ca + dz * sa, c[2] + dz * ca - dy * sa];
}
const rad = d => d * Math.PI / 180;

// ---------- poses: angles of each part, vertical offset and fall ----------
const T = (i, n) => n <= 1 ? 0 : i / (n - 1);
const ANIMS = {
	Idle: { frames: 4, pose: (i, n) => { const t = Math.sin(i / n * Math.PI * 2); return { bob: Math.round(t * 0.4 + 0.4), armR: 8 + 4 * t, armL: -6 - 3 * t, sword: 8 + 4 * t, shield: -6 - 3 * t }; } },
	Walk: { frames: 8, pose: (i, n) => { const t = Math.sin(i / n * Math.PI * 2); return { legL: 38 * t, legR: -38 * t, armL: -26 * t, shield: -26 * t, armR: 26 * t, sword: 26 * t, bob: Math.abs(Math.cos(i / n * Math.PI * 2)) > 0.7 ? 1 : 0 }; } },
	Fight: { frames: 6, pose: (i, n) => { const t = T(i, n); const a = t < 0.4 ? -150 * (t / 0.4) : -150 + 200 * Math.min(1, (t - 0.4) / 0.3) - (t > 0.7 ? 50 * (t - 0.7) / 0.3 : 0); return { armR: a, sword: a, armL: -20, shield: -20, legL: 14, legR: -14 }; } },
	Death: { frames: 8, pose: (i, n) => { const t = T(i, n); return { fall: 90 * Math.min(1, t * 1.15), armR: -40 * t, sword: -40 * t, armL: 50 * t, shield: 50 * t, legL: 10 * t, legR: -10 * t }; } },
	Transition: { frames: 4, pose: (i, n) => { const t = T(i, n); return { armR: -70 * t, sword: -70 * t, armL: -20 * t, shield: -20 * t }; } },
	Taunt: { frames: 6, pose: (i, n) => { const t = Math.sin(i / n * Math.PI * 2); return { armR: -170 + 10 * t, sword: -170 + 10 * t, armL: -10, shield: -10, bob: t > 0.5 ? 1 : 0 }; } },
};

// ---------- drawing ----------
function drawFrame(anim, dir, i) {
	const A = ANIMS[anim], p = A.pose(i, A.frames), phi = rad(dir * 45);
	const f = [-Math.sin(phi), Math.cos(phi)], r = [-Math.cos(phi), -Math.sin(phi)]; // forward and the soldier's "right" on the map (x east, z south)
	const px = [];
	for (const c of MODEL) {
		let pt = [c.x, c.y, c.z];
		const a = p[c.part];
		if (a) pt = pitch(pt, PIVOT[c.part], rad(a));
		if (c.part === 'sword' && p.sword) { /* ya girada arriba */ }
		pt[1] += p.bob || 0;
		if (p.fall) { pt = pitch(pt, [0, 0, 0], rad(p.fall)); pt[1] = Math.max(0, pt[1]); }
		// local -> map: x*(right) + z*(forward); the soldier's "right" when facing south is to the west (-x)
		const wx = pt[0] * r[0] + pt[2] * f[0], wz = pt[0] * r[1] + pt[2] * f[1];
		let col = c.color.slice();
		if (c.shade) { const k = Math.round((0.55 + 0.45 * Math.min(1, Math.max(0, (c.y - 5) / 8))) * 4) / 4; col = col.map(v => Math.round(v * k)); }
		else { const k = Math.round((0.82 + 0.18 * Math.min(1, pt[1] / 14)) * 4) / 4; col = col.map(v => Math.round(v * k)); }
		px.push({ sx: FEET[0] + wx * S, sy: FEET[1] + wz * S * 0.5 - pt[1] * S, depth: wz * 0.5 + pt[1] * 0.8, col });
	}
	px.sort((a, b) => a.depth - b.depth);
	const img = Buffer.alloc(W * H * 4);
	const put = (x, y, col) => { if (x < 0 || y < 0 || x >= W || y >= H) return; const o = (y * W + x) * 4; img[o] = col[0]; img[o + 1] = col[1]; img[o + 2] = col[2]; img[o + 3] = 255; };
	for (const q of px) {
		const x0 = Math.round(q.sx - S / 2), y0 = Math.round(q.sy - S);
		for (let y = 0; y < S; y++) for (let x = 0; x < S; x++) put(x0 + x, y0 + y, q.col);
	}
	// dark 1-pixel outline
	const out = Buffer.from(img);
	for (let y = 0; y < H; y++) for (let x = 0; x < W; x++) {
		if (img[(y * W + x) * 4 + 3]) continue;
		let near = false;
		for (const [dx, dy] of [[1, 0], [-1, 0], [0, 1], [0, -1]]) { const X = x + dx, Y = y + dy; if (X >= 0 && Y >= 0 && X < W && Y < H && img[(Y * W + X) * 4 + 3]) near = true; }
		if (near) { const o = (y * W + x) * 4; out[o] = 24; out[o + 1] = 16; out[o + 2] = 12; out[o + 3] = 255; }
	}
	return out;
}

// ---------- program ----------
const args = process.argv.slice(2);
const pngArg = args.indexOf('--png') >= 0 ? args[args.indexOf('--png') + 1] : null;
const pngDir = pngArg ? path.resolve(pngArg) : fs.mkdtempSync(path.join(os.tmpdir(), 'soldado-png-'));
fs.mkdirSync(pngDir, { recursive: true });
let n = 0;
for (const anim of Object.keys(ANIMS)) for (let d = 0; d < DIRS; d++) for (let i = 0; i < ANIMS[anim].frames; i++) {
	fs.writeFileSync(path.join(pngDir, `${anim}_d${d}_f${String(i).padStart(2, '0')}.png`), png.encode({ width: W, height: H, data: drawFrame(anim, d, i) }));
	n++;
}
console.log(`${n} PNG dibujados en ${pngDir}`);

const tools = path.join(__dirname, '..', '..', 'tools');
const unitDir = path.join(__dirname, '..', 'unit-graphics-example', 'units', 'RSimpleSoldier');
const node = process.execPath;
cp.execFileSync(node, [path.join(tools, 'convert-sprite.js'), pngDir, '--jugador', '--salida', unitDir], { stdio: 'inherit' });
cp.execFileSync(node, [path.join(tools, 'create-entity.js'), 'unidad', unitDir, '--nombre', 'RSimpleSoldier'], { stdio: 'inherit' });
if (!pngArg) fs.rmSync(pngDir, { recursive: true, force: true });
