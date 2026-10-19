// Crea modelli di città e villaggio propri per una civiltà di una mod (SDK di Imperivm Resurrected).
//
//   node crea-modello.js <cartella del gioco> <cartella della mod> <Prefisso> [--base Gaul] [--larghezza N] [--senza-villaggi]
//
// Copia le città e i villaggi della civiltà base ("Gaul Town 1", "Gaul Village 3"...) con il nome della tua ("Dacian Town 1"...),
// li mette in una zona libera del piano dei modelli e li scrive come patch in
//   <mod>\patches\randommapsettlements\maps\1\map.obj.xml
// Poi aggiunge  "plantilla": "<Prefisso>"  al json della civiltà (civs\<id>.json).
//
// --larghezza N  rende le città più larghe: aggiunge N tratti da 220 unità (una torre e un pezzo di muro per ogni lato lungo).
//            Con --base Gaul, "Gaul Town 1" misura 1540 x 1760; con --larghezza 1 diventa 1760 x 1760 (quadrata). Massimo 4.
//
// --retta   SPERIMENTALE: recinto davvero rettangolare (angoli con una torre, senza gli angoli tagliati del gioco) con --lato N tratti
//            da 220 unità per lato (8 = 1760 x 1760). Le porte (N, S, E) vanno al centro di ogni lato: il gioco ha solo porte
//            disegnate per gli angoli tagliati, quindi bisogna controllare in una partita se si adattano. Gli edifici all'interno si copiano
//            dalla città base.
//
// Con gli stessi pezzi di muro, torri e porte del gioco: cambia solo la disposizione. Non serve toccare il terreno
// (la mappa casuale lo riempie in base al bioma).
const fs = require('fs'), path = require('path');
const { T } = require('./lib/lang');

const args = process.argv.slice(2);
const opt = (name, def) => { const i = args.indexOf(name); if (i < 0) return def; const v = args[i + 1]; args.splice(i, 2); return v; };
const flag = name => { const i = args.indexOf(name); if (i < 0) return false; args.splice(i, 1); return true; };
const baseName = opt('--base', 'Gaul');
const widen = parseInt(opt('--ancho', '0'), 10);
const noVillages = flag('--sin-aldeas');
const straight = flag('--recta');
const sideGaps = parseInt(opt('--lado', '8'), 10);
const [gameDir, modDir, prefix] = args;
if (!gameDir || !modDir || !prefix || !(widen >= 0 && widen <= 4) || !(sideGaps >= 6 && sideGaps <= 9)) {
	console.error(T('uso: node crea-modello.js <carpeta del juego> <carpeta del mod> <Prefijo> [--base Gaul] [--ancho 0..4] [--sin-aldeas] [--recta [--lado 6..9]]',
		'usage: node crea-modello.js <game folder> <mod folder> <Prefix> [--base Gaul] [--ancho 0..4] [--sin-aldeas] [--recta [--lado 6..9]]',
		'uso: node crea-modello.js <cartella del gioco> <cartella della mod> <Prefisso> [--base Gaul] [--ancho 0..4] [--sin-aldeas] [--recta [--lado 6..9]]'));
	process.exit(1);
}
if (!/^[A-Za-z][A-Za-z ]{1,30}$/.test(prefix)) {
	console.error(T('el prefijo son letras y espacios, p. ej. "Dacian"', 'the prefix is letters and spaces, e.g. "Dacian"', 'il prefisso è formato da lettere e spazi, p. es. "Dacian"'));
	process.exit(1);
}

// 1. I modelli del gioco: il file che la mod stessa esporta quando si apre il gioco (vedi più sotto come ottenerlo)
const dumped = path.join(gameDir, "mod-dump", "randommapsettlements", "maps", "1", "map.obj.xml");
if (!fs.existsSync(dumped)) {
	console.error(T(`Falta ${dumped}
Se crea así: en la carpeta del juego, en imperivm-mod.ini, sección [Depuracion], pon  Volcar=randommapsettlements/maps/1/map.obj.xml ,
abre el juego, empieza una escaramuza en un mapa aleatorio, sal, y vuelve a ejecutar esto. (Después puedes quitar la línea.)`,
`Missing ${dumped}
To create it: in the game folder, in imperivm-mod.ini, section [Depuracion], set  Volcar=randommapsettlements/maps/1/map.obj.xml ,
open the game, start a skirmish on a random map, quit, and run this again. (You can remove the line afterwards.)`,
`Manca ${dumped}
Si crea così: nella cartella del gioco, in imperivm-mod.ini, sezione [Depuracion], scrivi  Volcar=randommapsettlements/maps/1/map.obj.xml ,
apri il gioco, avvia una schermaglia su una mappa casuale, esci e riesegui questo comando. (Dopo puoi togliere la riga.)`));
	process.exit(1);
}
const text = fs.readFileSync(dumped, 'latin1').replace(/\r\n/g, '\n');
const settlements = [];  // { name, xml }
for (let i = text.indexOf("<settlement"); i >= 0; i = text.indexOf("<settlement", i + 1)) {
	const end = text.indexOf("</settlement>", i);
	const xml = text.slice(i, end + 13);
	const name = (/name="([^"]*)"/.exec(xml.slice(0, xml.indexOf(">"))) || [])[1];
	if (name) settlements.push({ name, xml });
}
const mine = settlements.filter(s => s.name.startsWith(baseName + ' Town') || (!noVillages && s.name.startsWith(baseName + ' Village')));
if (!mine.length) {
	console.error(T(`no hay plantillas "${baseName} Town/Village" (¿base mal escrita? Gaul, Republican Roman, Carthaginian, Iberian, Imperial Roman, Briton, Egyptian, German)`,
		`no templates "${baseName} Town/Village" found (base misspelled? Gaul, Republican Roman, Carthaginian, Iberian, Imperial Roman, Briton, Egyptian, German)`,
		`nessun modello "${baseName} Town/Village" (base scritta male? Gaul, Republican Roman, Carthaginian, Iberian, Imperial Roman, Briton, Egyptian, German)`));
	process.exit(1);
}

// 2. Oggetti di un modello
const objRe = /<scriptobj\b[\s\S]*?\/>/g;
const parse = o => ({ cls: /class="(\w+)"/.exec(o)[1], x: +/\bx="(-?\d+)"/.exec(o)[1], y: +/\by="(-?\d+)"/.exec(o)[1], xml: o });
const setXY = (o, x, y) => o.replace(/(\bx=")-?\d+(")/, `$1${x}$2`).replace(/(\by=")-?\d+(")/, `$1${y}$2`);

// Allarga: taglia al centro (x = 0), sposta a destra ciò che sta a destra e riempie le righe in alto e in basso
function widenTown(objs, k) {
	const cx = (Math.min(...objs.map(o => o.x)) + Math.max(...objs.map(o => o.x))) / 2;
	const rel = objs.map(o => ({ ...o, x: o.x - cx }));
	const towers = rel.filter(o => /Tower/.test(o.cls));
	const walls = rel.filter(o => /Walls[NS]$/.test(o.cls));
	const topY = Math.min(...towers.map(o => o.y)), botY = Math.max(...towers.map(o => o.y));
	// Si può solo se le righe in alto e in basso hanno torri in x = -110 e 110 (la forma delle città della Gallia)
	const ok = y => [-110, 110].every(x => towers.some(o => Math.abs(o.x - x) <= 2 && Math.abs(o.y - y) <= 2));
	if (!ok(topY) || !ok(botY)) return null;
	const out = rel.map(o => (o.x > 1 ? { ...o, x: o.x + 220 * k } : o));
	const towerXml = towers.find(o => Math.abs(o.x - 110) <= 2).xml;
	for (const y of [topY, botY]) {
		const t = towers.find(o => Math.abs(o.y - y) <= 2 && Math.abs(o.x - 110) <= 2);
		const w = walls.find(o => Math.abs(o.y - (y < 0 ? y + 1 : y - 10)) <= 12 && Math.abs(o.x - 220) <= 2) || walls.find(o => /Walls[NS]$/.test(o.cls) && ((y < 0) === (o.y < 0)));
		for (let i = 0; i < k; i++) {
			out.push({ ...t, x: 110 + 220 * i });
			if (w) out.push({ ...w, x: 220 + 220 * i, y: w.y });
		}
	}
	void towerXml;
	return out.map(o => ({ ...o, x: Math.round(o.x + cx) }));
}

// Recinto rettangolare: torri ogni 220, pezzi di muro tra di esse, porte N, S ed E al centro di ogni lato. Gli edifici all'interno
// (tutto ciò che non è torre, muro o porta) si copiano così come sono. Restituisce null se non trova le classi
function squareTown(objs, gaps) {
	const cx = (Math.min(...objs.map(o => o.x)) + Math.max(...objs.map(o => o.x))) / 2;
	const cy = (Math.min(...objs.map(o => o.y)) + Math.max(...objs.map(o => o.y))) / 2;
	const rel = objs.map(o => ({ ...o, x: o.x - cx, y: o.y - cy }));
	const isRing = o => /Tower|Walls|Gate/.test(o.cls);
	const tower = rel.find(o => /Tower/.test(o.cls));
	const wall = rel.find(o => /Walls[NSEW]$/.test(o.cls));
	const gate = rel.find(o => /Gate/.test(o.cls));
	if (!tower || !wall || !gate) return null;
	const wallPrefix = wall.cls.replace(/[NSEW]+$/, '');
	const gatePrefix = gate.cls.slice(0, gate.cls.indexOf('Gate') + 4);
	const clone = (o, cls, x, y) => ({ ...o, cls, xml: o.xml.replace(/class="[A-Za-z0-9_]+"/, 'class="' + cls + '"'), x: Math.round(x), y: Math.round(y) });
	const half = 110 * gaps;
	const out = rel.filter(o => !isRing(o));
	const mid = gaps / 2;
	for (const side of ['N', 'S', 'E', 'W']) {
		for (let i = 0; i <= gaps; i++) {
			const t = -half + 220 * i;  // posizione lungo il lato
			const corner = (side === 'E' && i === 0) || (side === 'S' && i === gaps) || (side === 'W' && (i === 0 || i === gaps));  // ogni angolo, una sola volta
			const inGate = side !== 'W' && Math.abs(i - mid) < 1.01;  // la porta occupa due spazi: si tolgono la torre centrale e i suoi due muri
			if (!corner && !(inGate && i === mid)) {
				const x = side === 'N' ? t : side === 'S' ? t : side === 'E' ? half : -half;
				const y = side === 'N' ? -half : side === 'S' ? half : t;
				out.push(clone(tower, tower.cls, x, y));
			}
			if (i < gaps) {
				const m = t + 110;  // punto medio dello spazio
				const gateGap = side !== 'W' && (i === mid - 1 || i === mid);
				if (gateGap) continue;
				const x = side === 'N' || side === 'S' ? m : side === 'E' ? half : -half;
				const y = side === 'N' ? -half + 1 : side === 'S' ? half - 10 : m;
				out.push(clone(wall, wallPrefix + side, x, y));
			}
		}
	}
	for (const side of ['N', 'S', 'E']) {
		out.push(clone(gate, gatePrefix + side, side === 'E' ? half : 0, side === 'N' ? -half : side === 'S' ? half : 0));
	}
	return out.map(o => ({ ...o, x: Math.round(o.x + cx), y: Math.round(o.y + cy) }));
}

// 3. Collocare nella zona libera del piano (y 24192-27904: senza oggetti né terreno propri)
let cursorX = 3000;
const renamed = [];
const notes = [];
for (const s of mine) {
	const newName = prefix + s.name.slice(baseName.length);
	let objs = [...s.xml.matchAll(objRe)].map(m => parse(m[0]));
	const isTown = / Town/.test(s.name);
	if (isTown && straight) {
		const q = squareTown(objs, sideGaps);
		if (q) objs = q;
		else notes.push(T(newName + ': no se pudo hacer rectangular (faltan torres, muros o puertas)',
			newName + ': could not make it rectangular (towers, walls or gates missing)',
			newName + ': impossibile renderla rettangolare (mancano torri, mura o porte)'));
	}
	else if (isTown && widen > 0) {
		const w = widenTown(objs, widen);
		if (w) objs = w;
		else notes.push(T(`${newName}: no se puede ensanchar (sus filas de arriba y abajo no tienen la forma de las de Galia); queda igual que ${s.name}`,
			`${newName}: cannot be widened (its top and bottom rows do not have the shape of the Gaul ones); it stays the same as ${s.name}`,
			`${newName}: non si può allargare (le sue file in alto e in basso non hanno la forma di quelle della Gallia); resta uguale a ${s.name}`));
	}
	const minX = Math.min(...objs.map(o => o.x)), maxX = Math.max(...objs.map(o => o.x));
	const minY = Math.min(...objs.map(o => o.y)), maxY = Math.max(...objs.map(o => o.y));
	const w = maxX - minX, h = maxY - minY;
	const dx = Math.round(cursorX + w / 2 - (minX + maxX) / 2), dy = Math.round(26000 - (minY + maxY) / 2);
	const head = s.xml.slice(0, s.xml.indexOf('>') + 1).replace(/name="[^"]*"/, `name="${newName}"`);
	const body = objs.map((o, i) => '\t' + setXY(o.xml, o.x + dx, o.y + dy).replace(/(\bnum=")\d+(")/, `$1${i}$2`).trim()).join('\n');
	renamed.push(`${head}\n${body}\n</settlement>`);
	console.log(T(`${newName}: ${objs.length} objetos, ${w} x ${h} unidades, en x ${cursorX}`,
		`${newName}: ${objs.length} objects, ${w} x ${h} units, at x ${cursorX}`,
		`${newName}: ${objs.length} oggetti, ${w} x ${h} unità, a x ${cursorX}`));
	cursorX += w + 3000;
}
// lo spazio di 3000 dopo l'ultimo non conta: quello che deve starci è il suo bordo destro
if (cursorX - 3000 > 31000) { console.error(T('no caben en la zona libre', 'they do not fit in the free area', 'non c\'è spazio nella zona libera')); process.exit(1); }

const patch = `<patch>
  <!-- ${T(`Plantillas de ${prefix} (generadas por crea-modello.js a partir de ${baseName}). Zona libre del plano: y 24192-27904`,
	`Templates of ${prefix} (generated by crea-modello.js from ${baseName}). Free area of the map: y 24192-27904`,
	`Modelli di ${prefix} (generati da crea-modello.js a partire da ${baseName}). Zona libera della mappa: y 24192-27904`)} -->
  <add select="/mapobject">
${renamed.join('\n')}
  </add>
</patch>
`;
const dest = path.join(modDir, 'patches', 'randommapsettlements', 'maps', '1', 'map.obj.xml');
fs.mkdirSync(path.dirname(dest), { recursive: true });
fs.writeFileSync(dest, patch, 'utf8');
for (const n of notes) console.warn(T('AVISO: ', 'WARNING: ', 'AVVISO: ') + n);
console.log(T(`\nEscrito ${dest}\nAhora pon  "plantilla": "${prefix}"  en el json de tu civilización (civs\\<id>.json) y valida el mod con valida-mod.js.`,
	`\nWritten ${dest}\nNow set  "plantilla": "${prefix}"  in your civilization's json (civs\\<id>.json) and validate the mod with valida-mod.js.`,
	`\nScritto ${dest}\nOra scrivi  "plantilla": "${prefix}"  nel json della tua civiltà (civs\\<id>.json) e convalida la mod con valida-mod.js.`));
