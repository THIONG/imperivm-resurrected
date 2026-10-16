// Crea un objeto nuevo para los héroes: el parche de items.xml y, si quieres, el comportamiento que lo da a cada héroe (SDK de Imperivm Resurrected).
//
//   node crear-objeto.js <carpeta del mod> <id> [opciones]
//
// Opciones:
//   --vida N  --dano N  --armadura N  --nivel N  --experiencia N   lo que suma a quien lo lleva (--armadura pone armor_slash y armor_pierce)
//   --icono <ruta>          un icono del juego, por ejemplo "gameres/icons/items/Belt of might.bmp" (por defecto el de los dientes de oso)
//   --descripcion "Texto"   lo que se lee en la ventana del objeto (sin él, se genera en inglés: "Adds 10 to damage")
//   --dar-heroes            hace que cada héroe reciba el objeto al aparecer (comportamiento en la clase Hero + script)
//   --datos <carpeta DATA>  (opcional) comprueba que el id no lo use ya un objeto del juego
//
// Escribe en la carpeta del mod (nada, si algo falla):
//   patches\data\items.xml                   el objeto; si el parche ya existe, se AÑADE sin tocar los que hay
//   patches\data\classes\Hero.sc.xml         con --dar-heroes: un comportamiento más para la clase Hero (se añade si el parche ya existe)
//   data\mod\dar_<id>.vs                     con --dar-heroes: el script que da el objeto si el héroe aún no lo tiene
//   mod.json                                 solo si el mod no tiene uno
const fs = require('fs'), path = require('path');
const { T } = require('./lib/lang');

const HELP = T('uso: node crear-objeto.js <carpeta del mod> <id> [--vida N --dano N --armadura N --nivel N --experiencia N\n' +
	'         --icono <ruta> --descripcion "Texto" --dar-heroes --datos <carpeta DATA>]',
	'usage: node crear-objeto.js <mod folder> <id> [--vida N --dano N --armadura N --nivel N --experiencia N\n' +
	'         --icono <path> --descripcion "Text" --dar-heroes --datos <DATA folder>]',
	'uso: node crear-objeto.js <cartella della mod> <id> [--vida N --dano N --armadura N --nivel N --experiencia N\n' +
	'         --icono <percorso> --descripcion "Testo" --dar-heroes --datos <cartella DATA>]');
const die = msg => { console.error(T('error: ', 'error: ', 'errore: ') + msg); process.exit(1); };

// --- argumentos -------------------------------------------------------------------------------------------------------
const OPTS = ['vida', 'dano', 'armadura', 'nivel', 'experiencia', 'icono', 'descripcion', 'datos'];
const FLAGS = ['dar-heroes'];
const pos = [], opt = {};
{
	const a = process.argv.slice(2);
	for (let i = 0; i < a.length; i++) {
		if (!a[i].startsWith('--')) { pos.push(a[i]); continue; }
		let [name, value] = a[i].slice(2).split(/=(.*)/s);
		if (name === 'ayuda' || name === 'help') { console.log(HELP); process.exit(0); }
		if (FLAGS.includes(name)) { opt[name] = true; continue; }
		if (!OPTS.includes(name)) die(T('opción desconocida: --' + name, 'unknown option: --' + name, 'opzione sconosciuta: --' + name) + '\n' + HELP);
		if (value === undefined) { value = a[++i]; if (value === undefined || value.startsWith('--')) die(T('a --' + name + ' le falta el valor', '--' + name + ' is missing its value', 'a --' + name + ' manca il valore')); }
		opt[name] = value;
	}
}
if (pos.length !== 2) { console.error(HELP); process.exit(1); }
const [modDir, id] = pos;
// El id es el texto con el que los scripts piden el objeto: AddItem("id"). Los del juego llevan espacios ("Bear teeth amulet")
if (!id.trim() || id !== id.trim() || id.length > 60 || /["\\<>&\r\n\t]/.test(id)) {
	die(T('id no válido: "' + id + '" (hasta 60 caracteres, sin comillas, barras, < > & ni espacios al principio o al final)', 'invalid id: "' + id + '" (up to 60 characters, no quotes, slashes, < > & or leading/trailing spaces)', 'id non valido: "' + id + '" (fino a 60 caratteri, senza virgolette, barre, < > & né spazi all\'inizio o alla fine)'));
}
const num = name => {
	if (opt[name] === undefined) return 0;
	if (!/^-?\d+$/.test(opt[name])) die(T('--' + name + ' tiene que ser un número entero (es "' + opt[name] + '")', '--' + name + ' must be an integer (got "' + opt[name] + '")', '--' + name + ' deve essere un numero intero (è "' + opt[name] + '")'));
	return Number(opt[name]);
};
const bonus = { health: num('vida'), damage: num('dano'), armor: num('armadura'), level: num('nivel'), experience: num('experiencia') };
const esc = s => s.replace(/&/g, '&amp;').replace(/"/g, '&quot;').replace(/</g, '&lt;').replace(/>/g, '&gt;');
const icon = (opt.icono || 'gameres/icons/items/Bear Teeth.bmp').replace(/\\/g, '/');
if (opt.icono !== undefined && (!icon.trim() || /["<>&]/.test(icon))) die(T('--icono no vale: "' + opt.icono + '"', '--icono is not valid: "' + opt.icono + '"', '--icono non è valido: "' + opt.icono + '"'));
const notes = [];
if (!/\.bmp$/i.test(icon)) notes.push(T('el icono "' + icon + '" no acaba en .bmp: el juego usa BMP', 'the icon "' + icon + '" does not end in .bmp: the game uses BMP', 'l\'icona "' + icon + '" non finisce in .bmp: il gioco usa BMP'));
if (opt.descripcion !== undefined && !opt.descripcion.trim()) die(T('--descripcion está vacía', '--descripcion is empty', '--descripcion è vuota'));

// --- el objeto --------------------------------------------------------------------------------------------------------------
const parts = [];
if (bonus.health) parts.push(bonus.health + ' to health');
if (bonus.damage) parts.push(bonus.damage + ' to damage');
if (bonus.armor) parts.push(bonus.armor + ' to armor');
if (bonus.level) parts.push(bonus.level + ' to level');
if (bonus.experience) parts.push(bonus.experience + ' to experience');
if (!parts.length) notes.push(T('el objeto no suma nada: pon --vida, --dano, --armadura, --nivel o --experiencia (o edita el <bonus>)', 'the item adds nothing: give --vida, --dano, --armadura, --nivel or --experiencia (or edit the <bonus>)', 'l\'oggetto non aggiunge niente: metti --vida, --dano, --armadura, --nivel o --experiencia (o modifica il <bonus>)'));
const autoDescription = parts.length ? 'Adds ' + (parts.length > 1 ? parts.slice(0, -1).join(', ') + ' and ' + parts[parts.length - 1] : parts[0]) : id;
const description = opt.descripcion !== undefined ? opt.descripcion : autoDescription;
const itemXml =
	'  ' + T('<!-- Objeto nuevo "' + id.replace(/--/g, '- -') + '". El icono es uno del juego; con tu propio icono, ponlo en gameres/icons/items/ dentro del mod y cambia la ruta. -->',
		'<!-- New item "' + id.replace(/--/g, '- -') + '". The icon is one from the game; for your own icon, put it in gameres/icons/items/ inside the mod and change the path. -->',
		'<!-- Nuovo oggetto "' + id.replace(/--/g, '- -') + '". L\'icona è una del gioco; per un\'icona tua, mettila in gameres/icons/items/ dentro la mod e cambia il percorso. -->') + '\n' +
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

// Se añade a un parche que ya existe, o se crea uno
const outputs = new Map();   // ruta -> contenido
const readText = f => fs.readFileSync(f, 'utf8').replace(/^﻿/, '').replace(/\r\n/g, '\n');
function appendToPatch(file, block, what, already) {
	if (!fs.existsSync(file)) return '<patch>\n' + block + '</patch>\n';
	const cur = readText(file);
	if (!/^\s*(<\?[^>]*\?>\s*)?(<!--[\s\S]*?-->\s*)*<patch\b/.test(cur) || !/<\/patch\s*>\s*$/.test(cur)) {
		die(T(file + ' existe, pero no es un parche válido (tiene que empezar por <patch> y acabar en </patch>). No lo toco: arréglalo o muévelo', file + ' exists, but it is not a valid patch (it must start with <patch> and end with </patch>). Not touching it: fix it or move it', file + ' esiste, ma non è una patch valida (deve iniziare con <patch> e finire con </patch>). Non lo tocco: correggilo o spostalo'));
	}
	if (already && already(cur)) return null;
	return cur.replace(/<\/patch\s*>\s*$/, m => (cur.slice(0, cur.length - m.length).endsWith('\n') ? '' : '\n') + block + '</patch>\n');
}

const itemsFile = path.join(modDir, 'patches', 'data', 'items.xml');
if (fs.existsSync(itemsFile)) {
	const cur = readText(itemsFile);
	for (const m of cur.replace(/<!--[\s\S]*?-->/g, '').matchAll(/<item\b[^>]*?\bid\s*=\s*"([^"]*)"/g)) {
		if (m[1].toLowerCase() === esc(id).toLowerCase()) die(T('tu parche ' + itemsFile + ' ya tiene un objeto "' + id + '". Elige otro id', 'your patch ' + itemsFile + ' already has an item "' + id + '". Pick another id', 'la tua patch ' + itemsFile + ' ha già un oggetto "' + id + '". Scegli un altro id'));
	}
}
if (opt.datos) {
	if (!fs.existsSync(opt.datos)) die(T('no existe la carpeta DATA: ' + opt.datos, 'the DATA folder does not exist: ' + opt.datos, 'la cartella DATA non esiste: ' + opt.datos));
	const itemsGame = fs.readdirSync(opt.datos).find(f => f.toLowerCase() === 'items.xml');
	if (!itemsGame) die(T(opt.datos + ' no parece la carpeta DATA del data.pak (falta items.xml). Sácala con extraer-datos.js', opt.datos + ' does not look like the DATA folder of data.pak (items.xml is missing). Extract it with extraer-datos.js', opt.datos + ' non sembra la cartella DATA di data.pak (manca items.xml). Estraila con extraer-datos.js'));
	for (const m of readText(path.join(opt.datos, itemsGame)).matchAll(/<item\b[^>]*?\bid\s*=\s*"([^"]*)"/g)) {
		if (m[1].toLowerCase() === esc(id).toLowerCase()) die(T('el juego ya tiene un objeto "' + m[1] + '". Elige otro id', 'the game already has an item "' + m[1] + '". Pick another id', 'il gioco ha già un oggetto "' + m[1] + '". Scegli un altro id'));
	}
}
else notes.push(T('no he comprobado que el juego no tenga ya un objeto con ese id (pasa --datos <carpeta DATA>)', 'did not check whether the game already has an item with that id (give --datos <DATA folder>)', 'non ho verificato che il gioco non abbia già un oggetto con quell\'id (passa --datos <cartella DATA>)'));
outputs.set(itemsFile, appendToPatch(itemsFile, itemXml));

// --- darlo a los héroes ----------------------------------------------------------------------------------------------------
let scriptRel = null;
if (opt['dar-heroes']) {
	const slug = id.normalize('NFD').replace(/[\u0300-\u036f]/g, '').toLowerCase().replace(/[^a-z0-9]+/g, '_').replace(/^_+|_+$/g, '') || 'objeto';
	scriptRel = 'data/mod/dar_' + slug + '.vs';
	const scriptFile = path.join(modDir, 'data', 'mod', 'dar_' + slug + '.vs');
	if (fs.existsSync(scriptFile)) die(T('ya existe ' + scriptFile + ' (otro objeto con un nombre parecido, o ya lo creaste). No lo piso', scriptFile + ' already exists (another item with a similar name, or you already created it). Not overwriting it', scriptFile + ' esiste già (un altro oggetto con un nome simile, o l\'hai già creato). Non lo sovrascrivo'));
	outputs.set(scriptFile,
		'// void, Obj This\n\n' +
		T('// Se espera un poco, como hacen los scripts del juego, para no ejecutarse todos en el mismo instante\n', '// Wait a little, as the game scripts do, so they do not all run at the same instant\n', '// Si aspetta un po\', come fanno gli script del gioco, per non eseguirsi tutti nello stesso istante\n') +
		'Sleep(rand(500) + 100);\n\n' +
		T('// Se le da el objeto al héroe si aún no lo tiene (al cargar una partida guardada el script vuelve a empezar: así no se duplica)\n', '// The hero is given the item if it does not have it yet (when a saved game is loaded the script starts again: this avoids duplicates)\n', '// All\'eroe si dà l\'oggetto se non ce l\'ha ancora (caricando una partita salvata lo script riparte: così non si duplica)\n') +
		'if (!This.FindItem("' + id + '").IsValid) {\n' +
		'\tif (This.AddItem("' + id + '")) Mod_Log("' + T('Un heroe ha recibido ', 'A hero received ', 'Un eroe ha ricevuto ') + id + '");\n' +
		'\telse Mod_Log("' + T('El heroe no ha podido recibir ' + id + ' (sin hueco libre)', 'The hero could not receive ' + id + ' (no free slot)', 'L\'eroe non ha potuto ricevere ' + id + ' (nessuno spazio libero)') + '");\n' +
		'}\n\n' +
		T('// Un script de comportamiento no debe terminar\n', '// A behavior script must not end\n', '// Uno script di comportamento non deve terminare\n') +
		'while(true) Sleep(50000);\n');
	const heroFile = path.join(modDir, 'patches', 'data', 'classes', 'Hero.sc.xml');
	const block = '  ' + T('<!-- Un comportamiento más para todos los héroes: se les da el objeto "' + id.replace(/--/g, '- -') + '" al aparecer -->',
		'<!-- One more behavior for all heroes: they are given the item "' + id.replace(/--/g, '- -') + '" when they appear -->',
		'<!-- Un comportamento in più per tutti gli eroi: ricevono l\'oggetto "' + id.replace(/--/g, '- -') + '" quando compaiono -->') + '\n' +
		'  <add select="/class">\n    <behavior script="' + scriptRel + '"/>\n  </add>\n';
	const hero = appendToPatch(heroFile, block, 'Hero', cur => cur.includes('"' + scriptRel + '"'));
	if (hero !== null) outputs.set(heroFile, hero);
}

const modJson = path.join(modDir, 'mod.json');
if (!fs.existsSync(modJson)) {
	const dir = path.basename(path.resolve(modDir));
	outputs.set(modJson, JSON.stringify({
		id: dir.toLowerCase().replace(/[^a-z0-9_-]+/g, '-').replace(/^-+|-+$/g, '') || 'mi-mod', name: dir, version: '1.0', category: 'Gameplay & Balance',
		tags: ['Gameplay'], authors: [],
		description: T('Objeto nuevo ' + id + '. Cambia la jugabilidad: en multijugador todos tienen que llevarlo.', 'New item ' + id + '. It changes gameplay: in multiplayer everyone must have it.', 'Nuovo oggetto ' + id + '. Cambia il gameplay: in multigiocatore tutti devono averlo.'), redistribuible: false,
	}, null, 2) + '\n');
}

// --- se escribe de una vez --------------------------------------------------------------------------------------------------
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
	die(T('no he podido escribir ' + e.path + ' (' + e.message + '). He deshecho lo escrito', 'could not write ' + e.path + ' (' + e.message + '). Everything written was undone', 'impossibile scrivere ' + e.path + ' (' + e.message + '). Ho annullato quanto scritto'));
}

// --- informe ----------------------------------------------------------------------------------------------------------------
console.log(T('objeto "' + id + '": ' + (parts.length ? parts.join(', ') : 'sin bonos'), 'item "' + id + '": ' + (parts.length ? parts.join(', ') : 'no bonuses'), 'oggetto "' + id + '": ' + (parts.length ? parts.join(', ') : 'senza bonus')));
for (const f of written) console.log('  ' + path.relative(modDir, f) + (backups.get(f) !== null ? T(' (se ha añadido al que había)', ' (added to the existing one)', ' (aggiunto a quello esistente)') : ''));
if (!opt['dar-heroes']) console.log(T('  no se da a nadie: usa --dar-heroes, o un script con Obj::AddItem("' + id + '")', '  it is not given to anyone: use --dar-heroes, or a script with Obj::AddItem("' + id + '")', '  non viene dato a nessuno: usa --dar-heroes, o uno script con Obj::AddItem("' + id + '")'));
for (const n of notes) console.log(T('  aviso: ', '  warning: ', '  avviso: ') + n);
console.log(T('Falta: el icono es ' + icon + ' (con uno propio, ponlo en gameres/icons/items/ del mod). Pasa el mod por validar-mod.js --datos <DATA>.', 'To do: the icon is ' + icon + ' (for your own, put it in gameres/icons/items/ of the mod). Run the mod through validar-mod.js --datos <DATA>.', 'Da fare: l\'icona è ' + icon + ' (per una tua, mettila in gameres/icons/items/ della mod). Passa la mod per validar-mod.js --datos <DATA>.'));
