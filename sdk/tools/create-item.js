// Creates a new item for the heroes: the items.xml patch and, optionally, the behavior that gives it to each hero (Imperivm Resurrected SDK).
//
//   node create-item.js <mod folder> <id> [options]
//
// Options:
//   --health N  --damage N  --armor N  --level N  --experience N   what it adds to whoever carries it (--armor sets armor_slash and armor_pierce)
//   --icon <path>          a game icon, for example "gameres/icons/items/Belt of might.bmp" (by default, the bear teeth amulet's)
//   --description "Text"   what is read in the item's window (without it, one is generated in English: "Adds 10 to damage")
//   --give-heroes            makes each hero receive the item when it appears (behavior in the Hero class + script)
//   --data <DATA folder>  (optional) checks that no game item already uses the id
//
// Writes to the mod folder (nothing, if something fails):
//   patches\data\items.xml                   the item; if the patch already exists, the item is ADDED without touching the existing ones
//   patches\data\classes\Hero.sc.xml         with --give-heroes: one more behavior for the Hero class (added if the patch already exists)
//   data\mod\dar_<id>.vs                     with --give-heroes: the script that gives the item if the hero does not have it yet
//   mod.json                                 only if the mod does not have one
const fs = require('fs'), path = require('path');

const HELP = 'usage: node create-item.js <mod folder> <id> [--health N --damage N --armor N --level N --experience N\n' +
	'         --icon <path> --description "Text" --give-heroes --data <DATA folder>]';
const die = msg => { console.error('error: ' + msg); process.exit(1); };

// --- arguments --------------------------------------------------------------------------------------------------------
const OPTS = ['health', 'damage', 'armor', 'level', 'experience', 'icon', 'description', 'data'];
const FLAGS = ['give-heroes'];
const pos = [], opt = {};
{
	const a = process.argv.slice(2);
	for (let i = 0; i < a.length; i++) {
		if (!a[i].startsWith('--')) { pos.push(a[i]); continue; }
		let [name, value] = a[i].slice(2).split(/=(.*)/s);
		if (name === 'ayuda' || name === 'help') { console.log(HELP); process.exit(0); }
		if (FLAGS.includes(name)) { opt[name] = true; continue; }
		if (!OPTS.includes(name)) die('unknown option: --' + name + '\n' + HELP);
		if (value === undefined) { value = a[++i]; if (value === undefined || value.startsWith('--')) die('--' + name + ' is missing its value'); }
		opt[name] = value;
	}
}
if (pos.length !== 2) { console.error(HELP); process.exit(1); }
const [modDir, id] = pos;
// The id is the text scripts use to ask for the item: AddItem("id"). The game's items have spaces ("Bear teeth amulet")
if (!id.trim() || id !== id.trim() || id.length > 60 || /["\\<>&\r\n\t]/.test(id)) {
	die('invalid id: "' + id + '" (up to 60 characters, no quotes, slashes, < > & or leading/trailing spaces)');
}
const num = name => {
	if (opt[name] === undefined) return 0;
	if (!/^-?\d+$/.test(opt[name])) die('--' + name + ' must be an integer (got "' + opt[name] + '")');
	return Number(opt[name]);
};
const bonus = { health: num('health'), damage: num('damage'), armor: num('armor'), level: num('level'), experience: num('experience') };
const esc = s => s.replace(/&/g, '&amp;').replace(/"/g, '&quot;').replace(/</g, '&lt;').replace(/>/g, '&gt;');
const icon = (opt.icon || 'gameres/icons/items/Bear Teeth.bmp').replace(/\\/g, '/');
if (opt.icon !== undefined && (!icon.trim() || /["<>&]/.test(icon))) die('--icon is not valid: "' + opt.icon + '"');
const notes = [];
if (!/\.bmp$/i.test(icon)) notes.push('the icon "' + icon + '" does not end in .bmp: the game uses BMP');
if (opt.description !== undefined && !opt.description.trim()) die('--description is empty');

// --- the item ---------------------------------------------------------------------------------------------------------------
const parts = [];
if (bonus.health) parts.push(bonus.health + ' to health');
if (bonus.damage) parts.push(bonus.damage + ' to damage');
if (bonus.armor) parts.push(bonus.armor + ' to armor');
if (bonus.level) parts.push(bonus.level + ' to level');
if (bonus.experience) parts.push(bonus.experience + ' to experience');
if (!parts.length) notes.push('the item adds nothing: give --health, --damage, --armor, --level or --experience (or edit the <bonus>)');
const autoDescription = parts.length ? 'Adds ' + (parts.length > 1 ? parts.slice(0, -1).join(', ') + ' and ' + parts[parts.length - 1] : parts[0]) : id;
const description = opt.description !== undefined ? opt.description : autoDescription;
const itemXml =
	'  ' + ('<!-- New item "' + id.replace(/--/g, '- -') + '". The icon is one from the game; for your own icon, put it in gameres/icons/items/ inside the mod and change the path. -->') + '\n' +
	'  <add select="/items">\n' +
	'    <item id="' + esc(id) + '"\n' +
	'          image="' + esc(icon) + '"\n' +
	'          sound="data/sound entities/ItemUse.xml"\n' +
	'          level="0"\n' +
	'          name="' + esc(id) + '"\n' +
	'          description="' + esc(description) + '"\n' +
	'          important="no">\n' +
	'      <bonus health="' + bonus.health + '" damage="' + bonus.damage + '" armor_slash="' + bonus.armor + '" armor_pierce="' + bonus.armor +
	'" level="' + bonus.level + '" experience="' + bonus.experience + '"/>\n' +
	'    </item>\n' +
	'  </add>\n';

// It is added to a patch that already exists, or a new one is created
const outputs = new Map();   // path -> contents
const readText = f => fs.readFileSync(f, 'utf8').replace(/^﻿/, '').replace(/\r\n/g, '\n');
function appendToPatch(file, block, what, already) {
	if (!fs.existsSync(file)) return '<patch>\n' + block + '</patch>\n';
	const cur = readText(file);
	if (!/^\s*(<\?[^>]*\?>\s*)?(<!--[\s\S]*?-->\s*)*<patch\b/.test(cur) || !/<\/patch\s*>\s*$/.test(cur)) {
		die(file + ' exists, but it is not a valid patch (it must start with <patch> and end with </patch>). Not touching it: fix it or move it');
	}
	if (already && already(cur)) return null;
	return cur.replace(/<\/patch\s*>\s*$/, m => (cur.slice(0, cur.length - m.length).endsWith('\n') ? '' : '\n') + block + '</patch>\n');
}

const itemsFile = path.join(modDir, 'patches', 'data', 'items.xml');
if (fs.existsSync(itemsFile)) {
	const cur = readText(itemsFile);
	for (const m of cur.replace(/<!--[\s\S]*?-->/g, '').matchAll(/<item\b[^>]*?\bid\s*=\s*"([^"]*)"/g)) {
		if (m[1].toLowerCase() === esc(id).toLowerCase()) die('your patch ' + itemsFile + ' already has an item "' + id + '". Pick another id');
	}
}
if (opt.data) {
	if (!fs.existsSync(opt.data)) die('the DATA folder does not exist: ' + opt.data);
	const itemsGame = fs.readdirSync(opt.data).find(f => f.toLowerCase() === 'items.xml');
	if (!itemsGame) die(opt.data + ' does not look like the DATA folder of data.pak (items.xml is missing). Extract it with extract-data.js');
	for (const m of readText(path.join(opt.data, itemsGame)).matchAll(/<item\b[^>]*?\bid\s*=\s*"([^"]*)"/g)) {
		if (m[1].toLowerCase() === esc(id).toLowerCase()) die('the game already has an item "' + m[1] + '". Pick another id');
	}
}
else notes.push('did not check whether the game already has an item with that id (give --data <DATA folder>)');
outputs.set(itemsFile, appendToPatch(itemsFile, itemXml));

// --- giving it to the heroes -----------------------------------------------------------------------------------------------
let scriptRel = null;
if (opt['give-heroes']) {
	const slug = id.normalize('NFD').replace(/[\u0300-\u036f]/g, '').toLowerCase().replace(/[^a-z0-9]+/g, '_').replace(/^_+|_+$/g, '') || 'objeto';
	scriptRel = 'data/mod/dar_' + slug + '.vs';
	const scriptFile = path.join(modDir, 'data', 'mod', 'dar_' + slug + '.vs');
	if (fs.existsSync(scriptFile)) die(scriptFile + ' already exists (another item with a similar name, or you already created it). Not overwriting it');
	outputs.set(scriptFile,
		'// void, Obj This\n\n' +
		'// Wait a little, as the game scripts do, so they do not all run at the same instant\n' +
		'Sleep(rand(500) + 100);\n\n' +
		'// The hero is given the item if it does not have it yet (when a saved game is loaded the script starts again: this avoids duplicates)\n' +
		'if (!This.FindItem("' + id + '").IsValid) {\n' +
		'\tif (This.AddItem("' + id + '")) Mod_Log("' + 'A hero received ' + id + '");\n' +
		'\telse Mod_Log("' + ('The hero could not receive ' + id + ' (no free slot)') + '");\n' +
		'}\n\n' +
		'// A behavior script must not end\n' +
		'while(true) Sleep(50000);\n');
	const heroFile = path.join(modDir, 'patches', 'data', 'classes', 'Hero.sc.xml');
	const block = '  ' + ('<!-- One more behavior for all heroes: they are given the item "' + id.replace(/--/g, '- -') + '" when they appear -->') + '\n' +
		'  <add select="/class">\n    <behavior script="' + scriptRel + '"/>\n  </add>\n';
	const hero = appendToPatch(heroFile, block, 'Hero', cur => cur.includes('"' + scriptRel + '"'));
	if (hero !== null) outputs.set(heroFile, hero);
}

const modJson = path.join(modDir, 'mod.json');
if (!fs.existsSync(modJson)) {
	const dir = path.basename(path.resolve(modDir));
	outputs.set(modJson, JSON.stringify({
		id: dir.toLowerCase().replace(/[^a-z0-9_-]+/g, '-').replace(/^-+|-+$/g, '') || 'my-mod', name: dir, version: '1.0', category: 'Gameplay & Balance',
		tags: [], authors: [],
		description: 'New item ' + id + '. It changes gameplay: in multiplayer everyone must have it.', redistributable: false,
	}, null, 2) + '\n');
}

// --- written all at once ----------------------------------------------------------------------------------------------------
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

// --- report -----------------------------------------------------------------------------------------------------------------
console.log('item "' + id + '": ' + (parts.length ? parts.join(', ') : 'no bonuses'));
for (const f of written) console.log('  ' + path.relative(modDir, f) + (backups.get(f) !== null ? ' (added to the existing one)' : ''));
if (!opt['give-heroes']) console.log('  it is not given to anyone: use --give-heroes, or a script with Obj::AddItem("' + id + '")');
for (const n of notes) console.log('  warning: ' + n);
console.log('To do: the icon is ' + icon + ' (for your own, put it in gameres/icons/items/ of the mod). Run the mod through validate-mod.js --data <DATA>.');
