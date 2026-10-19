// Creates a new random map type for the "Map type" dropdown in the lobby (Imperivm Resurrected SDK).
//
//   node create-random-map.js <mod folder> <Name> <drawing.png> [other.png ...] [options]
//     --players N       maximum players on a large map (8 by default); on a medium one, 6 or fewer
//     --small N         maximum players on a small map (0 = it does not appear on small maps; 2 by default)
//     --distortion X      how much the shape is deformed when it is generated (0.1 almost none, 0.25 a lot; 0.2 by default)
//     --no-rotate         so the map is not rotated or mirrored at random
//
// The drawing is the shape of the map seen from above: a PNG (square; scaled to 512 x 512) painted with four colors
//     black   (0,0,0)       sea
//     white   (255,255,255) land
//     magenta (255,0,255)   mountains: thin lines of about 3 pixels (optional)
//     red     (255,0,0)     points where the game can place starting positions and neutral cities. The game's own drawings have hundreds of
//                           scattered points all over the land; if your drawing has no red, the tool spreads them over the land by itself
// Colors that are not exact are snapped to the closest one, so you can draw with any tool.
//
// With several PNGs, the game picks one at random in each game (the game ships between 1 and 17 per type; this tool always writes 12 and repeats yours). With just one, the map always comes out with that shape
// (rotated and mirrored, unless you pass --no-rotate).
//
// Writes to the mod folder:
//   data\randommap\<Name>.rm.xml     the type definition
//   randommap\<Name>\010.bmp ... 120.bmp   the 12 drawings (if you pass fewer, they are repeated)
const fs = require('fs'), path = require('path');
const png = require('./lib/png.js');

const args = process.argv.slice(2);
const opt = (n, def) => { const i = args.indexOf(n); if (i < 0) return def; const v = args[i + 1]; args.splice(i, 2); return v; };
const flag = n => { const i = args.indexOf(n); if (i < 0) return false; args.splice(i, 1); return true; };
const maxPlayers = parseInt(opt('--players', '8'), 10);
const maxSmall = parseInt(opt('--small', '2'), 10);
const distortion = parseFloat(opt('--distortion', '0.2'));
const noRotate = flag('--no-rotate');
const [modDir, name, ...images] = args;
if (!modDir || !name || !images.length) {
	console.error('usage: node create-random-map.js <mod folder> <Name> <drawing.png> [other.png ...] [--players 8] [--small 2] [--distortion 0.2] [--no-rotate]');
	process.exit(1);
}
const GAME_TYPES = ['coastal', 'continent', 'empty', 'island', 'large islands', 'mediterranean', 'mountain passes', 'randommap', 'small islands', 'tournament'];
if (GAME_TYPES.includes(name.toLowerCase())) {
	console.error('"' + name + '" is the name of a game map type: you would overwrite it. Use another name.');
	process.exit(1);
}
if (!/^[A-Za-z][A-Za-z0-9 ]{1,30}$/.test(name)) {
	console.error('the name is letters, digits and spaces (must start with a letter), e.g. "Archipielago"');
	process.exit(1);
}
if (!(maxPlayers >= 2 && maxPlayers <= 8) || !(maxSmall >= 0 && maxSmall <= 8) || !(distortion >= 0 && distortion <= 0.5)) {
	console.error('--players goes from 2 to 8, --small from 0 to 8 and --distortion from 0 to 0.5');
	process.exit(1);
}

const KEY = [[0, 0, 0, 'mar'], [255, 255, 255, 'tierra'], [255, 0, 0, 'sitio de partida'], [255, 0, 255, 'montañas']];
const SIZE = 512;

// PNG -> 512 x 512 with four colors, and count of red blobs (4 neighbors)
function prepare(file) {
	const img = png.decode(fs.readFileSync(file));
	const cls = new Uint8Array(SIZE * SIZE);
	for (let y = 0; y < SIZE; y++) {
		for (let x = 0; x < SIZE; x++) {
			const sx = Math.min(img.width - 1, Math.floor(x * img.width / SIZE)), sy = Math.min(img.height - 1, Math.floor(y * img.height / SIZE));
			const i = (sy * img.width + sx) * 4;
			const a = img.data[i + 3] / 255;
			const r = img.data[i] * a + 255 * (1 - a) * 0, g = img.data[i + 1] * a, b = img.data[i + 2] * a;  // transparent is sea
			let best = 0, bd = 1e9;
			KEY.forEach((k, n) => { const d = (r - k[0]) ** 2 + (g - k[1]) ** 2 + (b - k[2]) ** 2; if (d < bd) { bd = d; best = n; } });
			cls[y * SIZE + x] = best;
		}
	}
	// red blobs
	const seen = new Uint8Array(SIZE * SIZE);
	let blobs = 0;
	for (let s = 0; s < SIZE * SIZE; s++) {
		if (cls[s] !== 2 || seen[s]) continue;
		blobs++;
		const stack = [s]; seen[s] = 1;
		while (stack.length) {
			const p = stack.pop(), px = p % SIZE, py = (p / SIZE) | 0;
			for (const [dx, dy] of [[1, 0], [-1, 0], [0, 1], [0, -1]]) {
				const nx = px + dx, ny = py + dy;
				if (nx < 0 || ny < 0 || nx >= SIZE || ny >= SIZE) continue;
				const q = ny * SIZE + nx;
				if (cls[q] === 2 && !seen[q]) { seen[q] = 1; stack.push(q); }
			}
		}
	}
	const count = [0, 0, 0, 0];
	cls.forEach(c => count[c]++);
	return { cls, blobs, count };
}

// 16-bit BMP (RGB555, uncompressed, bottom-up), like the game's
function bmp16(cls) {
	const rowBytes = SIZE * 2, size = 54 + rowBytes * SIZE;
	const b = Buffer.alloc(size);
	b.write('BM', 0); b.writeUInt32LE(size, 2); b.writeUInt32LE(54, 10);
	b.writeUInt32LE(40, 14); b.writeInt32LE(SIZE, 18); b.writeInt32LE(SIZE, 22); b.writeUInt16LE(1, 26); b.writeUInt16LE(16, 28);
	b.writeUInt32LE(0, 30); b.writeUInt32LE(rowBytes * SIZE, 34); b.writeInt32LE(2835, 38); b.writeInt32LE(2835, 42);
	const v555 = KEY.map(k => ((k[0] >> 3) << 10) | ((k[1] >> 3) << 5) | (k[2] >> 3));
	for (let y = 0; y < SIZE; y++) {
		for (let x = 0; x < SIZE; x++) b.writeUInt16LE(v555[cls[(SIZE - 1 - y) * SIZE + x]], 54 + y * rowBytes + x * 2);
	}
	return b;
}

// If the drawing has no red, points are spread over the land: a 12-pixel grid with a random point (fixed for each name) in 60 % of
// the cells, more than 6 pixels away from the sea, as the game's drawings do
function scatter(p) {
	let seed = 0;
	for (const c of name) seed = (seed * 31 + c.charCodeAt(0)) >>> 0;
	const rnd = () => { seed = (seed + 0x6d2b79f5) >>> 0; let t = seed; t = Math.imul(t ^ (t >>> 15), t | 1); t ^= t + Math.imul(t ^ (t >>> 7), t | 61); return ((t ^ (t >>> 14)) >>> 0) / 4294967296; };
	const land = (x, y) => x >= 0 && y >= 0 && x < SIZE && y < SIZE && p.cls[y * SIZE + x] === 1;
	let n = 0;
	for (let cy = 0; cy < SIZE; cy += 12) {
		for (let cx = 0; cx < SIZE; cx += 12) {
			if (rnd() > 0.6) continue;
			const x = cx + Math.floor(rnd() * 10) + 1, y = cy + Math.floor(rnd() * 10) + 1;
			let ok = true;
			for (let dy = -6; dy <= 6 && ok; dy += 3) for (let dx = -6; dx <= 6 && ok; dx += 3) if (!land(x + dx, y + dy)) ok = false;
			if (!ok) continue;
			for (const [dx, dy] of [[0, 0], [1, 0], [0, 1], [1, 1]]) p.cls[(y + dy) * SIZE + x + dx] = 2;
			n++;
		}
	}
	p.blobs = n; p.auto = true;
	p.count = [0, 0, 0, 0]; p.cls.forEach(c => p.count[c]++);
}
if (images.length > 1 && 12 % images.length !== 0) {
	console.warn(`WARNING: 12 drawings cannot be split evenly among ${images.length} PNG: some will appear more often than others (with 12, 6, 4, 3, 2 or 1 PNG the split is even)`);
}
const prepared = images.map(f => { const p = prepare(f); if (p.blobs === 0) scatter(p); return { file: f, ...p }; });
for (const p of prepared) {
	const pctLand = (100 * p.count[1] / SIZE / SIZE).toFixed(0), pctSea = (100 * p.count[0] / SIZE / SIZE).toFixed(0);
	console.log(`${path.basename(p.file)}: ${p.blobs} start points${p.auto ? " (placed by the tool)" : ""}, ${pctLand} % land, ${pctSea} % sea${p.count[3] ? ', with mountains' : ''}`);
	if (p.count[1] === 0) {
		console.warn(`WARNING: ${path.basename(p.file)} has no land at all (white)`);
	}
}

const outDir = path.join(modDir, 'randommap', name);
fs.mkdirSync(outDir, { recursive: true });
for (let i = 1; i <= 12; i++) {
	fs.writeFileSync(path.join(outDir, String(i * 10).padStart(3, '0') + '.bmp'), bmp16(prepared[(i - 1) % prepared.length].cls));
}
const xmlDir = path.join(modDir, 'data', 'randommap');
fs.mkdirSync(xmlDir, { recursive: true });
const medium = Math.min(6, maxPlayers);
const xml = `<?xml version="1.0"?>

<map-template name="${name}" >

	<bitmap file="RandomMap/${name}/"/>

	<transform allow-rotate="${noRotate ? 0 : 1}" allow-mirror="${noRotate ? 0 : 1}" />

	<distortion amp="${distortion}" first-oct="2" number-octs="4" persist="0.5"/>

	<color-code red="0"   green="0"   blue="0"   meaning="sea"/>
	<color-code red="255" green="255" blue="255" meaning="land"/>
	<color-code red="255" green="0"   blue="0"   meaning="player-stronghold"/>
	<color-code red="255"   green="0" blue="255"   meaning="mountain"/>

	<limit map-size="small" max-players="${maxSmall}" no_ruins="1"/>
	<limit map-size="medium" max-players="${medium}" no_ruins="1"/>
	<limit map-size="large" max-players="${maxPlayers}" no_ruins="1"/>

</map-template>
`;
fs.writeFileSync(path.join(xmlDir, name + '.rm.xml'), xml, 'utf8');
console.log(`\nWritten ${path.join(xmlDir, name + '.rm.xml')} and 12 drawings in ${outDir}`);
console.log('Enable it with the mod and check the "Map type" dropdown in the lobby: it will show "Random: ' + name + '".');
