// Crea un'unità nuova a partire da una del gioco: la sua classe, l'ordine per reclutarla e i suoi testi (SDK di Imperivm Resurrected).
//
//   node crea-unita.js <cartella DATA del data.pak estratto> <cartella della mod> <id> --base <classe del gioco> [opzioni]
//
// Opzioni:
//   --vita N  --danno N  --armatura N  --velocita N   statistiche (senza di esse, quelle della base). --armatura imposta armor_slash e armor_pierce
//   --edificio <classe>                                dove si recluta (senza, dove si recluta la base)
//   --costo-oro N  --costo-cibo N  --tempo N        prezzo e tempo di reclutamento (millisecondi)
//   --tasto x                                          il tasto del pulsante (senza, uno libero in quell'edificio)
//   --nome "Testo"  --plurale "Testi"                 nome in spagnolo
//   --nome-en "Text"  --plurale-en "Texts"            nome in inglese (scrive anche loc\english.loc.xml)
//   --nome-it "Testo"                                nome in italiano (scrive anche loc\italian.loc.xml)
//
// Scrive nella cartella della mod (niente, se qualcosa fallisce):
//   data\classes\<id>.sc.xml            copia INTERA della classe base, con un altro id e le statistiche richieste
//   data\commands\<id>.xml              l'ordine per reclutarla: copia di quello della base, con un'altra classe, costo e tasto
//   loc\spanish.loc.xml (e gli altri)   i testi; se il file esiste già, si AGGIUNGONO le voci
//   mod.json                            solo se la mod non ne ha una
const fs = require('fs'), path = require('path');
const { T } = require('./lib/lang');

const HELP = T('uso: node crea-unita.js <DATA> <carpeta del mod> <id> --base <clase del juego> [--vida N --dano N --armadura N --velocidad N\n' +
	'         --edificio <clase> --coste-oro N --coste-comida N --tiempo N --tecla x --nombre "Texto" --plural "Textos"\n' +
	'         --nombre-en "Text" --plural-en "Texts" --nombre-it "Testo"]',
	'usage: node crea-unita.js <DATA> <mod folder> <id> --base <game class> [--vida N --dano N --armadura N --velocidad N\n' +
	'         --edificio <class> --coste-oro N --coste-comida N --tiempo N --tecla x --nombre "Text" --plural "Texts"\n' +
	'         --nombre-en "Text" --plural-en "Texts" --nombre-it "Testo"]',
	'uso: node crea-unita.js <DATA> <cartella della mod> <id> --base <classe del gioco> [--vida N --dano N --armadura N --velocidad N\n' +
	'         --edificio <classe> --coste-oro N --coste-comida N --tiempo N --tecla x --nombre "Testo" --plural "Testi"\n' +
	'         --nombre-en "Text" --plural-en "Texts" --nombre-it "Testo"]');
const die = msg => { console.error(T('error: ', 'error: ', 'errore: ') + msg); process.exit(1); };

// --- argomenti --------------------------------------------------------------------------------------------------------
const OPTS = ['base', 'vida', 'dano', 'armadura', 'velocidad', 'edificio', 'coste-oro', 'coste-comida', 'tiempo', 'tecla',
	'nombre', 'plural', 'nombre-en', 'plural-en', 'nombre-it'];
const pos = [], opt = {};
{
	const a = process.argv.slice(2);
	for (let i = 0; i < a.length; i++) {
		if (!a[i].startsWith('--')) { pos.push(a[i]); continue; }
		let [name, value] = a[i].slice(2).split(/=(.*)/s);
		if (name === 'ayuda' || name === 'help') { console.log(HELP); process.exit(0); }
		if (!OPTS.includes(name)) die(T('opción desconocida: --' + name, 'unknown option: --' + name, 'opzione sconosciuta: --' + name) + '\n' + HELP);
		if (value === undefined) { value = a[++i]; if (value === undefined || value.startsWith('--')) die(T('a --' + name + ' le falta el valor', '--' + name + ' is missing its value', 'a --' + name + ' manca il valore')); }
		opt[name] = value;
	}
}
if (pos.length !== 3) { console.error(HELP); process.exit(1); }
const [dataDir, modDir, id] = pos;
if (!opt.base) die(T('falta --base <clase del juego> (la unidad que se copia, por ejemplo RHastatus)', 'missing --base <game class> (the unit to copy, for example RHastatus)', 'manca --base <classe del gioco> (l\'unità da copiare, per esempio RHastatus)'));
if (!/^[A-Za-z][A-Za-z0-9_]{0,23}$/.test(id)) die(T('id no válido: "' + id + '" (letras, números y _; empieza por letra; hasta 24)', 'invalid id: "' + id + '" (letters, digits and _; must start with a letter; up to 24)', 'id non valido: "' + id + '" (lettere, numeri e _; inizia con una lettera; fino a 24)'));
const num = (name, min) => {
	if (opt[name] === undefined) return undefined;
	if (!/^-?\d+$/.test(opt[name]) || Number(opt[name]) < min) die(T('--' + name + ' tiene que ser un número entero de ' + min + ' o más (es "' + opt[name] + '")', '--' + name + ' must be an integer of ' + min + ' or more (got "' + opt[name] + '")', '--' + name + ' deve essere un numero intero di ' + min + ' o più (è "' + opt[name] + '")'));
	return Number(opt[name]);
};
const stats = { maxhealth: num('vida', 1), damage: num('dano', 0), armor: num('armadura', 0), speed: num('velocidad', 1) };
const costGold = num('coste-oro', 0), costFood = num('coste-comida', 0), execDelay = num('tiempo', 0);
if (opt.tecla !== undefined && !/^[A-Za-z0-9]$/.test(opt.tecla)) die(T('--tecla es una sola letra o número (es "' + opt.tecla + '")', '--tecla must be a single letter or digit (got "' + opt.tecla + '")', '--tecla è una sola lettera o cifra (è "' + opt.tecla + '")'));
for (const k of ['nombre', 'plural', 'nombre-en', 'plural-en', 'nombre-it']) {
	if (opt[k] !== undefined && (!opt[k].trim() || /[\r\n]/.test(opt[k]) || opt[k].includes('@'))) die(T('--' + k + ' no puede estar vacío ni llevar @ ni saltos de línea', '--' + k + ' cannot be empty or contain @ or line breaks', '--' + k + ' non può essere vuoto né contenere @ o a capo'));
}

// --- lettura dei dati del gioco ----------------------------------------------------------------------------------------
const lower = s => s.toLowerCase();
const ci = (dir, n) => { const f = fs.readdirSync(dir).find(f => lower(f) === lower(n)); return f; };
if (!fs.existsSync(dataDir) || !fs.statSync(dataDir).isDirectory()) die(T('no existe la carpeta DATA: ' + dataDir, 'the DATA folder does not exist: ' + dataDir, 'la cartella DATA non esiste: ' + dataDir));
const classesName = ci(dataDir, 'classes'), commandsName = ci(dataDir, 'commands');
if (!classesName || !commandsName) die(T(dataDir + ' no parece la carpeta DATA del data.pak (faltan classes y commands). Sácala con estrai-dati.js', dataDir + ' does not look like the DATA folder of data.pak (classes and commands are missing). Extract it with estrai-dati.js', dataDir + ' non sembra la cartella DATA di data.pak (mancano classes e commands). Estraila con estrai-dati.js'));
const classesDir = path.join(dataDir, classesName), commandsDir = path.join(dataDir, commandsName);
// I XML del gioco sono in ASCII; quelli di una mod, in UTF-8. Si legge come UTF-8 se è valido e, altrimenti, come Latin-1
const readText = f => {
	const b = fs.readFileSync(f);
	let t;
	try { t = new TextDecoder('utf-8', { fatal: true }).decode(b); } catch (e) { t = b.toString('latin1'); }
	return t.replace(/^﻿/, '').replace(/\r\n/g, '\n');
};
const attr = (tag, name) => { const m = tag.match(new RegExp('\\b' + name + '\\s*=\\s*"([^"]*)"')); return m ? m[1] : undefined; };
const esc = s => s.replace(/&/g, '&amp;').replace(/"/g, '&quot;').replace(/</g, '&lt;').replace(/>/g, '&gt;');

const classes = new Map();   // id (minuscolo) -> { id, altid, file, text, header }
const aliases = new Map();   // altid (minuscolo) -> id reale
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
// Classi che la mod ha già (e altri suoi file), per non ripetere l'id
const modClassesDir = path.join(modDir, 'data', 'classes');
if (fs.existsSync(modClassesDir)) scanClasses(modClassesDir, null);

if (takenIds.has(lower(id))) die(T('el id "' + id + '" ya lo usa una clase (del juego o de tu mod). Elige otro', 'the id "' + id + '" is already used by a class (from the game or your mod). Pick another', 'l\'id "' + id + '" è già usato da una classe (del gioco o della tua mod). Scegline un altro'));

const base = classes.get(lower(opt.base)) || classes.get(lower(aliases.get(lower(opt.base)) || ''));
if (!base) die(T('no encuentro la clase base "' + opt.base + '" en ' + classesDir + '. Usa el id de una unidad del juego (por ejemplo RHastatus)', 'base class "' + opt.base + '" not found in ' + classesDir + '. Use the id of a game unit (for example RHastatus)', 'classe base "' + opt.base + '" non trovata in ' + classesDir + '. Usa l\'id di un\'unità del gioco (per esempio RHastatus)'));
const cpp = attr(base.header, 'cpp_class');
if (!cpp || !/^CVX(Unit|Druid|Catapult|Wagon|Ship|FlyingUnit|Hero)$/.test(cpp)) {
	die(T(base.id + ' no es una unidad (' + (cpp ? 'cpp_class ' + cpp : 'no tiene cpp_class, es una clase abstracta') + '). Elige una unidad completa, como RHastatus o GSwordsman',
		base.id + ' is not a unit (' + (cpp ? 'cpp_class ' + cpp : 'it has no cpp_class, it is an abstract class') + '). Pick a complete unit, such as RHastatus or GSwordsman',
		base.id + ' non è un\'unità (' + (cpp ? 'cpp_class ' + cpp : 'non ha cpp_class, è una classe astratta') + '). Scegli un\'unità completa, come RHastatus o GSwordsman'));
}
if (!/\bentity\s*=/.test(base.header)) die(T(base.id + ' no tiene entity (el modelo): una copia desaparecería al crearse. Elige una unidad completa', base.id + ' has no entity (the model): a copy would vanish when created. Pick a complete unit', base.id + ' non ha entity (il modello): una copia sparirebbe appena creata. Scegli un\'unità completa'));
const baseId = base.id;

// --- gli ordini del gioco -----------------------------------------------------------------------------------------------
const cmds = [];   // { name, file, text, open, srcs }
const takenCmdNames = new Set();
const usedKeys = new Map();  // src (minuscolo) -> Set di tasti
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
if (takenCmdNames.has(lower(trainName))) die(T('ya existe una orden llamada "' + trainName + '" (en el juego o en tu mod). Elige otro id', 'a command named "' + trainName + '" already exists (in the game or your mod). Pick another id', 'esiste già un comando chiamato "' + trainName + '" (nel gioco o nella tua mod). Scegli un altro id'));

const baseNames = [lower(baseId), base.altid ? lower(base.altid) : null].filter(Boolean);
const trains = cmds.filter(c => attr(c.open, 'method') === 'train' && baseNames.includes(lower((attr(c.open, 'param') || '').split(',')[0].trim())));
if (!trains.length) {
	const hired = cmds.some(c => attr(c.open, 'method') === 'trainex' && new RegExp('\\b' + baseId + '\\b', 'i').test(attr(c.open, 'param') || ''));
	die(T('en el juego no hay una orden normal de reclutar (method="train") para ' + baseId + (hired
		? ': se contrata en grupo (method="trainex"), y esta herramienta no copia ese tipo de orden. Elige otra base'
		: '. Elige una base que se recluta en un edificio (por ejemplo RHastatus), o haz la orden a mano (unita.md)'),
	'the game has no regular recruit command (method="train") for ' + baseId + (hired
		? ': it is hired as a group (method="trainex"), and this tool does not copy that kind of command. Pick another base'
		: '. Pick a base that is recruited in a building (for example RHastatus), or write the command by hand (unita.md)'),
	'nel gioco non c\'è un comando normale di reclutamento (method="train") per ' + baseId + (hired
		? ': si assolda in gruppo (method="trainex"), e questo strumento non copia quel tipo di comando. Scegli un\'altra base'
		: '. Scegli una base che si recluta in un edificio (per esempio RHastatus), oppure scrivi il comando a mano (unita.md)')));
}

// Edificio in cui si recluta
let building = opt.edificio;
// Se la base si recluta in più luoghi, si preferisce l'edificio della sua stessa civiltà
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
		die(T('el edificio "' + building + '" no existe (ni en el juego ni en tu mod). Usa el id de una clase de edificio, por ejemplo RBarracks', 'the building "' + building + '" does not exist (neither in the game nor in your mod). Use the id of a building class, for example RBarracks', 'l\'edificio "' + building + '" non esiste (né nel gioco né nella tua mod). Usa l\'id di una classe di edificio, per esempio RBarracks'));
	}
	const bcpp = real && attr(real.header, 'cpp_class');
	if (bcpp && /^CVX(Unit|Hero|Decor|Feedback|ScriptObj)$/.test(bcpp)) die(T('"' + building + '" no es un edificio (cpp_class ' + bcpp + ')', '"' + building + '" is not a building (cpp_class ' + bcpp + ')', '"' + building + '" non è un edificio (cpp_class ' + bcpp + ')'));
	chosen = trains.find(c => c.srcs.some(s => lower(s) === lower(building))) || chosen;
}
const buildings = building ? [building] : chosen.srcs;
if (!buildings.length) die(T('la orden de reclutar de ' + baseId + ' no dice en qué edificio sale. Indica uno con --edificio', 'the recruit command of ' + baseId + ' does not say which building it comes from. Give one with --edificio', 'il comando di reclutamento di ' + baseId + ' non dice da quale edificio parte. Indicane uno con --edificio'));
const notes = [];
if ((sameRace.length ? sameRace : trains).length > 1 && !building) notes.push(T(baseId + ' se recluta con varias órdenes (' + trains.map(c => c.srcs.join('/')).join('; ') + '): se ha copiado la de ' + chosen.srcs.join('/') + '. Usa --edificio para elegir',
	baseId + ' is recruited with several commands (' + trains.map(c => c.srcs.join('/')).join('; ') + '): the one for ' + chosen.srcs.join('/') + ' was copied. Use --edificio to choose',
	baseId + ' si recluta con più comandi (' + trains.map(c => c.srcs.join('/')).join('; ') + '): è stato copiato quello di ' + chosen.srcs.join('/') + '. Usa --edificio per scegliere'));
if (building && !chosen.srcs.some(s => lower(s) === lower(building))) notes.push(T(baseId + ' no se recluta en ' + building + ': la orden se ha copiado de la de ' + chosen.srcs.join('/') + ' y puede que su verificación (groupverifier) no encaje allí',
	baseId + ' is not recruited in ' + building + ': the command was copied from the one for ' + chosen.srcs.join('/') + ' and its verifier (groupverifier) may not fit there',
	baseId + ' non si recluta in ' + building + ': il comando è stato copiato da quello di ' + chosen.srcs.join('/') + ' e la sua verifica (groupverifier) potrebbe non andare bene lì'));

// --- nomi e testi -------------------------------------------------------------------------------------------------------
const baseDisplay = attr((base.text.match(/<properties\b[^>]*\bdisplay_name\s*=[^>]*>/i) || [''])[0], 'display_name');
const upperFirst = s => s[0].toUpperCase() + s.slice(1), lowerFirst = s => s[0].toLowerCase() + s.slice(1);
const pluralEn = s => /s$/i.test(s) ? s : s + 's';
const pluralEs = s => /z$/i.test(s) ? s.slice(0, -1) + 'ces' : /[aeiouáéíóú0-9]$/i.test(s) ? s + 's' : /s$/i.test(s) ? s : s + 'es';
const nameEs = opt.nombre || id;
const nameEn = opt['nombre-en'], nameIt = opt['nombre-it'];
const key = nameEn || nameEs;                                          // display_name è la chiave del testo
const keyPlural = nameEn ? (opt['plural-en'] || pluralEn(nameEn)) : (opt.plural || pluralEs(nameEs));
if (!opt.nombre && !nameEn) notes.push(T('no has dado --nombre: la unidad se llama "' + id + '". Cámbialo en loc\\spanish.loc.xml', 'you did not give --nombre: the unit is called "' + id + '". Change it in loc\\spanish.loc.xml', 'non hai dato --nombre: l\'unità si chiama "' + id + '". Cambialo in loc\\spanish.loc.xml'));
if (/\s/.test(key.trim()) && !opt.plural && !opt['plural-en']) notes.push(T('el plural de un nombre de varias palabras se adivina mal ("' + keyPlural + '"): pon --plural' + (nameEn ? ' / --plural-en' : ''), 'the plural of a multi-word name is guessed badly ("' + keyPlural + '"): give --plural' + (nameEn ? ' / --plural-en' : ''), 'il plurale di un nome di più parole viene indovinato male ("' + keyPlural + '"): metti --plural' + (nameEn ? ' / --plural-en' : '')));

// Il testo del pulsante: il "rollover" dell'ordine (verbo + nome)
const baseRollover = attr(chosen.open, 'rollover') || '';
const verbEn = (baseRollover.match(/^(\S+)\s/) || [])[1] || 'Train';
const verbs = { Equip: ['Equipar', 'Equipaggia'], Hire: ['Contratar', 'Assolda'], Train: ['Entrenar', 'Addestra'], Recruit: ['Reclutar', 'Recluta'] };
const [verbEs, verbIt] = verbs[verbEn] || verbs.Train;
const rollover = baseDisplay && baseRollover.includes(baseDisplay) ? baseRollover.split(baseDisplay).join(key) : (verbs[verbEn] ? verbEn : 'Train') + ' ' + key;

// Il tasto: quello richiesto, o una lettera del nome libera in quell'edificio
const keysInUse = new Set();
for (const b of buildings) for (const k of (usedKeys.get(lower(b)) || [])) keysInUse.add(k);
let tecla = opt.tecla ? lower(opt.tecla) : null;
if (!tecla) {
	for (const ch of lower((nameEn || nameEs) + id).replace(/[^a-z0-9]/g, '')) if (!keysInUse.has(ch)) { tecla = ch; break; }
	if (!tecla) { tecla = lower(attr(chosen.open, 'key') || ''); if (tecla) notes.push(T('no he encontrado una tecla libre en ' + buildings.join('/') + ': uso la de la base (' + tecla + '). Pon --tecla', 'no free key found in ' + buildings.join('/') + ': using the base\'s (' + tecla + '). Give --tecla', 'non ho trovato un tasto libero in ' + buildings.join('/') + ': uso quello della base (' + tecla + '). Metti --tecla')); }
}
else if (keysInUse.has(tecla)) notes.push(T('la tecla "' + tecla + '" ya la usa otra orden de ' + buildings.join('/') + ' (la mayoría de las veces se puede repetir, pero chocan en la partida)', 'the key "' + tecla + '" is already used by another command of ' + buildings.join('/') + ' (it can usually be repeated, but they clash in game)', 'il tasto "' + tecla + '" è già usato da un altro comando di ' + buildings.join('/') + ' (di solito si può ripetere, ma in partita si scontrano)'));

// --- la classe --------------------------------------------------------------------------------------------------------------
const changes = [];
let xml = base.text;
const header = base.header;
let newHeader = header.replace(/(\bid\s*=\s*")[^"]*"/, '$1' + id + '"').replace(/\s+altid\s*=\s*"[^"]*"/, '');
xml = xml.replace(header, () => newHeader);
// Cambia un attributo di un <properties>; se la classe non lo imposta (lo eredita), aggiunge una riga
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
		changes.push(label + ' ' + value + T(' (la base lo heredaba de su padre)', ' (the base inherited it from its parent)', ' (la base lo ereditava dal padre)'));
	}
}
if (stats.maxhealth !== undefined) setProp('maxhealth', stats.maxhealth, T('vida', 'health', 'vita'));
if (stats.damage !== undefined) setProp('damage', stats.damage, T('daño', 'damage', 'danno'));
if (stats.armor !== undefined) { setProp('armor_slash', stats.armor, T('armadura (corte)', 'armor (slash)', 'armatura (taglio)')); setProp('armor_pierce', stats.armor, T('armadura (perforante)', 'armor (pierce)', 'armatura (perforante)')); }
if (stats.speed !== undefined) setProp('speed', stats.speed, T('velocidad', 'speed', 'velocità'));
// Nome
if (/\bdisplay_name\s*=/.test(xml)) {
	xml = xml.replace(/(\bdisplay_name\s*=\s*")[^"]*"/, '$1' + esc(key) + '"');
	xml = xml.replace(/(\bdisplay_name_plural\s*=\s*")[^"]*"/, '$1' + esc(keyPlural) + '"');
	if (!/\bdisplay_name_plural\s*=/.test(xml)) xml = xml.replace(/(\bdisplay_name\s*=\s*"[^"]*")/, '$1 display_name_plural="' + esc(keyPlural) + '"');
}
else xml = xml.replace(/<\/class\s*>/i, '\t<properties display_name="' + esc(key) + '" display_name_plural="' + esc(keyPlural) + '"/>\n\n</class>');
// Dove compare nell'editor
xml = xml.replace(/(\bedittree_pos\s*=\s*"[^"]*\/)[^"\/]*"/, '$1' + esc(key.replace(/\//g, '-')) + '"');
const classComment = T('<!-- Unidad nueva ' + id + ': COPIA ENTERA de la clase ' + baseId + ' del juego, con otro id' + (changes.length ? ' y otras estadísticas (' + changes.join(', ') + ')' : '') +
	'. Se copia entera, y no con parent, porque el juego no hereda del padre la entidad (el modelo). Hecha con crea-unita.js. -->\n',
	'<!-- New unit ' + id + ': FULL COPY of the game class ' + baseId + ', with a different id' + (changes.length ? ' and different stats (' + changes.join(', ') + ')' : '') +
	'. It is copied whole, not through parent, because the game does not inherit the entity (the model) from the parent. Made with crea-unita.js. -->\n',
	'<!-- Nuova unità ' + id + ': COPIA INTERA della classe ' + baseId + ' del gioco, con un altro id' + (changes.length ? ' e altre statistiche (' + changes.join(', ') + ')' : '') +
	'. Viene copiata intera, e non con parent, perché il gioco non eredita dal padre l\'entità (il modello). Fatta con crea-unita.js. -->\n');
xml = xml.replace(/<class\b/i, () => classComment + '<class');
if (!xml.endsWith('\n')) xml += '\n';

// --- l'ordine ---------------------------------------------------------------------------------------------------------------
let open = chosen.open;
const setA = (name, value) => {
	const re = new RegExp('(\\b' + name + '\\s*=\\s*")[^"]*"');
	if (re.test(open)) open = open.replace(re, (m, a) => a + esc(String(value)) + '"');
	else open = open.replace(/\s*\/?>$/, m => '\n\t' + name + '="' + esc(String(value)) + '"' + m);
};
setA('name', chosen.name.includes(baseId) ? chosen.name.split(baseId).join(id) : trainName);
const cmdName = attr(open, 'name');
if (takenCmdNames.has(lower(cmdName))) die(T('ya existe una orden llamada "' + cmdName + '". Elige otro id', 'a command named "' + cmdName + '" already exists. Pick another id', 'esiste già un comando chiamato "' + cmdName + '". Scegli un altro id'));
if (tecla) setA('key', tecla);
if (costGold !== undefined) setA('costgold', costGold);
if (costFood !== undefined) setA('costfood', costFood);
if (execDelay !== undefined) setA('execdelay', execDelay);
setA('rollover', rollover);
// param è la classe e, a volte, altri dati ("RArcher,Arrows"): si cambia solo la classe
setA('param', id + (attr(open, 'param') || '').replace(/^[^,]*/, ''));
setA('sclass', id);
if (attr(open, 'description') !== undefined) notes.push(T('la orden de ' + baseId + ' trae un description="' + attr(open, 'description') + '" que se ha copiado igual. Cámbialo en data\\commands\\' + id + '.xml', 'the command of ' + baseId + ' has a description="' + attr(open, 'description') + '" that was copied as is. Change it in data\\commands\\' + id + '.xml', 'il comando di ' + baseId + ' ha un description="' + attr(open, 'description') + '" che è stato copiato così com\'è. Cambialo in data\\commands\\' + id + '.xml'));
open = tidyOpen(open);
function tidyOpen(o) {
	// Come nei file del gioco: name e priority nella prima riga e poi un attributo per riga
	const m = o.match(/^<cmd\s+([\s\S]*?)(\/?)>$/);
	const attrs = [...m[1].matchAll(/([A-Za-z_][\w-]*)\s*=\s*"([^"]*)"/g)].map(a => a[1] + '="' + a[2] + '"');
	const first = attrs.shift();
	return '<cmd ' + first + (attrs.length ? ' ' + attrs.shift() : '') + attrs.map(a => '\n\t' + a).join('') + m[2] + '>';
}
const bodyRest = chosen.text.slice(chosen.open.length).replace(/<\/cmd\s*>\s*$/i, '').replace(/<src\b[^>]*\/?>(\s*<\/src>)?/gi, '').trim();
const cmdXml = '<commands>\n\n' +
	T('<!-- La orden de reclutar ' + id + ' en ' + buildings.join(', ') + '. Es una copia de "' + chosen.name + '" (data\\commands\\' + chosen.file + ' del juego) con otra clase' +
	(costGold !== undefined || costFood !== undefined || execDelay !== undefined ? ', otro coste' : '') + ' y otra tecla. El juego carga todos los .xml de data\\commands. Hecha con crea-unita.js. -->\n',
	'<!-- The command to recruit ' + id + ' in ' + buildings.join(', ') + '. It is a copy of "' + chosen.name + '" (data\\commands\\' + chosen.file + ' of the game) with a different class' +
	(costGold !== undefined || costFood !== undefined || execDelay !== undefined ? ', a different cost' : '') + ' and a different key. The game loads every .xml in data\\commands. Made with crea-unita.js. -->\n',
	'<!-- Il comando per reclutare ' + id + ' in ' + buildings.join(', ') + '. È una copia di "' + chosen.name + '" (data\\commands\\' + chosen.file + ' del gioco) con un\'altra classe' +
	(costGold !== undefined || costFood !== undefined || execDelay !== undefined ? ', un altro costo' : '') + ' e un altro tasto. Il gioco carica tutti i .xml di data\\commands. Fatto con crea-unita.js. -->\n') +
	open + '\n' + (bodyRest ? '\t' + bodyRest.replace(/\n\s*/g, '\n\t') + '\n' : '') +
	buildings.map(b => '\t<src obj="' + esc(b) + '"/>\n').join('') + '</cmd>\n\n</commands>\n';

// --- i testi -----------------------------------------------------------------------------------------------------------------
const langs = [{ lang: 'spanish', name: nameEs, plural: opt.plural || pluralEs(nameEs), roll: verbEs + ' ' + lowerFirst(nameEs) }];
if (nameEn) langs.push({ lang: 'english', name: nameEn, plural: keyPlural, roll: rollover });
if (nameIt) {
	langs.push({ lang: 'italian', name: nameIt, plural: nameIt, roll: verbIt + ' ' + lowerFirst(nameIt) });
	notes.push(T('el plural italiano de "' + nameIt + '" se ha puesto igual que el singular; corrígelo en loc\\italian.loc.xml', 'the Italian plural of "' + nameIt + '" was set equal to the singular; fix it in loc\\italian.loc.xml', 'il plurale italiano di "' + nameIt + '" è stato messo uguale al singolare; correggilo in loc\\italian.loc.xml'));
}
const locEntries = L => [
	[key + '@' + id + ' class name', L.name],
	[keyPlural + '@' + id + ' class name plural', L.plural],
	[rollover + '@' + cmdName, L.roll],
];

// --- si prepara tutto e si scrive in una volta -------------------------------------------------------------------------------
const outputs = new Map();   // percorso -> contenuto
const classFile = path.join(modDir, 'data', 'classes', id + '.sc.xml');
const cmdFile = path.join(modDir, 'data', 'commands', id + '.xml');
for (const f of [classFile, cmdFile]) if (fs.existsSync(f)) die(T('ya existe ' + f + '. No lo piso: bórralo o elige otro id', f + ' already exists. Not overwriting it: delete it or pick another id', f + ' esiste già. Non lo sovrascrivo: cancellalo o scegli un altro id'));
outputs.set(classFile, xml);
outputs.set(cmdFile, cmdXml);
for (const L of langs) {
	const f = path.join(modDir, 'loc', L.lang + '.loc.xml');
	const lines = locEntries(L).map(([t, r]) => [t, '\t<translationtableentry text="' + esc(t) + '" result="' + esc(r) + '"/>']);
	if (fs.existsSync(f)) {
		let cur = fs.readFileSync(f, 'utf8').replace(/^﻿/, '');
		const fresh = lines.filter(([t]) => !cur.includes('text="' + esc(t) + '"'));
		if (fresh.length) {
			if (!/<\/translationtable>/i.test(cur)) die(T(f + ' no tiene </translationtable>: no sé dónde añadir los textos', f + ' has no </translationtable>: cannot tell where to add the texts', f + ' non ha </translationtable>: non so dove aggiungere i testi'));
			const add = '\t<!-- ' + id + ' -->\n' + fresh.map(l => l[1]).join('\n') + '\n';
			cur = cur.replace(/(\n?)([ \t]*)<\/translationtable>(?![\s\S]*<\/translationtable>)/i, (m, nl) => '\n' + add + '</translationtable>');
			outputs.set(f, cur);
		}
	}
	else outputs.set(f, '<translationtable>\n\t' + T('<!-- El nombre de la clase: "<display_name>@<id de la clase> class name", su plural, y el texto del botón de reclutar (el rollover de la orden, con el nombre de la orden como contexto) -->',
		'<!-- The class name: "<display_name>@<class id> class name", its plural, and the recruit button text (the rollover of the command, with the command name as context) -->',
		'<!-- Il nome della classe: "<display_name>@<id della classe> class name", il suo plurale, e il testo del pulsante di reclutamento (il rollover del comando, con il nome del comando come contesto) -->') + '\n' +
		lines.map(l => l[1]).join('\n') + '\n</translationtable>\n');
}
const modJson = path.join(modDir, 'mod.json');
if (!fs.existsSync(modJson)) {
	const mid = path.basename(path.resolve(modDir)).toLowerCase().replace(/[^a-z0-9_-]+/g, '-').replace(/^-+|-+$/g, '') || 'mi-mod';
	outputs.set(modJson, JSON.stringify({
		id: mid, name: path.basename(path.resolve(modDir)), version: '1.0', category: 'Units & Heroes', tags: ['Gameplay'], authors: [],
		description: T('Unidad nueva ' + nameEs + '. Cambia la jugabilidad: en multijugador todos tienen que llevarlo.', 'New unit ' + nameEs + '. It changes gameplay: in multiplayer everyone must have it.', 'Nuova unità ' + nameEs + '. Cambia il gameplay: in multigiocatore tutti devono averla.'), redistribuible: false,
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
	die(T('no he podido escribir ' + e.path + ' (' + e.message + '). He deshecho lo escrito', 'could not write ' + e.path + ' (' + e.message + '). Everything written was undone', 'impossibile scrivere ' + e.path + ' (' + e.message + '). Ho annullato quanto scritto'));
}

// --- rapporto ----------------------------------------------------------------------------------------------------------------
console.log(T('unidad ' + id + ' (copia de ' + baseId + (baseDisplay ? ', "' + baseDisplay + '"' : '') + ', se recluta en ' + buildings.join(', ') + ')',
	'unit ' + id + ' (copy of ' + baseId + (baseDisplay ? ', "' + baseDisplay + '"' : '') + ', recruited in ' + buildings.join(', ') + ')',
	'unità ' + id + ' (copia di ' + baseId + (baseDisplay ? ', "' + baseDisplay + '"' : '') + ', si recluta in ' + buildings.join(', ') + ')'));
for (const f of written) console.log('  ' + path.relative(modDir, f));
console.log(T('  cambios en la clase: ' + (changes.length ? changes.join('; ') : 'ninguno (mismas estadísticas que la base)'),
	'  changes to the class: ' + (changes.length ? changes.join('; ') : 'none (same stats as the base)'),
	'  modifiche alla classe: ' + (changes.length ? changes.join('; ') : 'nessuna (stesse statistiche della base)')));
console.log(T('  orden ' + cmdName + ': tecla ' + (tecla || '(la de la base)') + ', ' + (costGold !== undefined ? costGold : attr(chosen.open, 'costgold') || '?') + ' de oro',
	'  command ' + cmdName + ': key ' + (tecla || '(the base\'s)') + ', ' + (costGold !== undefined ? costGold : attr(chosen.open, 'costgold') || '?') + ' gold',
	'  comando ' + cmdName + ': tasto ' + (tecla || '(quello della base)') + ', ' + (costGold !== undefined ? costGold : attr(chosen.open, 'costgold') || '?') + ' di oro'));
for (const n of notes) console.log(T('  aviso: ', '  warning: ', '  avviso: ') + n);
console.log(T('Falta: el retrato y el botón son los de ' + baseId + ' (hazlos con converti-immagine.js si quieres propios), y la IA no reclutará la unidad. Pasa el mod por valida-mod.js --datos <DATA>.',
	'To do: the portrait and the button are those of ' + baseId + ' (make your own with converti-immagine.js if you want), and the AI will not recruit the unit. Run the mod through valida-mod.js --datos <DATA>.',
	'Da fare: il ritratto e il pulsante sono quelli di ' + baseId + ' (creane di tuoi con converti-immagine.js se vuoi), e l\'IA non recluterà l\'unità. Passa la mod per valida-mod.js --datos <DATA>.'));
