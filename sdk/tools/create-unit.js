// Creates a new unit from one of the game's: its class, the order to recruit it and its texts (Imperivm Resurrected SDK).
//
//   node create-unit.js <DATA folder of the extracted data.pak> <mod folder> <id> --base <game class> [options]
//
// Options:
//   --health N  --damage N  --armor N  --speed N   stats (without them, the base's). --armor sets armor_slash and armor_pierce
//   --building <class>                                 where it is recruited (without it, where the base is recruited)
//   --gold-cost N  --food-cost N  --time N        recruiting price and time (milliseconds)
//   --key x                                          the button's hotkey (without it, a free one in that building)
//   --name "Text"  --plural "Texts"                 name in English, the text of the game (also writes loc\english.loc.xml)
//   --name-es "Texto"  --plural-es "Textos"          name in Spanish (also writes loc\spanish.loc.xml)
//   --name-it "Testo"                                name in Italian (also writes loc\italian.loc.xml)
//
// Writes to the mod folder (nothing, if something fails):
//   data\classes\<id>.sc.xml            FULL copy of the base class, with a different id and the requested stats
//   data\commands\<id>.xml              the order to recruit it: a copy of the base's, with a different class, cost and hotkey
//   loc\spanish.loc.xml (and the others)   the texts; if the file already exists, the entries are ADDED
//   mod.json                            only if the mod does not have one
const fs = require('fs'), path = require('path');

const HELP = 'usage: node create-unit.js <DATA> <mod folder> <id> --base <game class> [--health N --damage N --armor N --speed N\n' +
	'         --building <class> --gold-cost N --food-cost N --time N --key x --name "Text" --plural "Texts"\n' +
	'         --name-es "Texto" --plural-es "Textos" --name-it "Testo"]';
const die = msg => { console.error('error: ' + msg); process.exit(1); };

// --- arguments --------------------------------------------------------------------------------------------------------
const OPTS = ['base', 'health', 'damage', 'armor', 'speed', 'building', 'gold-cost', 'food-cost', 'time', 'key',
	'name', 'plural', 'name-es', 'plural-es', 'name-it'];
const pos = [], opt = {};
{
	const a = process.argv.slice(2);
	for (let i = 0; i < a.length; i++) {
		if (!a[i].startsWith('--')) { pos.push(a[i]); continue; }
		let [name, value] = a[i].slice(2).split(/=(.*)/s);
		if (name === 'ayuda' || name === 'help') { console.log(HELP); process.exit(0); }
		if (!OPTS.includes(name)) die('unknown option: --' + name + '\n' + HELP);
		if (value === undefined) { value = a[++i]; if (value === undefined || value.startsWith('--')) die('--' + name + ' is missing its value'); }
		opt[name] = value;
	}
}
if (pos.length !== 3) { console.error(HELP); process.exit(1); }
const [dataDir, modDir, id] = pos;
if (!opt.base) die('missing --base <game class> (the unit to copy, for example RHastatus)');
if (!/^[A-Za-z][A-Za-z0-9_]{0,23}$/.test(id)) die('invalid id: "' + id + '" (letters, digits and _; must start with a letter; up to 24)');
const num = (name, min) => {
	if (opt[name] === undefined) return undefined;
	if (!/^-?\d+$/.test(opt[name]) || Number(opt[name]) < min) die('--' + name + ' must be an integer of ' + min + ' or more (got "' + opt[name] + '")');
	return Number(opt[name]);
};
const stats = { maxhealth: num('health', 1), damage: num('damage', 0), armor: num('armor', 0), speed: num('speed', 1) };
const costGold = num('gold-cost', 0), costFood = num('food-cost', 0), execDelay = num('time', 0);
if (opt.key !== undefined && !/^[A-Za-z0-9]$/.test(opt.key)) die('--key must be a single letter or digit (got "' + opt.key + '")');
for (const k of ['name', 'plural', 'name-es', 'plural-es', 'name-it']) {
	if (opt[k] !== undefined && (!opt[k].trim() || /[\r\n]/.test(opt[k]) || opt[k].includes('@'))) die('--' + k + ' cannot be empty or contain @ or line breaks');
}

// --- reading the game data ---------------------------------------------------------------------------------------------
const lower = s => s.toLowerCase();
const ci = (dir, n) => { const f = fs.readdirSync(dir).find(f => lower(f) === lower(n)); return f; };
if (!fs.existsSync(dataDir) || !fs.statSync(dataDir).isDirectory()) die('the DATA folder does not exist: ' + dataDir);
const classesName = ci(dataDir, 'classes'), commandsName = ci(dataDir, 'commands');
if (!classesName || !commandsName) die(dataDir + ' does not look like the DATA folder of data.pak (classes and commands are missing). Extract it with extract-data.js');
const classesDir = path.join(dataDir, classesName), commandsDir = path.join(dataDir, commandsName);
// The game's XMLs are ASCII; a mod's are UTF-8. Read as UTF-8 if valid and, if not, as Latin-1
const readText = f => {
	const b = fs.readFileSync(f);
	let t;
	try { t = new TextDecoder('utf-8', { fatal: true }).decode(b); } catch (e) { t = b.toString('latin1'); }
	return t.replace(/^﻿/, '').replace(/\r\n/g, '\n');
};
const attr = (tag, name) => { const m = tag.match(new RegExp('\\b' + name + '\\s*=\\s*"([^"]*)"')); return m ? m[1] : undefined; };
const esc = s => s.replace(/&/g, '&amp;').replace(/"/g, '&quot;').replace(/</g, '&lt;').replace(/>/g, '&gt;');

const classes = new Map();   // id (lowercase) -> { id, altid, file, text, header }
const aliases = new Map();   // altid (lowercase) -> real id
const takenIds = new Set();
const scanClasses = (dir, into) => {
	for (const f of fs.readdirSync(dir)) {
		if (!/\.sc\.xml$/i.test(f)) continue;
		const t = readText(path.join(dir, f));
		const h = t.match(/<class\b[^>]*>/i);
		if (!h) continue;
		const cid = attr(h[0], 'id'), alt = attr(h[0], 'altid');
		if (!cid) continue;
		takenIds.add(lower(cid));
		if (alt) { takenIds.add(lower(alt)); aliases.set(lower(alt), cid); }
		if (into) into.set(lower(cid), { id: cid, altid: alt, file: f, text: t, header: h[0] });
	}
};
scanClasses(classesDir, classes);
// Classes the mod already has (and other files of its own), to avoid repeating the id
const modClassesDir = path.join(modDir, 'data', 'classes');
if (fs.existsSync(modClassesDir)) scanClasses(modClassesDir, null);

if (takenIds.has(lower(id))) die('the id "' + id + '" is already used by a class (from the game or your mod). Pick another');

const base = classes.get(lower(opt.base)) || classes.get(lower(aliases.get(lower(opt.base)) || ''));
if (!base) die('base class "' + opt.base + '" not found in ' + classesDir + '. Use the id of a game unit (for example RHastatus)');
const cpp = attr(base.header, 'cpp_class');
if (!cpp || !/^CVX(Unit|Druid|Catapult|Wagon|Ship|FlyingUnit|Hero)$/.test(cpp)) {
	die(base.id + ' is not a unit (' + (cpp ? 'cpp_class ' + cpp : 'it has no cpp_class, it is an abstract class') + '). Pick a complete unit, such as RHastatus or GSwordsman');
}
if (!/\bentity\s*=/.test(base.header)) die(base.id + ' has no entity (the model): a copy would vanish when created. Pick a complete unit');
const baseId = base.id;

// --- the game's orders --------------------------------------------------------------------------------------------------
const cmds = [];   // { name, file, text, open, srcs }
const takenCmdNames = new Set();
const usedKeys = new Map();  // src (lowercase) -> Set of hotkeys
const scanCommands = (dir, isGame) => {
	for (const f of fs.readdirSync(dir)) {
		if (!/\.xml$/i.test(f)) continue;
		const t = readText(path.join(dir, f));
		for (const m of t.matchAll(/<cmd\b[\s\S]*?<\/cmd\s*>/gi)) {
			const open = m[0].match(/<cmd\b[^>]*>/i)[0];
			const name = attr(open, 'name');
			if (name) takenCmdNames.add(lower(name));
			const srcs = [...m[0].matchAll(/<src\b[^>]*\bobj\s*=\s*"([^"]*)"/gi)].map(s => s[1]);
			const key = attr(open, 'key');
			if (key) for (const s of srcs) { if (!usedKeys.has(lower(s))) usedKeys.set(lower(s), new Set()); usedKeys.get(lower(s)).add(lower(key)); }
			if (isGame) cmds.push({ name, file: f, text: m[0], open, srcs });
		}
	}
};
scanCommands(commandsDir, true);
const modCommandsDir = path.join(modDir, 'data', 'commands');
if (fs.existsSync(modCommandsDir)) scanCommands(modCommandsDir, false);

const trainName = 'train' + id;
if (takenCmdNames.has(lower(trainName))) die('a command named "' + trainName + '" already exists (in the game or your mod). Pick another id');

const baseNames = [lower(baseId), base.altid ? lower(base.altid) : null].filter(Boolean);
const trains = cmds.filter(c => attr(c.open, 'method') === 'train' && baseNames.includes(lower((attr(c.open, 'param') || '').split(',')[0].trim())));
if (!trains.length) {
	const hired = cmds.some(c => attr(c.open, 'method') === 'trainex' && new RegExp('\\b' + baseId + '\\b', 'i').test(attr(c.open, 'param') || ''));
	die('the game has no regular recruit command (method="train") for ' + baseId + (hired
		? ': it is hired as a group (method="trainex"), and this tool does not copy that kind of command. Pick another base'
		: '. Pick a base that is recruited in a building (for example RHastatus), or write the command by hand (units.md)'));
}

// Building where it is recruited
let building = opt.building;
// If the base is recruited in several places, the building of its own civilization is preferred
const baseRace = attr((base.text.match(/<properties\b[^>]*\brace\s*=[^>]*>/i) || [''])[0], 'race');
const srcRaceOk = c => c.srcs.some(s => {
	const k = classes.get(lower(s)) || classes.get(lower(aliases.get(lower(s)) || ''));
	return k && baseRace && attr((k.text.match(/<properties\b[^>]*\brace\s*=[^>]*>/i) || [''])[0], 'race') === baseRace;
});
const sameRace = trains.filter(srcRaceOk);
let chosen = sameRace[0] || trains[0];
if (building) {
	const real = classes.get(lower(building)) || classes.get(lower(aliases.get(lower(building)) || ''));
	if (!real && !(fs.existsSync(modClassesDir) && takenIds.has(lower(building)))) {
		die('the building "' + building + '" does not exist (neither in the game nor in your mod). Use the id of a building class, for example RBarracks');
	}
	const bcpp = real && attr(real.header, 'cpp_class');
	if (bcpp && /^CVX(Unit|Hero|Decor|Feedback|ScriptObj)$/.test(bcpp)) die('"' + building + '" is not a building (cpp_class ' + bcpp + ')');
	chosen = trains.find(c => c.srcs.some(s => lower(s) === lower(building))) || chosen;
}
const buildings = building ? [building] : chosen.srcs;
if (!buildings.length) die('the recruit command of ' + baseId + ' does not say which building it comes from. Give one with --building');
const notes = [];
if ((sameRace.length ? sameRace : trains).length > 1 && !building) notes.push(baseId + ' is recruited with several commands (' + trains.map(c => c.srcs.join('/')).join('; ') + '): the one for ' + chosen.srcs.join('/') + ' was copied. Use --building to choose');
if (building && !chosen.srcs.some(s => lower(s) === lower(building))) notes.push(baseId + ' is not recruited in ' + building + ': the command was copied from the one for ' + chosen.srcs.join('/') + ' and its verifier (groupverifier) may not fit there');

// --- names and texts ----------------------------------------------------------------------------------------------------
const baseDisplay = attr((base.text.match(/<properties\b[^>]*\bdisplay_name\s*=[^>]*>/i) || [''])[0], 'display_name');
const upperFirst = s => s[0].toUpperCase() + s.slice(1), lowerFirst = s => s[0].toLowerCase() + s.slice(1);
const pluralEn = s => /s$/i.test(s) ? s : s + 's';
const pluralEs = s => /z$/i.test(s) ? s.slice(0, -1) + 'ces' : /[aeiouáéíóú0-9]$/i.test(s) ? s + 's' : /s$/i.test(s) ? s : s + 'es';
const nameEn = opt.name || id;
const nameEs = opt['name-es'], nameIt = opt['name-it'];
const key = nameEn;                                                     // display_name is the text key
const keyPlural = opt.plural || pluralEn(nameEn);
if (!opt.name) notes.push('you did not give --name: the unit is called "' + id + '". Change it in loc\\english.loc.xml');
if (/\s/.test(key.trim()) && !opt.plural) notes.push('the plural of a multi-word name is guessed badly ("' + keyPlural + '"): give --plural' + (nameEs ? ' / --plural-es' : ''));

// The button text: the order's "rollover" (verb + name)
const baseRollover = attr(chosen.open, 'rollover') || '';
const verbEn = (baseRollover.match(/^(\S+)\s/) || [])[1] || 'Train';
const verbs = { Equip: ['Equipar', 'Equipaggia'], Hire: ['Contratar', 'Assolda'], Train: ['Entrenar', 'Addestra'], Recruit: ['Reclutar', 'Recluta'] };
const [verbEs, verbIt] = verbs[verbEn] || verbs.Train;
const rollover = baseDisplay && baseRollover.includes(baseDisplay) ? baseRollover.split(baseDisplay).join(key) : (verbs[verbEn] ? verbEn : 'Train') + ' ' + key;

// The hotkey: the requested one, or a letter of the name that is free in that building
const keysInUse = new Set();
for (const b of buildings) for (const k of (usedKeys.get(lower(b)) || [])) keysInUse.add(k);
let keyLetter = opt.key ? lower(opt.key) : null;
if (!keyLetter) {
	for (const ch of lower(nameEn + id).replace(/[^a-z0-9]/g, '')) if (!keysInUse.has(ch)) { keyLetter = ch; break; }
	if (!keyLetter) { keyLetter = lower(attr(chosen.open, 'key') || ''); if (keyLetter) notes.push('no free key found in ' + buildings.join('/') + ': using the base\'s (' + keyLetter + '). Give --key'); }
}
else if (keysInUse.has(keyLetter)) notes.push('the key "' + keyLetter + '" is already used by another command of ' + buildings.join('/') + ' (it can usually be repeated, but they clash in game)');

// --- the class --------------------------------------------------------------------------------------------------------------
const changes = [];
let xml = base.text;
const header = base.header;
let newHeader = header.replace(/(\bid\s*=\s*")[^"]*"/, '$1' + id + '"').replace(/\s+altid\s*=\s*"[^"]*"/, '');
xml = xml.replace(header, () => newHeader);
// Changes an attribute of a <properties>; if the class does not set it (it inherits it), a line is added
function setProp(name, value, label) {
	let found = false, old;
	xml = xml.replace(/<properties\b[^>]*>/gi, tag => {
		const re = new RegExp('(\\b' + name + '\\s*=\\s*")([^"]*)"');
		const m = tag.match(re);
		if (!m) return tag;
		found = true; old = m[2];
		return tag.replace(re, '$1' + value + '"');
	});
	if (found) { if (String(old) !== String(value)) changes.push(label + ' ' + old + ' -> ' + value); }
	else {
		xml = xml.replace(/<\/class\s*>/i, '\t<properties ' + name + '="' + value + '"/>\n\n</class>');
		changes.push(label + ' ' + value + ' (the base inherited it from its parent)');
	}
}
if (stats.maxhealth !== undefined) setProp('maxhealth', stats.maxhealth, 'health');
if (stats.damage !== undefined) setProp('damage', stats.damage, 'damage');
if (stats.armor !== undefined) { setProp('armor_slash', stats.armor, 'armor (slash)'); setProp('armor_pierce', stats.armor, 'armor (pierce)'); }
if (stats.speed !== undefined) setProp('speed', stats.speed, 'speed');
// Name
if (/\bdisplay_name\s*=/.test(xml)) {
	xml = xml.replace(/(\bdisplay_name\s*=\s*")[^"]*"/, '$1' + esc(key) + '"');
	xml = xml.replace(/(\bdisplay_name_plural\s*=\s*")[^"]*"/, '$1' + esc(keyPlural) + '"');
	if (!/\bdisplay_name_plural\s*=/.test(xml)) xml = xml.replace(/(\bdisplay_name\s*=\s*"[^"]*")/, '$1 display_name_plural="' + esc(keyPlural) + '"');
}
else xml = xml.replace(/<\/class\s*>/i, '\t<properties display_name="' + esc(key) + '" display_name_plural="' + esc(keyPlural) + '"/>\n\n</class>');
// Where it appears in the editor
xml = xml.replace(/(\bedittree_pos\s*=\s*"[^"]*\/)[^"\/]*"/, '$1' + esc(key.replace(/\//g, '-')) + '"');
const classComment = '<!-- New unit ' + id + ': FULL COPY of the game class ' + baseId + ', with a different id' + (changes.length ? ' and different stats (' + changes.join(', ') + ')' : '') +
	'. It is copied whole, not through parent, because the game does not inherit the entity (the model) from the parent. Made with create-unit.js. -->\n';
xml = xml.replace(/<class\b/i, () => classComment + '<class');
if (!xml.endsWith('\n')) xml += '\n';

// --- the order --------------------------------------------------------------------------------------------------------------
let open = chosen.open;
const setA = (name, value) => {
	const re = new RegExp('(\\b' + name + '\\s*=\\s*")[^"]*"');
	if (re.test(open)) open = open.replace(re, (m, a) => a + esc(String(value)) + '"');
	else open = open.replace(/\s*\/?>$/, m => '\n\t' + name + '="' + esc(String(value)) + '"' + m);
};
setA('name', chosen.name.includes(baseId) ? chosen.name.split(baseId).join(id) : trainName);
const cmdName = attr(open, 'name');
if (takenCmdNames.has(lower(cmdName))) die('a command named "' + cmdName + '" already exists. Pick another id');
if (keyLetter) setA('key', keyLetter);
if (costGold !== undefined) setA('costgold', costGold);
if (costFood !== undefined) setA('costfood', costFood);
if (execDelay !== undefined) setA('execdelay', execDelay);
setA('rollover', rollover);
// param is the class and, sometimes, more data ("RArcher,Arrows"): only the class is changed
setA('param', id + (attr(open, 'param') || '').replace(/^[^,]*/, ''));
setA('sclass', id);
if (attr(open, 'description') !== undefined) notes.push('the command of ' + baseId + ' has a description="' + attr(open, 'description') + '" that was copied as is. Change it in data\\commands\\' + id + '.xml');
open = tidyOpen(open);
function tidyOpen(o) {
	// As in the game's files: name and priority on the first line and one attribute per line after that
	const m = o.match(/^<cmd\s+([\s\S]*?)(\/?)>$/);
	const attrs = [...m[1].matchAll(/([A-Za-z_][\w-]*)\s*=\s*"([^"]*)"/g)].map(a => a[1] + '="' + a[2] + '"');
	const first = attrs.shift();
	return '<cmd ' + first + (attrs.length ? ' ' + attrs.shift() : '') + attrs.map(a => '\n\t' + a).join('') + m[2] + '>';
}
const bodyRest = chosen.text.slice(chosen.open.length).replace(/<\/cmd\s*>\s*$/i, '').replace(/<src\b[^>]*\/?>(\s*<\/src>)?/gi, '').trim();
const cmdXml = '<commands>\n\n' +
	('<!-- The command to recruit ' + id + ' in ' + buildings.join(', ') + '. It is a copy of "' + chosen.name + '" (data\\commands\\' + chosen.file + ' of the game) with a different class' +
	(costGold !== undefined || costFood !== undefined || execDelay !== undefined ? ', a different cost' : '') + ' and a different key. The game loads every .xml in data\\commands. Made with create-unit.js. -->\n') +
	open + '\n' + (bodyRest ? '\t' + bodyRest.replace(/\n\s*/g, '\n\t') + '\n' : '') +
	buildings.map(b => '\t<src obj="' + esc(b) + '"/>\n').join('') + '</cmd>\n\n</commands>\n';

// --- the texts ---------------------------------------------------------------------------------------------------------------
const langs = [{ lang: 'english', name: nameEn, plural: keyPlural, roll: rollover }];
if (nameEs) langs.push({ lang: 'spanish', name: nameEs, plural: opt['plural-es'] || pluralEs(nameEs), roll: verbEs + ' ' + lowerFirst(nameEs) });
if (nameIt) {
	langs.push({ lang: 'italian', name: nameIt, plural: nameIt, roll: verbIt + ' ' + lowerFirst(nameIt) });
	notes.push('the Italian plural of "' + nameIt + '" was set equal to the singular; fix it in loc\\italian.loc.xml');
}
const locEntries = L => [
	[key + '@' + id + ' class name', L.name],
	[keyPlural + '@' + id + ' class name plural', L.plural],
	[rollover + '@' + cmdName, L.roll],
];

// --- everything is prepared and written at once ------------------------------------------------------------------------------
const outputs = new Map();   // path -> contents
const classFile = path.join(modDir, 'data', 'classes', id + '.sc.xml');
const cmdFile = path.join(modDir, 'data', 'commands', id + '.xml');
for (const f of [classFile, cmdFile]) if (fs.existsSync(f)) die(f + ' already exists. Not overwriting it: delete it or pick another id');
outputs.set(classFile, xml);
outputs.set(cmdFile, cmdXml);
for (const L of langs) {
	const f = path.join(modDir, 'loc', L.lang + '.loc.xml');
	const lines = locEntries(L).map(([t, r]) => [t, '\t<translationtableentry text="' + esc(t) + '" result="' + esc(r) + '"/>']);
	if (fs.existsSync(f)) {
		let cur = fs.readFileSync(f, 'utf8').replace(/^﻿/, '');
		const fresh = lines.filter(([t]) => !cur.includes('text="' + esc(t) + '"'));
		if (fresh.length) {
			if (!/<\/translationtable>/i.test(cur)) die(f + ' has no </translationtable>: cannot tell where to add the texts');
			const add = '\t<!-- ' + id + ' -->\n' + fresh.map(l => l[1]).join('\n') + '\n';
			cur = cur.replace(/(\n?)([ \t]*)<\/translationtable>(?![\s\S]*<\/translationtable>)/i, (m, nl) => '\n' + add + '</translationtable>');
			outputs.set(f, cur);
		}
	}
	else outputs.set(f, '<translationtable>\n\t' + '<!-- The class name: "<display_name>@<class id> class name", its plural, and the recruit button text (the rollover of the command, with the command name as context) -->' + '\n' +
		lines.map(l => l[1]).join('\n') + '\n</translationtable>\n');
}
const modJson = path.join(modDir, 'mod.json');
if (!fs.existsSync(modJson)) {
	const mid = path.basename(path.resolve(modDir)).toLowerCase().replace(/[^a-z0-9_-]+/g, '-').replace(/^-+|-+$/g, '') || 'my-mod';
	outputs.set(modJson, JSON.stringify({
		id: mid, name: path.basename(path.resolve(modDir)), version: '1.0', category: 'Units & Heroes', tags: [], authors: [],
		description: 'New unit ' + nameEn + '. It changes gameplay: in multiplayer everyone must have it.', redistributable: false,
	}, null, 2) + '\n');
}

const written = [], backups = new Map();
try {
	for (const [f, content] of outputs) {
		fs.mkdirSync(path.dirname(f), { recursive: true });
		backups.set(f, fs.existsSync(f) ? fs.readFileSync(f) : null);
		fs.writeFileSync(f, content, 'utf8');
		written.push(f);
	}
}
catch (e) {
	for (const f of written) { const b = backups.get(f); if (b === null) fs.rmSync(f, { force: true }); else fs.writeFileSync(f, b); }
	die('could not write ' + e.path + ' (' + e.message + '). Everything written was undone');
}

// --- report ------------------------------------------------------------------------------------------------------------------
console.log('unit ' + id + ' (copy of ' + baseId + (baseDisplay ? ', "' + baseDisplay + '"' : '') + ', recruited in ' + buildings.join(', ') + ')');
for (const f of written) console.log('  ' + path.relative(modDir, f));
console.log('  changes to the class: ' + (changes.length ? changes.join('; ') : 'none (same stats as the base)'));
console.log('  command ' + cmdName + ': key ' + (keyLetter || '(the base\'s)') + ', ' + (costGold !== undefined ? costGold : attr(chosen.open, 'costgold') || '?') + ' gold');
for (const n of notes) console.log('  warning: ' + n);
console.log('To do: the portrait and the button are those of ' + baseId + ' (make your own with convert-image.js if you want), and the AI will not recruit the unit. Run the mod through validate-mod.js --data <DATA>.');
