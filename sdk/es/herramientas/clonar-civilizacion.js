// Clona una civilización del juego como civilización nueva de un mod (SDK de Imperivm Resurrected).
//
//   node clonar-civilizacion.js <carpeta DATA del data.pak extraído> <carpeta del mod> <id> <letra> [base=Gaul] [indice=10]
//
// Escribe en la carpeta del mod:
//   civs\<id>.json                     la declaración de la civilización
//   data\classes\<L>*.sc.xml           una copia de cada clase de la base, con otro id y race="<id>"
//   data\commands\<id>.xml             las órdenes de la base que nombran sus clases, con las clases nuevas
//   patches\data\commands\*.xml        añade las clases nuevas a las órdenes compartidas (reparar, mejoras...)
const fs = require('fs'), path = require('path');
const { T } = require('./lib/lang');

const [dataDir, modDir, id, letter, base = 'Gaul', indexArg = '10'] = process.argv.slice(2);
if (!dataDir || !modDir || !id || !letter) {
	console.error(T('uso: node clonar-civilizacion.js <DATA> <carpeta del mod> <id> <letra> [base=Gaul] [indice=10]',
		'usage: node clonar-civilizacion.js <DATA> <mod folder> <id> <letter> [base=Gaul] [index=10]',
		'uso: node clonar-civilizacion.js <DATA> <cartella della mod> <id> <lettera> [base=Gaul] [indice=10]'));
	process.exit(1);
}
if (!/^[A-Za-z][A-Za-z0-9_]{0,23}$/.test(id) || !/^[A-Za-z]$/.test(letter)) {
	console.error(T('id o letra no válidos', 'invalid id or letter', 'id o lettera non validi')); process.exit(1);
}
const L = letter.toUpperCase(), l = letter.toLowerCase();
const Id = id[0].toUpperCase() + id.slice(1);

// Letra de cada civilización del juego y nombre de su héroe genérico
const BASES = {
	Gaul: { letter: 'G', hero: 'GaulHero' }, RepublicanRome: { letter: 'R', hero: 'RomeHero' },
	Carthage: { letter: 'C', hero: 'CarthageHero' }, Iberia: { letter: 'I', hero: 'IberiaHero' },
	ImperialRome: { letter: 'M', hero: 'ImperialHero' }, Britain: { letter: 'B', hero: 'BritainHero' },
	Egypt: { letter: 'E', hero: 'EgyptHero' }, Germany: { letter: 'T', hero: 'GermanHero' },
};
if (!BASES[base]) { console.error(T('base desconocida: ' + base, 'unknown base: ' + base, 'base sconosciuta: ' + base)); process.exit(1); }
const bl = BASES[base].letter, blow = bl.toLowerCase();

const read = f => fs.readFileSync(f, 'latin1');
const ci = (dir, name) => fs.readdirSync(dir).find(f => f.toLowerCase() === name.toLowerCase());
const classesDir = path.join(dataDir, ci(dataDir, 'classes'));
const commandsDir = path.join(dataDir, ci(dataDir, 'commands'));

// 1. Clases de la base: id -> texto del archivo. Se copia entero: el juego NO hereda del padre cosas como la entidad (entity),
// y una clase sin entity se crea sin modelo y desaparece
const classes = new Map();
const everyClass = new Map();  // todas las clases del juego, por id (para las que no llevan race pero la civilización necesita)
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
// Los centinelas: la IA, las torres, los muros y los puestos de guardia crean "<letra>Sentry", "<letra>Sentry1" y "<letra>GuardPostSentry" con la
// letra del DUEÑO. Son clases sin race (hijas de <letra>Sentry), así que no entran por la raza: se piden por nombre. La primera es obligatoria
for (const extra of ['Sentry', 'Sentry1', 'GuardPostSentry']) {
	const id = bl + extra;
	if (everyClass.has(id) && !classes.has(id)) classes.set(id, everyClass.get(id));
}
// Las clases hijas de las de la base que no llevan race (GHero1..GHero4, hijas de GaulHero: la raza la heredan del padre) también son de la
// civilización. Sin ellas no hay héroes que contratar (GetRandomHeroClass se queda buscando para siempre una clase con modelo)
const parentOf = t => ((t.match(/<class\b[^>]*?\bparent\s*=\s*"([^"]*)"/) || [])[1]);
for (let grew = true; grew;) {
	grew = false;
	for (const [cid, t] of everyClass) {
		if (classes.has(cid) || /<properties\b[^>]*\brace\s*=/.test(t)) continue;
		const p = parentOf(t);
		if (p && classes.has(p)) { classes.set(cid, t); grew = true; }
	}
}
// Qué clases se clonan: las que empiezan por la letra de la base + mayúscula (o "house", como Rhouse03 de Roma republicana) y el héroe genérico
const rename = new Map();
for (const cid of classes.keys()) {
	if (new RegExp('^' + bl + '([A-Z]|house)').test(cid)) rename.set(cid, L + cid.slice(1));
	else if (cid === BASES[base].hero) rename.set(cid, Id + 'Hero');
}
console.log(T(`${classes.size} clases con race="${base}", ${rename.size} se clonan`,
	`${classes.size} classes with race="${base}", ${rename.size} will be cloned`,
	`${classes.size} classi con race="${base}", ${rename.size} verranno clonate`));

// Muchas órdenes no nombran la clase sino su alias (altid): <src obj="DruidHouse"/> es la casa de los druidas de Galia (GDruidHouse, altid
// "DruidHouse"). Como las copias no llevan altid, las órdenes de esos alias se ofrecen a la copia por el id de su clase
const aliasOf = new Map();  // altid -> id de la clase original
for (const [cid, text] of classes) {
	const alias = (text.match(/<class\b[^>]*?\baltid\s*=\s*"([^"]*)"/) || [])[1];
	if (alias && rename.has(cid)) aliasOf.set(alias, cid);
}

// 2. Clases nuevas: copias de las originales con otro id y otra civilización. Las rutas de imágenes y entidades no cambian
fs.mkdirSync(path.join(modDir, 'data', 'classes'), { recursive: true });
for (const [oldId, newId] of rename) {
	let xml = classes.get(oldId);
	xml = xml.replace(/(<class\b[^>]*?\bid\s*=\s*")[^"]*"/, '$1' + newId + '"');
	xml = xml.replace(/(<class\b[^>]*?\bparent\s*=\s*")([^"]*)"/, (s, p, par) => p + (rename.get(par) || par) + '"');
	xml = xml.split('race="' + base + '"').join('race="' + Id + '"');
	// El altid es un alias único («Barrack», «Village»...) con el que los scripts y la IA buscan "el cuartel" de una civilización: el juego
	// lo registra para una sola clase. Si la copia lo repite, o la IA de la copia o la de la original dejan de ver sus edificios
	xml = xml.replace(/(<class\b[^>]*?)\s+altid\s*=\s*"[^"]*"/, '$1');
	// Muros, puertas y torres ponen sus centinelas con sentry_class_name="GSentry1": en la copia, los de la copia
	xml = xml.replace(/(\bsentry_class_name\s*=\s*")([^"]*)"/g, (s, a, v) => a + (rename.get(v) || v) + '"');
	fs.writeFileSync(path.join(modDir, 'data', 'classes', newId + '.sc.xml'), xml, 'latin1');
}

// 3. Órdenes
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
const aliasOrders = {};  // nombre de la orden de la base -> nombre de la orden clonada (para la IA)
const shared = new Map();  // archivo -> [{name, srcs}]
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
		// Las exclusiones (<nsrc>): una orden de "todos los puestos menos GGuardPost" ofrecería sus botones al DGuardPost si no se excluye
			const nsrcs = [...body.matchAll(/<nsrc\s+obj\s*=\s*"([^"]*)"/g)].map(x => aliasOf.get(x[1]) || x[1]).filter(s => rename.has(s));
			if (nsrcs.length && name) {
				if (!shared.has(f)) shared.set(f, []);
				shared.get(f).push({ name, srcs: [], nsrcs: nsrcs.map(s => rename.get(s)) });
			}
			if (!mine.length || !name) continue;
		const head = body.replace(/<src\b[\s\S]*$/, '');
		const refs = new RegExp(classRe.source).test(head);
		if (refs || mapName(name) !== name || /^trainpeasant|^hirehero/i.test(name)) {
			// la orden nombra clases de la base: copia con las clases nuevas
			let nn = mapName(name);
			if (nn === name) nn = name + ' ' + Id;
			let c = body.replace(/<src\b[^>]*\/>\s*/g, '');
			c = c.replace(/(\bname\s*=\s*")[^"]*"/, '$1' + nn + '"');
			const srcTxt = mine.map(s => '\t<src obj="' + rename.get(s) + '"/>').join('\n');
			// Solo se cambian las clases de param y sclass: las imágenes (button, queueicon...), la ayuda y los textos son los de la base
			c = c.replace(/(\b(?:param|sclass)\s*=\s*")([^"]*)"/g, (s, a, v) => a + mapIds(v) + '"');
			c = c.replace(/<\/cmd>$/, srcTxt + '\n</cmd>');
			cloned.push(c);
			aliasOrders[name] = nn;
		}
		else {
			// orden genérica (reparar, mejoras...): se parchea la original para que también la ofrezcan las clases nuevas
			if (!shared.has(f)) shared.set(f, []);
			shared.get(f).push({ name, srcs: mine.map(s => rename.get(s)) });
			nShared++;
		}
	}
}
fs.mkdirSync(path.join(modDir, 'data', 'commands'), { recursive: true });
fs.writeFileSync(path.join(modDir, 'data', 'commands', Id + '.xml'),
	'<commands>\n\n<!-- ' + T(`${Id} (clonada de ${base})`, `${Id} (cloned from ${base})`, `${Id} (clonata da ${base})`) + ' -->\n\n' + cloned.join('\n\n') + '\n\n</commands>\n', 'latin1');
for (const [f, list] of shared) {
	const dest = path.join(modDir, 'patches', 'data', 'commands', f);
	fs.mkdirSync(path.dirname(dest), { recursive: true });
	const ops = list.map(o => '\t<add select="/commands/cmd[@name=\'' + o.name + '\']">' +
		o.srcs.map(s => '<src obj="' + s + '"/>').join('') + (o.nsrcs || []).map(s => '<nsrc obj="' + s + '"/>').join('') + '</add>');
	fs.writeFileSync(dest, '<patch>\n' + ops.join('\n') + '\n</patch>\n', 'latin1');
}

// 4. Declaración
fs.mkdirSync(path.join(modDir, 'civs'), { recursive: true });
fs.writeFileSync(path.join(modDir, 'civs', id.toLowerCase() + '.json'), JSON.stringify({
	id: id.toLowerCase(), index: Number(indexArg), base, name: { spanish: Id, english: Id, italian: Id }, letter: L, ai: true,
		// Con "ia": true la IA juega con la civilización usando los guiones del juego, que ven la base: estas tablas traducen lo que piden
		// (clases y órdenes de la base) a lo clonado. No las borres si activas "ia"
		alias: { classes: Object.fromEntries(rename), commands: aliasOrders },
}, null, 2) + '\n');
console.log(T(`${rename.size} clases, ${cloned.length} órdenes nuevas, ${nShared} órdenes compartidas parcheadas`,
	`${rename.size} classes, ${cloned.length} new commands, ${nShared} shared commands patched`,
	`${rename.size} classi, ${cloned.length} nuovi comandi, ${nShared} comandi condivisi modificati con patch`));
