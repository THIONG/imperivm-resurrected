// Creates a new outpost from one of the game's and makes it appear spread across the random maps (Imperivm Resurrected SDK).
//
//   node create-outpost.js <DATA folder of the extracted data.pak> <mod folder> <id> [base=IOutpost] [name]
//
// Writes to the mod folder:
//   data\classes\<id>.sc.xml                         a full copy of the base class, with a different id and name
//   patches\data\randommap\placeobjects.xml          adds it to the three random map sizes, in the same quantities as the base
//   patches\data\commands\<file>.xml                 only if the base has orders of its own (the trading posts): gives them to the new one too
//   loc\spanish.loc.xml                              the Spanish name (if the file does not exist)
const fs = require('fs'), path = require('path');

const [dataDir, modDir, id, baseId = 'IOutpost', nameArg] = process.argv.slice(2);
if (!dataDir || !modDir || !id) {
	console.error('usage: node create-outpost.js <DATA> <mod folder> <id> [base=IOutpost] [name]');
	process.exit(1);
}
if (!/^[A-Za-z][A-Za-z0-9_]{0,31}$/.test(id)) {
	console.error('invalid id (letters, digits and _; must start with a letter)');
	process.exit(1);
}
const name = nameArg || id;

const read = f => fs.readFileSync(f, 'latin1');
const ci = (dir, n) => fs.readdirSync(dir).find(f => f.toLowerCase() === n.toLowerCase());
const classesDir = path.join(dataDir, ci(dataDir, 'classes'));
const commandsDir = path.join(dataDir, ci(dataDir, 'commands'));

// 1. The base class
let baseFile = null, xml = null;
for (const f of fs.readdirSync(classesDir)) {
	if (!/\.sc\.xml$/i.test(f)) continue;
	const t = read(path.join(classesDir, f));
	const m = t.match(/<class\b([^>]*)>/);
	if (m && new RegExp('\\bid\\s*=\\s*"' + baseId + '"', 'i').test(m[1])) { baseFile = f; xml = t; break; }
}
if (!xml) { console.error('class ' + baseId + ' not found in ' + classesDir); process.exit(1); }
if (!/cpp_class\s*=\s*"CVXOutpost"/.test(xml) && !/parent\s*=\s*"Outpost"/.test(xml)) {
	console.error(baseId + ' is not an outpost (cpp_class CVXOutpost, parent Outpost)');
	process.exit(1);
}
const displayOld = (xml.match(/display_name\s*=\s*"([^"]*)"/) || [])[1];
xml = xml.replace(/(<class\b[^>]*?\bid\s*=\s*")[^"]*"/, '$1' + id + '"');
// The altid is a unique alias: the copy does not carry it
xml = xml.replace(/(<class\b[^>]*?)\s+altid\s*=\s*"[^"]*"/, '$1');
xml = xml.replace(/(display_name\s*=\s*")[^"]*"/, '$1' + name + '"');
xml = xml.replace(/(edittree_pos\s*=\s*"Structures\/Outposts\/)[^"]*"/, '$1' + name + '"');
fs.mkdirSync(path.join(modDir, 'data', 'classes'), { recursive: true });
fs.writeFileSync(path.join(modDir, 'data', 'classes', id + '.sc.xml'), xml, 'latin1');

// 2. The random maps: placeobjects.xml has three <PlaceGameObjects> blocks (maps of 8192, 16384 and 32768). The outpost is added to each one
// with the same numbers as the base
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
else console.warn('warning: ' + baseId + ' is not in placeobjects.xml: the outpost will not appear on its own in random maps');

// 3. Orders that name the base class (those of the trading posts): the new class is added
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

// 4. The Spanish name
const locFile = path.join(modDir, 'loc', 'spanish.loc.xml');
if (!fs.existsSync(locFile)) {
	fs.mkdirSync(path.dirname(locFile), { recursive: true });
	fs.writeFileSync(locFile, '<translationtable>\n\t<!-- ' + 'The class name: "<display_name>@<class id> class name". Change it to the Spanish name' + ' -->\n' +
		'\t<translationtableentry text="' + name + '@' + id + ' class name" result="' + name + '"/>\n</translationtable>\n');
}

const nShared = [...shared.values()].reduce((a, l) => a + l.length, 0), oldTxt = displayOld ? ', "' + displayOld + '"' : '';
console.log(`outpost ${id} (copy of ${baseId}${oldTxt}): class, ${ops.length} of 3 random map sizes, ${nShared} shared commands`);
