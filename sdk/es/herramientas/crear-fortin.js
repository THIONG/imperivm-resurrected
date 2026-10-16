// Crea un fortín (puesto avanzado) nuevo a partir de uno del juego, y hace que salga repartido por los mapas aleatorios (SDK de Imperivm Resurrected).
//
//   node crear-fortin.js <carpeta DATA del data.pak extraído> <carpeta del mod> <id> [base=IOutpost] [nombre]
//
// Escribe en la carpeta del mod:
//   data\classes\<id>.sc.xml                         una copia entera de la clase de la base, con otro id y otro nombre
//   patches\data\randommap\placeobjects.xml          lo añade a los tres tamaños de mapa aleatorio, con las mismas cantidades que la base
//   patches\data\commands\<archivo>.xml              solo si la base tiene órdenes propias (los puestos comerciales): se las da también al nuevo
//   loc\spanish.loc.xml                              el nombre en español (si el archivo no existe)
const fs = require('fs'), path = require('path');
const { T } = require('./lib/lang');

const [dataDir, modDir, id, baseId = 'IOutpost', nameArg] = process.argv.slice(2);
if (!dataDir || !modDir || !id) {
	console.error(T('uso: node crear-fortin.js <DATA> <carpeta del mod> <id> [base=IOutpost] [nombre]',
		'usage: node crear-fortin.js <DATA> <mod folder> <id> [base=IOutpost] [name]',
		'uso: node crear-fortin.js <DATA> <cartella della mod> <id> [base=IOutpost] [nome]'));
	process.exit(1);
}
if (!/^[A-Za-z][A-Za-z0-9_]{0,31}$/.test(id)) {
	console.error(T('id no válido (letras, números y _; empieza por letra)', 'invalid id (letters, digits and _; must start with a letter)', 'id non valido (lettere, numeri e _; inizia con una lettera)'));
	process.exit(1);
}
const name = nameArg || id;

const read = f => fs.readFileSync(f, 'latin1');
const ci = (dir, n) => fs.readdirSync(dir).find(f => f.toLowerCase() === n.toLowerCase());
const classesDir = path.join(dataDir, ci(dataDir, 'classes'));
const commandsDir = path.join(dataDir, ci(dataDir, 'commands'));

// 1. La clase de la base
let baseFile = null, xml = null;
for (const f of fs.readdirSync(classesDir)) {
	if (!/\.sc\.xml$/i.test(f)) continue;
	const t = read(path.join(classesDir, f));
	const m = t.match(/<class\b([^>]*)>/);
	if (m && new RegExp('\\bid\\s*=\\s*"' + baseId + '"', 'i').test(m[1])) { baseFile = f; xml = t; break; }
}
if (!xml) { console.error(T('no encuentro la clase ' + baseId + ' en ' + classesDir, 'class ' + baseId + ' not found in ' + classesDir, 'classe ' + baseId + ' non trovata in ' + classesDir)); process.exit(1); }
if (!/cpp_class\s*=\s*"CVXOutpost"/.test(xml) && !/parent\s*=\s*"Outpost"/.test(xml)) {
	console.error(T(baseId + ' no es un puesto avanzado (cpp_class CVXOutpost, parent Outpost)',
		baseId + ' is not an outpost (cpp_class CVXOutpost, parent Outpost)',
		baseId + ' non è un avamposto (cpp_class CVXOutpost, parent Outpost)'));
	process.exit(1);
}
const displayOld = (xml.match(/display_name\s*=\s*"([^"]*)"/) || [])[1];
xml = xml.replace(/(<class\b[^>]*?\bid\s*=\s*")[^"]*"/, '$1' + id + '"');
// El altid es un alias único: la copia no lo lleva
xml = xml.replace(/(<class\b[^>]*?)\s+altid\s*=\s*"[^"]*"/, '$1');
xml = xml.replace(/(display_name\s*=\s*")[^"]*"/, '$1' + name + '"');
xml = xml.replace(/(edittree_pos\s*=\s*"Structures\/Outposts\/)[^"]*"/, '$1' + name + '"');
fs.mkdirSync(path.join(modDir, 'data', 'classes'), { recursive: true });
fs.writeFileSync(path.join(modDir, 'data', 'classes', id + '.sc.xml'), xml, 'latin1');

// 2. Los mapas aleatorios: placeobjects.xml tiene tres bloques <PlaceGameObjects> (mapas de 8192, 16384 y 32768). Se añade el fortín en cada uno
// con los mismos números que la base
const poFile = path.join(dataDir, ci(dataDir, 'randommap'), 'placeobjects.xml');
const po = read(poFile);
const blocks = [...po.matchAll(/<PlaceGameObjects\b[\s\S]*?<\/PlaceGameObjects>/g)].map(m => m[0]);
const ops = [];
blocks.forEach((b, i) => {
	const m = b.match(new RegExp('<ifdef[^>]*>\\s*<GameObj\\s+class="' + baseId + '"[^>]*/>\\s*</ifdef>', 'i'));
	if (m) ops.push('\t<add select="/placeobjects/PlaceGameObjects[' + (i + 1) + ']">' + m[0].replace(new RegExp('class="' + baseId + '"', 'i'), 'class="' + id + '"') + '</add>');
});
if (ops.length) {
	const dest = path.join(modDir, 'patches', 'data', 'randommap', 'placeobjects.xml');
	fs.mkdirSync(path.dirname(dest), { recursive: true });
	fs.writeFileSync(dest, '<patch>\n' + ops.join('\n') + '\n</patch>\n', 'latin1');
}
else console.warn(T('aviso: ' + baseId + ' no sale en placeobjects.xml: el fortín no saldrá solo en los mapas aleatorios',
	'warning: ' + baseId + ' is not in placeobjects.xml: the outpost will not appear on its own in random maps',
	'avviso: ' + baseId + ' non è in placeobjects.xml: l\'avamposto non comparirà da solo nelle mappe casuali'));

// 3. Órdenes que nombran la clase de la base (las de los puestos comerciales): se añade la nueva
const shared = new Map();
for (const f of fs.readdirSync(commandsDir)) {
	const t = read(path.join(commandsDir, f));
	for (const m of t.matchAll(/<cmd\b[\s\S]*?<\/cmd>/g)) {
		const cmd = (m[0].match(/\bname\s*=\s*"([^"]*)"/) || [])[1];
		if (cmd && new RegExp('<src\\s+obj\\s*=\\s*"' + baseId + '"').test(m[0])) {
			if (!shared.has(f)) shared.set(f, []);
			shared.get(f).push(cmd);
		}
	}
}
for (const [f, list] of shared) {
	const dest = path.join(modDir, 'patches', 'data', 'commands', f);
	fs.mkdirSync(path.dirname(dest), { recursive: true });
	fs.writeFileSync(dest, '<patch>\n' + list.map(c => '\t<add select="/commands/cmd[@name=\'' + c + '\']"><src obj="' + id + '"/></add>').join('\n') + '\n</patch>\n', 'latin1');
}

// 4. El nombre en español
const locFile = path.join(modDir, 'loc', 'spanish.loc.xml');
if (!fs.existsSync(locFile)) {
	fs.mkdirSync(path.dirname(locFile), { recursive: true });
	fs.writeFileSync(locFile, '<translationtable>\n\t<!-- ' + T('El nombre de la clase: "<display_name>@<id de la clase> class name". Cámbialo por el nombre en español',
		'The class name: "<display_name>@<class id> class name". Change it to the Spanish name',
		'Il nome della classe: "<display_name>@<id della classe> class name". Cambialo con il nome in spagnolo') + ' -->\n' +
		'\t<translationtableentry text="' + name + '@' + id + ' class name" result="' + name + '"/>\n</translationtable>\n');
}

const nShared = [...shared.values()].reduce((a, l) => a + l.length, 0), oldTxt = displayOld ? ', "' + displayOld + '"' : '';
console.log(T(`fortín ${id} (copia de ${baseId}${oldTxt}): clase, ${ops.length} de 3 tamaños de mapa aleatorio, ${nShared} órdenes compartidas`,
	`outpost ${id} (copy of ${baseId}${oldTxt}): class, ${ops.length} of 3 random map sizes, ${nShared} shared commands`,
	`avamposto ${id} (copia di ${baseId}${oldTxt}): classe, ${ops.length} di 3 dimensioni di mappa casuale, ${nShared} comandi condivisi`));
