// Creates city and village templates of your own for a mod civilization (Imperivm Resurrected SDK).
//
//   node create-template.js <game folder> <mod folder> <Prefix> [--base Gaul] [--width N] [--no-villages]
//
// Copies the cities and villages of the base civilization ("Gaul Town 1", "Gaul Village 3"...) under the name of yours ("Dacian Town 1"...),
// places them in a free area of the template map and writes them as a patch in
//   <mod>\patches\randommapsettlements\maps\1\map.obj.xml
// Then adds  "plantilla": "<Prefix>"  to the civilization's json (civs\<id>.json).
//
// --width N  makes the cities wider: adds N sections of 220 units (a tower and a piece of wall on each long side).
//            With --base Gaul, "Gaul Town 1" measures 1540 x 1760; with --width 1 it comes out 1760 x 1760 (square). Maximum 4.
//
// --straight   EXPERIMENTAL: a truly rectangular enclosure (corners with a tower, without the game's cut corners) with --side N sections
//            of 220 units per side (8 = 1760 x 1760). The gates (N, S, E) go in the center of each side: the game only ships gates
//            drawn for the cut corners, so you have to check in a game whether they fit. The buildings inside are copied
//            from the base city.
//
// With the same wall, tower and gate pieces as the game: only the layout changes. There is no need to touch the terrain
// (the random map fills it in according to the biome).
const fs = require('fs'), path = require('path');

const args = process.argv.slice(2);
const opt = (name, def) => { const i = args.indexOf(name); if (i < 0) return def; const v = args[i + 1]; args.splice(i, 2); return v; };
const flag = name => { const i = args.indexOf(name); if (i < 0) return false; args.splice(i, 1); return true; };
const baseName = opt('--base', 'Gaul');
const widen = parseInt(opt('--width', '0'), 10);
const noVillages = flag('--no-villages');
const straight = flag('--straight');
const sideGaps = parseInt(opt('--side', '8'), 10);
const [gameDir, modDir, prefix] = args;
if (!gameDir || !modDir || !prefix || !(widen >= 0 && widen <= 4) || !(sideGaps >= 6 && sideGaps <= 9)) {
	console.error('usage: node create-template.js <game folder> <mod folder> <Prefix> [--base Gaul] [--width 0..4] [--no-villages] [--straight [--side 6..9]]');
	process.exit(1);
}
if (!/^[A-Za-z][A-Za-z ]{1,30}$/.test(prefix)) {
	console.error('the prefix is letters and spaces, e.g. "Dacian"');
	process.exit(1);
}

// 1. The game's templates: the file that the mod itself dumps when the game opens it (see below for how to get it)
const dumped = path.join(gameDir, "mod-dump", "randommapsettlements", "maps", "1", "map.obj.xml");
if (!fs.existsSync(dumped)) {
	console.error(`Missing ${dumped}
To create it: in the game folder, in imperivm-mod.ini, section [Debug], set  Dump=randommapsettlements/maps/1/map.obj.xml ,
open the game, start a skirmish on a random map, quit, and run this again. (You can remove the line afterwards.)`);
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
	console.error(`no templates "${baseName} Town/Village" found (base misspelled? Gaul, Republican Roman, Carthaginian, Iberian, Imperial Roman, Briton, Egyptian, German)`);
	process.exit(1);
}

// 2. Objects of a template
const objRe = /<scriptobj\b[\s\S]*?\/>/g;
const parse = o => ({ cls: /class="(\w+)"/.exec(o)[1], x: +/\bx="(-?\d+)"/.exec(o)[1], y: +/\by="(-?\d+)"/.exec(o)[1], xml: o });
const setXY = (o, x, y) => o.replace(/(\bx=")-?\d+(")/, `$1${x}$2`).replace(/(\by=")-?\d+(")/, `$1${y}$2`);

// Widens: cuts through the center (x = 0), shifts what is on the right further right and fills in the top and bottom rows
function widenTown(objs, k) {
	const cx = (Math.min(...objs.map(o => o.x)) + Math.max(...objs.map(o => o.x))) / 2;
	const rel = objs.map(o => ({ ...o, x: o.x - cx }));
	const towers = rel.filter(o => /Tower/.test(o.cls));
	const walls = rel.filter(o => /Walls[NS]$/.test(o.cls));
	const topY = Math.min(...towers.map(o => o.y)), buttonY = Math.max(...towers.map(o => o.y));
	// Only possible if the top and bottom rows have towers at x = -110 and 110 (the shape of the Gaul cities)
	const ok = y => [-110, 110].every(x => towers.some(o => Math.abs(o.x - x) <= 2 && Math.abs(o.y - y) <= 2));
	if (!ok(topY) || !ok(buttonY)) return null;
	const out = rel.map(o => (o.x > 1 ? { ...o, x: o.x + 220 * k } : o));
	const towerXml = towers.find(o => Math.abs(o.x - 110) <= 2).xml;
	for (const y of [topY, buttonY]) {
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

// Rectangular enclosure: towers every 220, wall pieces between them, N, S and E gates at the center of each side. The buildings inside
// (everything that is not a tower, wall or gate) are copied as they are. Returns null if it does not find the classes
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
			const t = -half + 220 * i;  // position along the side
			const corner = (side === 'E' && i === 0) || (side === 'S' && i === gaps) || (side === 'W' && (i === 0 || i === gaps));  // each corner, only once
			const inGate = side !== 'W' && Math.abs(i - mid) < 1.01;  // the gate takes up two gaps: the central tower and its two walls are removed
			if (!corner && !(inGate && i === mid)) {
				const x = side === 'N' ? t : side === 'S' ? t : side === 'E' ? half : -half;
				const y = side === 'N' ? -half : side === 'S' ? half : t;
				out.push(clone(tower, tower.cls, x, y));
			}
			if (i < gaps) {
				const m = t + 110;  // midpoint of the gap
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

// 3. Place in the free area of the template map (y 24192-27904: no objects or terrain of its own)
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
		else notes.push(newName + ': could not make it rectangular (towers, walls or gates missing)');
	}
	else if (isTown && widen > 0) {
		const w = widenTown(objs, widen);
		if (w) objs = w;
		else notes.push(`${newName}: cannot be widened (its top and bottom rows do not have the shape of the Gaul ones); it stays the same as ${s.name}`);
	}
	const minX = Math.min(...objs.map(o => o.x)), maxX = Math.max(...objs.map(o => o.x));
	const minY = Math.min(...objs.map(o => o.y)), maxY = Math.max(...objs.map(o => o.y));
	const w = maxX - minX, h = maxY - minY;
	const dx = Math.round(cursorX + w / 2 - (minX + maxX) / 2), dy = Math.round(26000 - (minY + maxY) / 2);
	const head = s.xml.slice(0, s.xml.indexOf('>') + 1).replace(/name="[^"]*"/, `name="${newName}"`);
	const body = objs.map((o, i) => '\t' + setXY(o.xml, o.x + dx, o.y + dy).replace(/(\bnum=")\d+(")/, `$1${i}$2`).trim()).join('\n');
	renamed.push(`${head}\n${body}\n</settlement>`);
	console.log(`${newName}: ${objs.length} objects, ${w} x ${h} units, at x ${cursorX}`);
	cursorX += w + 3000;
}
// the 3000 gap after the last one doesn't count: what has to fit is its right edge
if (cursorX - 3000 > 31000) { console.error('they do not fit in the free area'); process.exit(1); }

const patch = `<patch>
  <!-- ${`Templates of ${prefix} (generated by create-template.js from ${baseName}). Free area of the map: y 24192-27904`} -->
  <add select="/mapobject">
${renamed.join('\n')}
  </add>
</patch>
`;
const dest = path.join(modDir, 'patches', 'randommapsettlements', 'maps', '1', 'map.obj.xml');
fs.mkdirSync(path.dirname(dest), { recursive: true });
fs.writeFileSync(dest, patch, 'utf8');
for (const n of notes) console.warn('WARNING: ' + n);
console.log(`\nWritten ${dest}\nNow set  "plantilla": "${prefix}"  in your civilization's json (civs\\<id>.json) and validate the mod with validate-mod.js.`);
