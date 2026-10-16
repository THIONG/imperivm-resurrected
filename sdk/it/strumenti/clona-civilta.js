// Clona una civiltà del gioco come civiltà nuova di una mod (SDK di Imperivm Resurrected).
//
//   node clona-civilta.js <cartella DATA del data.pak estratto> <cartella della mod> <id> <lettera> [base=Gaul] [indice=10]
//
// Scrive nella cartella della mod:
//   civs\<id>.json                     la dichiarazione della civiltà
//   data\classes\<L>*.sc.xml           una copia di ogni classe della base, con un altro id e race="<id>"
//   data\commands\<id>.xml             gli ordini della base che nominano le sue classi, con le classi nuove
//   patches\data\commands\*.xml        aggiunge le classi nuove agli ordini condivisi (riparare, miglioramenti...)
const fs = require('fs'), path = require('path');
const { T } = require('./lib/lang');

const [dataDir, modDir, id, letter, base = 'Gaul', indexArg = '10'] = process.argv.slice(2);
if (!dataDir || !modDir || !id || !letter) {
	console.error(T('uso: node clona-civilta.js <DATA> <carpeta del mod> <id> <letra> [base=Gaul] [indice=10]',
		'usage: node clona-civilta.js <DATA> <mod folder> <id> <letter> [base=Gaul] [index=10]',
		'uso: node clona-civilta.js <DATA> <cartella della mod> <id> <lettera> [base=Gaul] [indice=10]'));
	process.exit(1);
}
if (!/^[A-Za-z][A-Za-z0-9_]{0,23}$/.test(id) || !/^[A-Za-z]$/.test(letter)) {
	console.error(T('id o letra no válidos', 'invalid id or letter', 'id o lettera non validi')); process.exit(1);
}
const L = letter.toUpperCase(), l = letter.toLowerCase();
const Id = id[0].toUpperCase() + id.slice(1);

// Lettera di ogni civiltà del gioco e nome del suo eroe generico
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

// 1. Classi della base: id -> testo del file. Si copia intero: il gioco NON eredita dal padre cose come l'entità (entity),
// e una classe senza entity viene creata senza modello e scompare
const classes = new Map();
const everyClass = new Map();  // tutte le classi del gioco, per id (per quelle che non hanno race ma di cui la civiltà ha bisogno)
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
// Le sentinelle: l'IA, le torri, le mura e i posti di guardia creano "<lettera>Sentry", "<lettera>Sentry1" e "<lettera>GuardPostSentry" con la
// lettera del PROPRIETARIO. Sono classi senza race (figlie di <lettera>Sentry), quindi non entrano per razza: si chiedono per nome. La prima è obbligatoria
for (const extra of ['Sentry', 'Sentry1', 'GuardPostSentry']) {
	const id = bl + extra;
	if (everyClass.has(id) && !classes.has(id)) classes.set(id, everyClass.get(id));
}
// Le classi figlie di quelle della base che non hanno race (GHero1..GHero4, figlie di GaulHero: la razza la ereditano dal padre) sono anch'esse della
// civiltà. Senza di esse non ci sono eroi da reclutare (GetRandomHeroClass resta a cercare per sempre una classe con modello)
const parentOf = t => ((t.match(/<class\b[^>]*?\bparent\s*=\s*"([^"]*)"/) || [])[1]);
for (let grew = true; grew;) {
	grew = false;
	for (const [cid, t] of everyClass) {
		if (classes.has(cid) || /<properties\b[^>]*\brace\s*=/.test(t)) continue;
		const p = parentOf(t);
		if (p && classes.has(p)) { classes.set(cid, t); grew = true; }
	}
}
// Quali classi si clonano: quelle che iniziano con la lettera della base + maiuscola (o "house", come Rhouse03 della Roma repubblicana) e l'eroe generico
const rename = new Map();
for (const cid of classes.keys()) {
	if (new RegExp('^' + bl + '([A-Z]|house)').test(cid)) rename.set(cid, L + cid.slice(1));
	else if (cid === BASES[base].hero) rename.set(cid, Id + 'Hero');
}
console.log(T(`${classes.size} clases con race="${base}", ${rename.size} se clonan`,
	`${classes.size} classes with race="${base}", ${rename.size} will be cloned`,
	`${classes.size} classi con race="${base}", ${rename.size} verranno clonate`));

// Molti ordini non nominano la classe ma il suo alias (altid): <src obj="DruidHouse"/> è la casa dei druidi della Gallia (GDruidHouse, altid
// "DruidHouse"). Poiché le copie non hanno altid, gli ordini di quegli alias si offrono alla copia tramite l'id della sua classe
const aliasOf = new Map();  // altid -> id della classe originale
for (const [cid, text] of classes) {
	const alias = (text.match(/<class\b[^>]*?\baltid\s*=\s*"([^"]*)"/) || [])[1];
	if (alias && rename.has(cid)) aliasOf.set(alias, cid);
}

// 2. Classi nuove: copie delle originali con un altro id e un'altra civiltà. I percorsi di immagini ed entità non cambiano
fs.mkdirSync(path.join(modDir, 'data', 'classes'), { recursive: true });
for (const [oldId, newId] of rename) {
	let xml = classes.get(oldId);
	xml = xml.replace(/(<class\b[^>]*?\bid\s*=\s*")[^"]*"/, '$1' + newId + '"');
	xml = xml.replace(/(<class\b[^>]*?\bparent\s*=\s*")([^"]*)"/, (s, p, par) => p + (rename.get(par) || par) + '"');
	xml = xml.split('race="' + base + '"').join('race="' + Id + '"');
	// L'altid è un alias unico («Barrack», «Village»...) con cui gli script e l'IA cercano "la caserma" di una civiltà: il gioco
	// lo registra per una sola classe. Se la copia lo ripete, o l'IA della copia o quella dell'originale smettono di vedere i propri edifici
	xml = xml.replace(/(<class\b[^>]*?)\s+altid\s*=\s*"[^"]*"/, '$1');
	// Mura, porte e torri mettono le loro sentinelle con sentry_class_name="GSentry1": nella copia, quelle della copia
	xml = xml.replace(/(\bsentry_class_name\s*=\s*")([^"]*)"/g, (s, a, v) => a + (rename.get(v) || v) + '"');
	fs.writeFileSync(path.join(modDir, 'data', 'classes', newId + '.sc.xml'), xml, 'latin1');
}

// 3. Ordini
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
const aliasOrders = {};  // nome dell'ordine della base -> nome dell'ordine clonato (per l'IA)
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
		// Le esclusioni (<nsrc>): un ordine di "tutte le postazioni tranne GGuardPost" offrirebbe i suoi pulsanti al DGuardPost se non lo si esclude
			const nsrcs = [...body.matchAll(/<nsrc\s+obj\s*=\s*"([^"]*)"/g)].map(x => aliasOf.get(x[1]) || x[1]).filter(s => rename.has(s));
			if (nsrcs.length && name) {
				if (!shared.has(f)) shared.set(f, []);
				shared.get(f).push({ name, srcs: [], nsrcs: nsrcs.map(s => rename.get(s)) });
			}
			if (!mine.length || !name) continue;
		const head = body.replace(/<src\b[\s\S]*$/, '');
		const refs = new RegExp(classRe.source).test(head);
		if (refs || mapName(name) !== name || /^trainpeasant|^hirehero/i.test(name)) {
			// l'ordine nomina classi della base: copia con le classi nuove
			let nn = mapName(name);
			if (nn === name) nn = name + ' ' + Id;
			let c = body.replace(/<src\b[^>]*\/>\s*/g, '');
			c = c.replace(/(\bname\s*=\s*")[^"]*"/, '$1' + nn + '"');
			const srcTxt = mine.map(s => '\t<src obj="' + rename.get(s) + '"/>').join('\n');
			// Si cambiano solo le classi di param e sclass: le immagini (button, queueicon...), l'aiuto e i testi sono quelli della base
			c = c.replace(/(\b(?:param|sclass)\s*=\s*")([^"]*)"/g, (s, a, v) => a + mapIds(v) + '"');
			c = c.replace(/<\/cmd>$/, srcTxt + '\n</cmd>');
			cloned.push(c);
			aliasOrders[name] = nn;
		}
		else {
			// ordine generico (riparare, miglioramenti...): si modifica con una patch l'originale perché lo offrano anche le classi nuove
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

// 4. Dichiarazione
fs.mkdirSync(path.join(modDir, 'civs'), { recursive: true });
fs.writeFileSync(path.join(modDir, 'civs', id.toLowerCase() + '.json'), JSON.stringify({
	id: id.toLowerCase(), index: Number(indexArg), base, name: { spanish: Id, english: Id, italian: Id }, letter: L, ai: true,
		// Con "ia": true l'IA gioca con la civiltà usando gli script del gioco, che vedono la base: queste tabelle traducono ciò che chiedono
		// (classi e ordini della base) in ciò che è stato clonato. Non cancellarle se attivi "ia"
		alias: { classes: Object.fromEntries(rename), commands: aliasOrders },
}, null, 2) + '\n');
console.log(T(`${rename.size} clases, ${cloned.length} órdenes nuevas, ${nShared} órdenes compartidas parcheadas`,
	`${rename.size} classes, ${cloned.length} new commands, ${nShared} shared commands patched`,
	`${rename.size} classi, ${cloned.length} nuovi comandi, ${nShared} comandi condivisi modificati con patch`));
