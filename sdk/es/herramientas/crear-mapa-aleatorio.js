// Crea un tipo de mapa aleatorio nuevo para el desplegable "Tipo de mapa" de la sala (SDK de Imperivm Resurrected).
//
//   node crear-mapa-aleatorio.js <carpeta del mod> <Nombre> <dibujo.png> [otro.png ...] [opciones]
//     --jugadores N       máximo de jugadores en mapa grande (8 por defecto); en el mediano, 6 o menos
//     --pequeno N         máximo de jugadores en mapa pequeño (0 = no sale en pequeño; 2 por defecto)
//     --distorsion X      cuánto se deforma la forma al generarla (0.1 casi nada, 0.25 mucho; 0.2 por defecto)
//     --sin-girar         que el mapa no se gire ni se refleje al azar
//
// El dibujo es la forma del mapa vista desde arriba: un PNG (cuadrado; se escala a 512 x 512) pintado con cuatro colores
//     negro   (0,0,0)       mar
//     blanco  (255,255,255) tierra
//     magenta (255,0,255)   montañas: líneas finas de unos 3 píxeles (opcional)
//     rojo    (255,0,0)     puntos donde el juego puede poner sitios de partida y ciudades neutrales. El juego trae cientos de
//                           puntos sueltos por toda la tierra; si tu dibujo no lleva rojo, la herramienta los reparte sola por la tierra
// Los colores que no sean exactos se pasan al más parecido, así que vale con dibujar con cualquier herramienta.
//
// Con varios PNG, el juego elige uno al azar en cada partida (el juego trae entre 1 y 17 por tipo; esta herramienta escribe siempre 12 y repite los tuyos). Con uno solo, el mapa sale siempre con esa forma
// (girada y reflejada, si no pones --sin-girar).
//
// Escribe en la carpeta del mod:
//   data\randommap\<Nombre>.rm.xml     la definición del tipo
//   randommap\<Nombre>\010.bmp ... 120.bmp   los 12 dibujos (si pasas menos, se repiten)
const fs = require('fs'), path = require('path');
const png = require('./lib/png.js');
const { T } = require('./lib/lang');

const args = process.argv.slice(2);
const opt = (n, def) => { const i = args.indexOf(n); if (i < 0) return def; const v = args[i + 1]; args.splice(i, 2); return v; };
const flag = n => { const i = args.indexOf(n); if (i < 0) return false; args.splice(i, 1); return true; };
const maxPlayers = parseInt(opt('--jugadores', '8'), 10);
const maxSmall = parseInt(opt('--pequeno', '2'), 10);
const distortion = parseFloat(opt('--distorsion', '0.2'));
const noRotate = flag('--sin-girar');
const [modDir, name, ...images] = args;
if (!modDir || !name || !images.length) {
	console.error(T('uso: node crear-mapa-aleatorio.js <carpeta del mod> <Nombre> <dibujo.png> [otro.png ...] [--jugadores 8] [--pequeno 2] [--distorsion 0.2] [--sin-girar]',
		'usage: node crear-mapa-aleatorio.js <mod folder> <Name> <drawing.png> [other.png ...] [--jugadores 8] [--pequeno 2] [--distorsion 0.2] [--sin-girar]',
		'uso: node crear-mapa-aleatorio.js <cartella della mod> <Nome> <disegno.png> [altro.png ...] [--jugadores 8] [--pequeno 2] [--distorsion 0.2] [--sin-girar]'));
	process.exit(1);
}
const GAME_TYPES = ['coastal', 'continent', 'empty', 'island', 'large islands', 'mediterranean', 'mountain passes', 'randommap', 'small islands', 'tournament'];
if (GAME_TYPES.includes(name.toLowerCase())) {
	console.error(T('"' + name + '" es el nombre de un tipo de mapa del juego: pisarías el suyo. Usa otro nombre.',
		'"' + name + '" is the name of a game map type: you would overwrite it. Use another name.',
		'"' + name + '" è il nome di un tipo di mappa del gioco: sovrascriveresti quello. Usa un altro nome.'));
	process.exit(1);
}
if (!/^[A-Za-z][A-Za-z0-9 ]{1,30}$/.test(name)) {
	console.error(T('el nombre son letras, números y espacios (empieza por letra), p. ej. "Archipielago"',
		'the name is letters, digits and spaces (must start with a letter), e.g. "Archipielago"',
		'il nome è formato da lettere, numeri e spazi (inizia con una lettera), p. es. "Archipielago"'));
	process.exit(1);
}
if (!(maxPlayers >= 2 && maxPlayers <= 8) || !(maxSmall >= 0 && maxSmall <= 8) || !(distortion >= 0 && distortion <= 0.5)) {
	console.error(T('--jugadores va de 2 a 8, --pequeno de 0 a 8 y --distorsion de 0 a 0.5',
		'--jugadores goes from 2 to 8, --pequeno from 0 to 8 and --distorsion from 0 to 0.5',
		'--jugadores va da 2 a 8, --pequeno da 0 a 8 e --distorsion da 0 a 0.5'));
	process.exit(1);
}

const KEY = [[0, 0, 0, 'mar'], [255, 255, 255, 'tierra'], [255, 0, 0, 'sitio de partida'], [255, 0, 255, 'montañas']];
const SIZE = 512;

// PNG -> 512 x 512 de cuatro colores, y recuento de manchas rojas (4 vecinos)
function prepare(file) {
	const img = png.decode(fs.readFileSync(file));
	const cls = new Uint8Array(SIZE * SIZE);
	for (let y = 0; y < SIZE; y++) {
		for (let x = 0; x < SIZE; x++) {
			const sx = Math.min(img.width - 1, Math.floor(x * img.width / SIZE)), sy = Math.min(img.height - 1, Math.floor(y * img.height / SIZE));
			const i = (sy * img.width + sx) * 4;
			const a = img.data[i + 3] / 255;
			const r = img.data[i] * a + 255 * (1 - a) * 0, g = img.data[i + 1] * a, b = img.data[i + 2] * a;  // lo transparente es mar
			let best = 0, bd = 1e9;
			KEY.forEach((k, n) => { const d = (r - k[0]) ** 2 + (g - k[1]) ** 2 + (b - k[2]) ** 2; if (d < bd) { bd = d; best = n; } });
			cls[y * SIZE + x] = best;
		}
	}
	// manchas rojas
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

// BMP de 16 bits (RGB555, sin compresión, de abajo arriba), como los del juego
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

// Si el dibujo no trae rojo, se reparten puntos por la tierra: una cuadrícula de 12 píxeles con un punto al azar (fijo para cada nombre) en el 60 % de
// las casillas, a más de 6 píxeles del mar, como hacen los dibujos del juego
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
	console.warn(T(`AVISO: 12 dibujos no se reparten por igual entre ${images.length} PNG: algunos saldrán más veces que otros (con 12, 6, 4, 3, 2 o 1 PNG el reparto es igual)`,
		`WARNING: 12 drawings cannot be split evenly among ${images.length} PNG: some will appear more often than others (with 12, 6, 4, 3, 2 or 1 PNG the split is even)`,
		`AVVISO: 12 disegni non si dividono in parti uguali tra ${images.length} PNG: alcuni usciranno più spesso di altri (con 12, 6, 4, 3, 2 o 1 PNG la divisione è uguale)`));
}
const prepared = images.map(f => { const p = prepare(f); if (p.blobs === 0) scatter(p); return { file: f, ...p }; });
for (const p of prepared) {
	const pctLand = (100 * p.count[1] / SIZE / SIZE).toFixed(0), pctSea = (100 * p.count[0] / SIZE / SIZE).toFixed(0);
	console.log(T(`${path.basename(p.file)}: ${p.blobs} puntos de sitios${p.auto ? " (repartidos por la herramienta)" : ""}, ${pctLand} % tierra, ${pctSea} % mar${p.count[3] ? ', con montañas' : ''}`,
		`${path.basename(p.file)}: ${p.blobs} start points${p.auto ? " (placed by the tool)" : ""}, ${pctLand} % land, ${pctSea} % sea${p.count[3] ? ', with mountains' : ''}`,
		`${path.basename(p.file)}: ${p.blobs} punti di partenza${p.auto ? " (distribuiti dallo strumento)" : ""}, ${pctLand} % terra, ${pctSea} % mare${p.count[3] ? ', con montagne' : ''}`));
	if (p.count[1] === 0) {
		console.warn(T(`AVISO: ${path.basename(p.file)} no tiene nada de tierra (blanco)`,
			`WARNING: ${path.basename(p.file)} has no land at all (white)`,
			`AVVISO: ${path.basename(p.file)} non ha nessuna terra (bianco)`));
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
console.log(T(`\nEscrito ${path.join(xmlDir, name + '.rm.xml')} y 12 dibujos en ${outDir}`,
	`\nWritten ${path.join(xmlDir, name + '.rm.xml')} and 12 drawings in ${outDir}`,
	`\nScritto ${path.join(xmlDir, name + '.rm.xml')} e 12 disegni in ${outDir}`));
console.log(T('Actívalo con el mod y mira el desplegable "Tipo de mapa" de la sala: saldrá "Aleatorio: ' + name + '".',
	'Enable it with the mod and check the "Map type" dropdown in the lobby: it will show "Random: ' + name + '".',
	'Attivala con la mod e guarda il menu a tendina "Tipo di mappa" della sala: comparirà "Casuale: ' + name + '".'));
