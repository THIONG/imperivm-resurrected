// Clones a game civilization as a new civilization in a mod (Imperivm Resurrected SDK).
//
//   node clone-civilization.js <DATA folder of the extracted data.pak> <mod folder> <id> <letter> [base=Gaul] [index=10]
//
// Writes to the mod folder:
//   civs\<id>.json                     the civilization declaration
//   data\classes\<L>*.sc.xml           a copy of each base class, with a different id and race="<id>"
//   data\commands\<id>.xml             the base orders that name its classes, with the new classes
//   patches\data\commands\*.xml        adds the new classes to the shared orders (repair, upgrades...)
const fs = require('fs'), path = require('path');

const [dataDir, modDir, id, letter, base = 'Gaul', indexArg = '10'] = process.argv.slice(2);
if (!dataDir || !modDir || !id || !letter) {
	console.error('usage: node clone-civilization.js <DATA> <mod folder> <id> <letter> [base=Gaul] [index=10]');
	process.exit(1);
}
if (!/^[A-Za-z][A-Za-z0-9_]{0,23}$/.test(id) || !/^[A-Za-z]$/.test(letter)) {
	console.error('invalid id or letter'); process.exit(1);
}
const L = letter.toUpperCase(), l = letter.toLowerCase();
const Id = id[0].toUpperCase() + id.slice(1);

// Letter of each game civilization and the name of its generic hero
const BASES = {
	Gaul: { letter: 'G', hero: 'GaulHero' }, RepublicanRome: { letter: 'R', hero: 'RomeHero' },
	Carthage: { letter: 'C', hero: 'CarthageHero' }, Iberia: { letter: 'I', hero: 'IberiaHero' },
	ImperialRome: { letter: 'M', hero: 'ImperialHero' }, Britain: { letter: 'B', hero: 'BritainHero' },
	Egypt: { letter: 'E', hero: 'EgyptHero' }, Germany: { letter: 'T', hero: 'GermanHero' },
};
if (!BASES[base]) { console.error('unknown base: ' + base); process.exit(1); }
const bl = BASES[base].letter, blow = bl.toLowerCase();

const read = f => fs.readFileSync(f, 'latin1');
const ci = (dir, name) => fs.readdirSync(dir).find(f => f.toLowerCase() === name.toLowerCase());
const classesDir = path.join(dataDir, ci(dataDir, 'classes'));
const commandsDir = path.join(dataDir, ci(dataDir, 'commands'));

// 1. Base classes: id -> file text. It is copied whole: the game does NOT inherit things like the entity from the parent,
// and a class without an entity is created with no model and disappears
const classes = new Map();
const everyClass = new Map();  // every game class, by id (for those that have no race but that the civilization needs)
for (const f of fs.readdirSync(classesDir)) {
	if (!/\.sc\.xml$/i.test(f)) continue;
	const t = read(path.join(classesDir, f));
	const m = t.match(/<class\b([^>]*)>/);
	if (!m) continue;
	const cid = (m[1].match(/\bid\s*=\s*"([^"]*)"/) || [])[1];
	if (!cid) continue;
	everyClass.set(cid, t);
	if (t.indexOf('race="' + base + '"') >= 0) classes.set(cid, t);
}
// Sentries: the AI, towers, walls and guard posts create "<letter>Sentry", "<letter>Sentry1" and "<letter>GuardPostSentry" with the
// OWNER's letter. They are classes without race (children of <letter>Sentry), so they do not come in through the race: they are requested by name. The first one is mandatory
for (const extra of ['Sentry', 'Sentry1', 'GuardPostSentry']) {
	const id = bl + extra;
	if (everyClass.has(id) && !classes.has(id)) classes.set(id, everyClass.get(id));
}
// The child classes of the base ones that have no race (GHero1..GHero4, children of GaulHero: they inherit the race from the parent) also belong to the
// civilization. Without them there are no heroes to hire (GetRandomHeroClass keeps looking forever for a class with a model)
const parentOf = t => ((t.match(/<class\b[^>]*?\bparent\s*=\s*"([^"]*)"/) || [])[1]);
for (let grew = true; grew;) {
	grew = false;
	for (const [cid, t] of everyClass) {
		if (classes.has(cid) || /<properties\b[^>]*\brace\s*=/.test(t)) continue;
		const p = parentOf(t);
		if (p && classes.has(p)) { classes.set(cid, t); grew = true; }
	}
}
// Which classes get cloned: those starting with the base letter + an uppercase letter (or "house", like Rhouse03 of Republican Rome) and the generic hero
const rename = new Map();
for (const cid of classes.keys()) {
	if (new RegExp('^' + bl + '([A-Z]|house)').test(cid)) rename.set(cid, L + cid.slice(1));
	else if (cid === BASES[base].hero) rename.set(cid, Id + 'Hero');
}
console.log(`${classes.size} classes with race="${base}", ${rename.size} will be cloned`);

// Many orders do not name the class but its alias (altid): <src obj="DruidHouse"/> is the Gaul druid house (GDruidHouse, altid
// "DruidHouse"). Since the copies carry no altid, the orders for those aliases are offered to the copy by its class id
const aliasOf = new Map();  // altid -> id of the original class
for (const [cid, text] of classes) {
	const alias = (text.match(/<class\b[^>]*?\baltid\s*=\s*"([^"]*)"/) || [])[1];
	if (alias && rename.has(cid)) aliasOf.set(alias, cid);
}

// 2. New classes: copies of the originals with a different id and a different civilization. The image and entity paths do not change
fs.mkdirSync(path.join(modDir, 'data', 'classes'), { recursive: true });
for (const [oldId, newId] of rename) {
	let xml = classes.get(oldId);
	xml = xml.replace(/(<class\b[^>]*?\bid\s*=\s*")[^"]*"/, '$1' + newId + '"');
	xml = xml.replace(/(<class\b[^>]*?\bparent\s*=\s*")([^"]*)"/, (s, p, par) => p + (rename.get(par) || par) + '"');
	xml = xml.split('race="' + base + '"').join('race="' + Id + '"');
	// The altid is a unique alias ("Barrack", "Village"...) that scripts and the AI use to find a civilization's "barracks": the game
	// registers it for a single class. If the copy repeats it, either the copy's AI or the original's stops seeing its buildings
	xml = xml.replace(/(<class\b[^>]*?)\s+altid\s*=\s*"[^"]*"/, '$1');
	// Walls, gates and towers place their sentries with sentry_class_name="GSentry1": in the copy, those of the copy
	xml = xml.replace(/(\bsentry_class_name\s*=\s*")([^"]*)"/g, (s, a, v) => a + (rename.get(v) || v) + '"');
	fs.writeFileSync(path.join(modDir, 'data', 'classes', newId + '.sc.xml'), xml, 'latin1');
}

// 3. Orders
const esc = s => s.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');
const alt = [...rename.keys()].sort((a, b) => b.length - a.length).map(esc).join('|');
const classRe = new RegExp('\\b(' + alt + ')\\b', 'g');
const mapIds = s => s.replace(classRe, m => rename.get(m));
function mapName(n) {
	const r = mapIds(n);
	if (r !== n) return r;
	return n.replace(new RegExp('^(trainpeasant)' + blow + '$'), '$1' + l)
		.replace(new RegExp('^(hirehero)' + bl + '$'), '$1' + L)
		.replace(new RegExp('^' + bl + '(Tribute|AddPop)$'), L + '$1')
		.replace(new RegExp('\\b' + base + '\\b'), Id);
}
const cloned = [];
const aliasOrders = {};  // name of the base order -> name of the cloned order (for the AI)
const shared = new Map();  // file -> [{name, srcs}]
let nShared = 0;
for (const f of fs.readdirSync(commandsDir)) {
	const t = read(path.join(commandsDir, f));
	const re = /<cmd\b[\s\S]*?<\/cmd>/g;
	let m;
	while ((m = re.exec(t))) {
		const body = m[0];
		const name = (body.match(/\bname\s*=\s*"([^"]*)"/) || [])[1];
		const srcs = [...body.matchAll(/<src\s+obj\s*=\s*"([^"]*)"/g)].map(x => aliasOf.get(x[1]) || x[1]);
		const mine = srcs.filter(s => rename.has(s));
		// Exclusions (<nsrc>): an order for "all posts except GGuardPost" would offer its buttons to the DGuardPost if it is not excluded
			const nsrcs = [...body.matchAll(/<nsrc\s+obj\s*=\s*"([^"]*)"/g)].map(x => aliasOf.get(x[1]) || x[1]).filter(s => rename.has(s));
			if (nsrcs.length && name) {
				if (!shared.has(f)) shared.set(f, []);
				shared.get(f).push({ name, srcs: [], nsrcs: nsrcs.map(s => rename.get(s)) });
			}
			if (!mine.length || !name) continue;
		const head = body.replace(/<src\b[\s\S]*$/, '');
		const refs = new RegExp(classRe.source).test(head);
		if (refs || mapName(name) !== name || /^trainpeasant|^hirehero/i.test(name)) {
			// the order names base classes: copy it with the new classes
			let nn = mapName(name);
			if (nn === name) nn = name + ' ' + Id;
			let c = body.replace(/<src\b[^>]*\/>\s*/g, '');
			c = c.replace(/(\bname\s*=\s*")[^"]*"/, '$1' + nn + '"');
			const srcTxt = mine.map(s => '\t<src obj="' + rename.get(s) + '"/>').join('\n');
			// Only the param and sclass classes are changed: the images (button, queueicon...), the help and the texts are the base's
			c = c.replace(/(\b(?:param|sclass)\s*=\s*")([^"]*)"/g, (s, a, v) => a + mapIds(v) + '"');
			c = c.replace(/<\/cmd>$/, srcTxt + '\n</cmd>');
			cloned.push(c);
			aliasOrders[name] = nn;
		}
		else {
			// generic order (repair, upgrades...): the original is patched so that the new classes offer it too
			if (!shared.has(f)) shared.set(f, []);
			shared.get(f).push({ name, srcs: mine.map(s => rename.get(s)) });
			nShared++;
		}
	}
}
fs.mkdirSync(path.join(modDir, 'data', 'commands'), { recursive: true });
fs.writeFileSync(path.join(modDir, 'data', 'commands', Id + '.xml'),
	'<commands>\n\n<!-- ' + `${Id} (cloned from ${base})` + ' -->\n\n' + cloned.join('\n\n') + '\n\n</commands>\n', 'latin1');
for (const [f, list] of shared) {
	const dest = path.join(modDir, 'patches', 'data', 'commands', f);
	fs.mkdirSync(path.dirname(dest), { recursive: true });
	const ops = list.map(o => '\t<add select="/commands/cmd[@name=\'' + o.name + '\']">' +
		o.srcs.map(s => '<src obj="' + s + '"/>').join('') + (o.nsrcs || []).map(s => '<nsrc obj="' + s + '"/>').join('') + '</add>');
	fs.writeFileSync(dest, '<patch>\n' + ops.join('\n') + '\n</patch>\n', 'latin1');
}

// 4. Declaration
fs.mkdirSync(path.join(modDir, 'civs'), { recursive: true });
fs.writeFileSync(path.join(modDir, 'civs', id.toLowerCase() + '.json'), JSON.stringify({
	id: id.toLowerCase(), index: Number(indexArg), base, name: { spanish: Id, english: Id, italian: Id }, letter: L, ai: true,
		// With "ia": true the AI plays the civilization using the game's scripts, which see the base: these tables translate what they ask for
		// (base classes and orders) into what was cloned. Do not delete them if you enable "ia"
		alias: { classes: Object.fromEntries(rename), commands: aliasOrders },
}, null, 2) + '\n');
console.log(`${rename.size} classes, ${cloned.length} new commands, ${nShared} shared commands patched`);
