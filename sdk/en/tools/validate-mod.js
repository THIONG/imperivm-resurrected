// Validates an Imperivm Resurrected mod without opening the game (SDK).

const LEGACY_KEYS = {"nombre":"name","descripcion":"description","autores":"authors","redistribuible":"redistributable","licencia":"license","cargarDespuesDe":"loadAfter","cargarAntesDe":"loadBefore","etiquetas":"tags","categoria":"category","tipo":"kind","idioma":"language","codigo":"code","ordenes":"commands","fuentes":"sources","archivos":"files","carpeta":"folder","perfil":"profile","requiere":"requires","incompatibleCon":"incompatibleWith","icono":"icon","indice":"index","bioma":"biome","plantilla":"template","nombreInterno":"internalName","letra":"letter","aleatoria":"random","ia":"ai","iaPropia":"ownAi","clases":"classes","valores":"values","defecto":"default"};
// Keys written with the Spanish names (before 0.31.51) still work: they are copied to the English ones before checking.
function modernKeys(o) {
	if (o && typeof o === 'object' && !Array.isArray(o)) {
		for (const [old, now] of Object.entries(LEGACY_KEYS)) if (o[old] !== undefined && o[now] === undefined) o[now] = o[old];
		if (o.alias && typeof o.alias === 'object') modernKeys(o.alias);
	}
	return o;
}

const LEGACY_DIRECTIVES = { principio: 'top', final: 'bottom', antes: 'before', despues: 'after', reemplazar: 'replace', fin: 'end' };
//
//   node validate-mod.js <mod folder> [--data <DATA folder of the extracted data.pak>]
//
// Checks the same things the game would check when loading it (and a few more), and reports them with file and line:
//   mod.json, allowed extensions, civs\*.json, specials\*.json, patches (XML and INI), classes, orders, texts (loc),
//   scripts (first line with the signature, events, Mod_* functions) and the UNIT_SPECIALS.INI bug.
// With --data it also checks that the files the mod patches exist in the game and that the classes and scripts it
// refers to exist. With --game (the game folder, read-only) it also checks the images and entities that the mod
// takes from the game itself (Units.pak, Buildings.pak, MapObjects.pak, Visuals.pak).
// Graphics (.ent.xml and .rle): missing images (the game HANGS when loading the class), repeated idx or outside 1..32, layers and
// animations that point to images that do not exist, variations different from columns, rows/columns that do not match the .rle,
// engine limits (32 images, 32 layers, 8 states, animations 1..28, 64 frames), corrupt .rle files and malformed .pass files.
// Exits with code 1 if there are errors.
// Messages come out in Spanish, English or Italian (lib\lang.js: --lang es|en|it).
const fs = require('fs'), path = require('path');
const { T } = require('./lib/lang');

const args = process.argv.slice(2);
const modDir = args.find(a => !a.startsWith('--') && !['--datos', '--juego'].includes(args[args.indexOf(a) - 1]));
const dataIdx = args.indexOf('--datos');
const dataDir = dataIdx >= 0 ? args[dataIdx + 1] : null;
const gameIdx = args.indexOf('--juego');
const gameDir = gameIdx >= 0 ? args[gameIdx + 1] : null;  // game folder: the names of the graphics .pak files are read (read-only)
if (!modDir || !fs.existsSync(modDir) || !fs.statSync(modDir).isDirectory()) {
	console.error(T(
		'uso: node validate-mod.js <carpeta del mod> [--datos <carpeta DATA del data.pak extraído>] [--juego <carpeta del juego>]',
		'usage: node validate-mod.js <mod folder> [--datos <DATA folder of the extracted data.pak>] [--juego <game folder>]',
		'uso: node validate-mod.js <cartella della mod> [--datos <cartella DATA del data.pak estratto>] [--juego <cartella del gioco>]'));
	process.exit(2);
}

// --- what the game knows ----------------------------------------------------------------------------------------------
const ALLOWED = ['.pak', '.bfhp', '.xml', '.ini', '.vs', '.bmp', '.tga', '.png', '.jpg', '.wav', '.ogg', '.mp3', '.rle', '.mmp', '.txt',
	'.json', '.loc', '.dat', '.sgt', '.dls', '.sty', '.avi', '.wmv', '.bik', '.fnt', '.ttf', '.md', '.pdf', '.html', '.htm', '.rtf', '.csv',
	'.sc', '.ent', '.cfg', '.lst', '.42', '.usr', '.sdw', '.pass'];
const FORBIDDEN = ['.exe', '.dll', '.asi', '.bat', '.cmd', '.com', '.scr', '.ps1', '.vbs', '.js', '.msi', '.sys', '.cpl', '.lnk', '.reg', '.jar', '.hta', '.wsf', '.pif'];
const GAME_CIVS = ['Gaul', 'RepublicanRome', 'Carthage', 'Iberia', 'ImperialRome', 'Britain', 'Egypt', 'Germany'];
const GAME_SPECIALS = ['Parry', 'Drain', 'Ferocity', 'Triple strike', 'Deflection', 'Active', 'Life steal', 'Determination', 'Combat skill',
	'Toughness', 'Offensive tactics', 'Defensive tactics', 'Penetration', 'Spike armor', 'Bleeding attack', 'Attack skill', 'Defense skill',
	'Death blow', 'Power strike', 'Expertise', 'Charge', 'Regeneration', 'Rage', 'Revenge', 'Triumph', 'Disease attack', 'Sneak', 'Invisibility',
	'Learning', 'Teaching', 'Freedom', 'Cripple', 'Healing', 'Keen sight', 'Trample', 'Curse'];
const EVENTS = {
	OnGameStart: ['void'],
	OnObjectDie: ['void', 'Obj'],
	OnObjectKill: ['void', 'Obj', 'Obj'],
	OnSettlementCaptured: ['void', 'Obj', 'int', 'int'],
	OnPlayerDefeated: ['void', 'int'],
	OnAttackResolved: ['void', 'Obj', 'Obj', 'int'],
};
const MOD_FUNCTIONS = ['Mod_Log', 'Mod_LogInt', 'Mod_IsActive', 'Mod_ApiVersion', 'Mod_SetInt', 'Mod_GetInt', 'Mod_AddInt', 'Mod_HasKey',
	'Mod_AttackDamage', 'Mod_SetAttackDamage', 'Mod_HasSpecial', 'Mod_SetSpecial', 'Mod_TrueRace', 'Mod_GetOption', 'Mod_GetOptionValue'];
const ID_RE = /^[A-Za-z][A-Za-z0-9_]{0,23}$/;

// --- report -----------------------------------------------------------------------------------------------------------
// kind is internal ('ERROR' | 'AVISO'); the printed label depends on the language
let errors = 0, warnings = 0;
const LABEL_ERROR = () => T('ERROR', 'ERROR', 'ERRORE');
const LABEL_WARN = () => T('AVISO', 'WARNING', 'AVVISO');
const report = (kind, file, line, msg) => {
	if (kind === 'ERROR') errors++; else warnings++;
	console.log(`${kind === 'ERROR' ? LABEL_ERROR() : LABEL_WARN()}  ${file}${line ? ':' + line : ''}  ${msg}`);
};
const error = (file, line, msg) => report('ERROR', file, line, msg);
const warn = (file, line, msg) => report('AVISO', file, line, msg);

// --- files ------------------------------------------------------------------------------------------------------------
function walk(dir, rel = '') {
	const out = [];
	for (const e of fs.readdirSync(dir, { withFileTypes: true })) {
		const r = rel ? rel + '/' + e.name : e.name;
		if (e.isDirectory()) out.push(...walk(path.join(dir, e.name), r)); else out.push(r);
	}
	return out;
}
const files = walk(modDir).filter(f => !f.startsWith('.'));
const read = rel => fs.readFileSync(path.join(modDir, rel), 'utf8').replace(/^﻿/, '');
const lineOf = (text, index) => text.slice(0, index).split('\n').length;
const lower = s => s.toLowerCase();
const has = rel => files.some(f => lower(f) === lower(rel));
// The game treats the data.pak folders as case-insensitive: they are looked up ignoring case
function gameFile(rel) {
	if (!dataDir) return null;
	let cur = dataDir;
	for (const part of rel.replace(/^data\//i, '').split('/')) {
		const found = fs.existsSync(cur) && fs.readdirSync(cur).find(f => lower(f) === lower(part));
		if (!found) return null;
		cur = path.join(cur, found);
	}
	return cur;
}

// --- XML: well-formedness check ----------------------------------------------------------------------------------------
function checkXml(text) {
	const stack = [];
	let i = 0, roots = 0;
	const fail = (msg, at) => ({ line: lineOf(text, at), msg });
	while (i < text.length) {
		const lt = text.indexOf('<', i);
		if (lt < 0) break;
		if (text.startsWith('<!--', lt)) {
			const end = text.indexOf('-->', lt + 4);
			if (end < 0) return fail(T('comentario sin cerrar', 'unclosed comment', 'commento non chiuso'), lt);
			i = end + 3; continue;
		}
		if (text.startsWith('<![CDATA[', lt)) {
			const end = text.indexOf(']]>', lt);
			if (end < 0) return fail(T('CDATA sin cerrar', 'unclosed CDATA', 'CDATA non chiuso'), lt);
			i = end + 3; continue;
		}
		if (text.startsWith('<?', lt) || text.startsWith('<!', lt)) {
			const end = text.indexOf('>', lt);
			if (end < 0) return fail(T('instrucción sin cerrar', 'unclosed instruction', 'istruzione non chiusa'), lt);
			i = end + 1; continue;
		}
		// tag: scanned while respecting quotes
		let j = lt + 1, quote = null;
		while (j < text.length) {
			const c = text[j];
			if (quote) { if (c === quote) quote = null; }
			else if (c === '"' || c === "'") quote = c;
			else if (c === '>') break;
			else if (c === '<') return fail(T(
				'"<" dentro de una etiqueta (¿falta un ">" o hay un "<" sin escribir como &lt; en un valor?)',
				'"<" inside a tag (is a ">" missing, or is there a "<" in a value not written as &lt;?)',
				'"<" dentro un tag (manca un ">" o c\'è un "<" in un valore non scritto come &lt;?)'), lt);
			j++;
		}
		if (j >= text.length) return fail(T('etiqueta sin cerrar', 'unclosed tag', 'tag non chiuso'), lt);
		const tag = text.slice(lt + 1, j);
		if (tag.startsWith('/')) {
			const name = tag.slice(1).trim();
			const top = stack.pop();
			if (!top) return fail(T(`</${name}> sin abrir`, `</${name}> was never opened`, `</${name}> senza apertura`), lt);
			if (top.name !== name) return fail(T(
				`</${name}> no cierra a <${top.name}> (abierto en la línea ${top.line})`,
				`</${name}> does not close <${top.name}> (opened on line ${top.line})`,
				`</${name}> non chiude <${top.name}> (aperto alla riga ${top.line})`), lt);
		}
		else {
			const m = tag.match(/^([^\s/>]+)/);
			if (!m) return fail(T('etiqueta vacía', 'empty tag', 'tag vuoto'), lt);
			const selfClosing = tag.endsWith('/');
			// attributes: name = "value"
			const attrText = tag.slice(m[0].length).replace(/\/$/, '');
			const rest = attrText.replace(/\s*[^\s=]+\s*=\s*("[^"]*"|'[^']*')/g, '').trim();
			if (rest) return fail(T(
				`atributo mal escrito en <${m[1]}> (cerca de "${rest.slice(0, 30)}"): los valores van entre comillas`,
				`bad attribute in <${m[1]}> (near "${rest.slice(0, 30)}"): values must be quoted`,
				`attributo scritto male in <${m[1]}> (vicino a "${rest.slice(0, 30)}"): i valori vanno tra virgolette`), lt);
			if (!selfClosing) stack.push({ name: m[1], line: lineOf(text, lt) });
			else if (stack.length === 0) roots++;
			if (stack.length === 1 && !selfClosing) roots++;
		}
		i = j + 1;
	}
	if (stack.length) return fail(T(`<${stack[stack.length - 1].name}> sin cerrar`, `<${stack[stack.length - 1].name}> is not closed`, `<${stack[stack.length - 1].name}> non chiuso`), text.length);
	return null;
}

// --- mod.json ---------------------------------------------------------------------------------------------------------
if (!has('mod.json')) {
	warn('mod.json', 0, T(
		'no hay mod.json: el juego creará uno con el nombre de la carpeta',
		'there is no mod.json: the game will create one named after the folder',
		'manca mod.json: il gioco ne creerà uno con il nome della cartella'));
}
else {
	try {
		const m = modernKeys(JSON.parse(read('mod.json')));
		for (const k of ['id', 'name', 'version']) if (!m[k]) warn('mod.json', 0, T(`falta "${k}"`, `"${k}" is missing`, `manca "${k}"`));
		// nombre and descripcion: a text, or an object with one text per language
		for (const k of ['name', 'description']) {
			const v = m[k];
			if (v && typeof v === 'object') {
				for (const [lang, text] of Object.entries(v)) if (typeof text !== 'string') error('mod.json', 0, T(
					`"${k}": el texto de "${lang}" tiene que ser un texto`,
					`"${k}": the text for "${lang}" has to be a string`,
					`"${k}": il testo di "${lang}" deve essere una stringa`));
				if (!Object.keys(v).length) error('mod.json', 0, T(`"${k}" está vacío`, `"${k}" is empty`, `"${k}" è vuoto`));
			}
			else if (v !== undefined && typeof v !== 'string') error('mod.json', 0, T(
				`"${k}" tiene que ser un texto o un objeto con un texto por idioma`,
				`"${k}" has to be a string or an object with one string per language`,
				`"${k}" deve essere una stringa o un oggetto con una stringa per lingua`));
		}
		// category and tags (see publishing.md): the category is the type in Browse content; the tags feed the players (Maps) and language filters
		const kCategorias = ['Gameplay & Balance', 'Units & Heroes', 'Civilizations', 'Maps', 'Campaigns', 'Conquest', 'Graphics',
			'Audio & Voices', 'Translations'];
		const kCategoriasAntiguas = { 'Scenarios': 'Maps', 'Interface': 'Graphics', 'Total Conversions': 'Gameplay & Balance' };
		const kConIdioma = ['Campaigns', 'Audio & Voices', 'Translations'];
		const kIdiomas = ['Spanish', 'English', 'Italian', 'German', 'French', 'Polish', 'Multilingual'];
		const esJugadores = t => /^[2-8](-[2-8])? players$/.test(t);
		if (m.category === undefined) {
			warn('mod.json', 0, T(
				`falta "category": la que elegirás al subirlo a GameBanana (${kCategorias.join(', ')})`,
				`"category" is missing: the one you will pick when uploading to GameBanana (${kCategorias.join(', ')})`,
				`manca "category": quella che sceglierai caricando su GameBanana (${kCategorias.join(', ')})`));
		}
		else if (kCategoriasAntiguas[m.category]) {
			warn('mod.json', 0, T(
				`"category": "${m.category}" es un nombre antiguo: usa "${kCategoriasAntiguas[m.category]}"`,
				`"category": "${m.category}" is an old name: use "${kCategoriasAntiguas[m.category]}"`,
				`"category": "${m.category}" è un nome vecchio: usa "${kCategoriasAntiguas[m.category]}"`));
		}
		else if (!kCategorias.includes(m.category)) {
			error('mod.json', 0, T(
				`"category": "${m.category}" no es una de las de GameBanana (${kCategorias.join(', ')})`,
				`"category": "${m.category}" is not a GameBanana category (${kCategorias.join(', ')})`,
				`"category": "${m.category}" non è una categoria di GameBanana (${kCategorias.join(', ')})`));
		}
		const categoria = kCategoriasAntiguas[m.category] || m.category;
		const etiquetas = Array.isArray(m.tags) ? m.tags : [];
		if (m.tags !== undefined && !Array.isArray(m.tags)) error('mod.json', 0, T('"tags" tiene que ser una lista', '"tags" has to be a list', '"tags" deve essere una lista'));
		for (const t of etiquetas) {
			if (t === 'Gameplay' || t === 'Cosmetic' || t === 'Online tested') {
				warn('mod.json', 0, T(
					`etiqueta "${t}": ya no hace falta (el juego sabe por los archivos si cambia la jugabilidad)`,
					`tag "${t}": no longer needed (the game knows from the files whether it changes gameplay)`,
					`etichetta "${t}": non serve più (il gioco capisce dai file se cambia il gameplay)`));
			}
			else if (esJugadores(t)) {
				if (categoria !== 'Maps') warn('mod.json', 0, T(
					`etiqueta "${t}": los jugadores solo se filtran en la categoría Maps`,
					`tag "${t}": players are only filtered in the Maps category`,
					`etichetta "${t}": i giocatori si filtrano solo nella categoria Maps`));
			}
			else if (kIdiomas.includes(t)) {
				if (!kConIdioma.includes(categoria)) warn('mod.json', 0, T(
					`etiqueta "${t}": el idioma solo se filtra en ${kConIdioma.join(', ')}`,
					`tag "${t}": the language is only filtered in ${kConIdioma.join(', ')}`,
					`etichetta "${t}": la lingua si filtra solo in ${kConIdioma.join(', ')}`));
			}
			else {
				warn('mod.json', 0, T(
					`etiqueta "${t}": el juego no la usa para filtrar (valen "4 players", "2-4 players" y ${kIdiomas.join(', ')})`,
					`tag "${t}": the game doesn't filter by it (valid ones: "4 players", "2-4 players" and ${kIdiomas.join(', ')})`,
					`etichetta "${t}": il gioco non la usa per filtrare (valgono "4 players", "2-4 players" e ${kIdiomas.join(', ')})`));
			}
		}
		if (categoria === 'Maps' && !etiquetas.some(esJugadores)) warn('mod.json', 0, T(
			'mapa sin etiqueta de jugadores: añade "4 players" (o "2-4 players") en "tags" y en las etiquetas de GameBanana, o no saldrá al filtrar por jugadores',
			'map without a players tag: add "4 players" (or "2-4 players") to "tags" and to the GameBanana tags, or it won\'t show up when filtering by players',
			'mappa senza etichetta dei giocatori: aggiungi "4 players" (o "2-4 players") in "tags" e nelle etichette di GameBanana, altrimenti non comparirà filtrando per giocatori'));
		if (kConIdioma.includes(categoria) && !etiquetas.some(t => kIdiomas.includes(t))) warn('mod.json', 0, T(
			`sin etiqueta de idioma: añade el idioma (${kIdiomas.join(', ')}) en "tags" y en las etiquetas de GameBanana, o no saldrá al filtrar por idioma`,
			`no language tag: add the language (${kIdiomas.join(', ')}) to "tags" and to the GameBanana tags, or it won't show up when filtering by language`,
			`nessuna etichetta della lingua: aggiungi la lingua (${kIdiomas.join(', ')}) in "tags" e nelle etichette di GameBanana, altrimenti non comparirà filtrando per lingua`));
	}
	catch (e) { error('mod.json', 0, T('no es JSON válido: ', 'not valid JSON: ', 'JSON non valido: ') + e.message); }
}

// --- extensions and paths ---------------------------------------------------------------------------------------------
for (const f of files) {
	const ext = path.extname(f);
	if (ext && FORBIDDEN.includes(lower(ext))) error(f, 0, T(
		`extensión "${ext}" prohibida (ejecutables y scripts del sistema): el mod NO se cargaría`,
		`extension "${ext}" is forbidden (executables and system scripts): the mod would NOT load`,
		`estensione "${ext}" vietata (eseguibili e script di sistema): la mod NON verrebbe caricata`));
	else if (ext && !ALLOWED.includes(lower(ext))) warn(f, 0, T(
		`extensión "${ext}": el juego no la usa, el archivo solo ocupa sitio`,
		`extension "${ext}": the game does not use it, the file only takes up space`,
		`estensione "${ext}": il gioco non la usa, il file occupa solo spazio`));
	if (f.includes('..') || /^[a-z]:/i.test(f)) error(f, 0, T('ruta no permitida', 'path not allowed', 'percorso non consentito'));
}

// --- game options (options/<id>.json) ---------------------------------------------------------------------------------
let optionBits = 0;
for (const f of files.filter(f => /^options\/[^/]+\.json$/i.test(f))) {
	let j;
	try { j = modernKeys(JSON.parse(read(f))); } catch (e) { error(f, 0, T('no es JSON válido: ', 'not valid JSON: ', 'JSON non valido: ') + e.message); continue; }
	const id = j.id ?? path.basename(f, '.json');
	if (!/^[A-Za-z0-9_-]{1,32}$/.test(id)) error(f, 0, T(
		`el id "${id}" no vale (letras, números, _ y -; hasta 32)`,
		`the id "${id}" is not valid (letters, digits, _ and -; up to 32)`,
		`l'id "${id}" non è valido (lettere, numeri, _ e -; fino a 32)`));
	if (j.name === undefined) warn(f, 0, T(
		'falta "name": en la sala saldrá el id',
		'"name" is missing: the lobby will show the id',
		'manca "name": nella stanza apparirà l\'id'));
	const defecto = j.default;
	const valores = j.values;
	if (valores === undefined) {
		if (defecto !== undefined && typeof defecto !== 'boolean') error(f, 0, T(
			'"default" de un interruptor es true o false',
			'"default" of a switch is true or false',
			'"default" di un interruttore è true o false'));
		optionBits += 1;
	}
	else if (!Array.isArray(valores) || valores.length < 2 || valores.length > 16) error(f, 0, T(
		'"values" es una lista de 2 a 16 opciones',
		'"values" is a list of 2 to 16 options',
		'"values" è una lista da 2 a 16 opzioni'));
	else {
		const ids = valores.map(v => String(typeof v === 'string' ? v : (v && v.id) || '').toLowerCase());
		valores.forEach((v, i) => { if (typeof v !== 'string' && !(v && typeof v === 'object')) error(f, 0, T(
			`values[${i}]: un texto o un objeto con "name"`,
			`values[${i}]: a string or an object with "name"`,
			`values[${i}]: una stringa o un oggetto con "name"`)); });
		if (defecto !== undefined && !(Number.isInteger(defecto) && defecto >= 0 && defecto < valores.length) && !(typeof defecto === 'string' && ids.includes(defecto.toLowerCase()))) {
			error(f, 0, T(
				'"default" de una lista es el número del valor (desde 0) o su id',
				'"default" of a list is the number of the value (from 0) or its id',
				'"default" di una lista è il numero del valore (da 0) o il suo id'));
		}
		optionBits += Math.ceil(Math.log2(valores.length));
	}
}
if (optionBits > 8) warn('options', 0, T(
	`las opciones de este mod ocupan ${optionBits} bits y entre todos los mods activos caben 8: sobrarán`,
	`the options of this mod take ${optionBits} bits and only 8 fit across all active mods: some will not fit`,
	`le opzioni di questa mod occupano ${optionBits} bit e tra tutte le mod attive ne entrano 8: alcune resteranno fuori`));

// --- city templates: every class must exist, or the random map is not generated and the game gives no warning -----------------
{
	const tf = files.find(x => lower(x) === 'patches/randommapsettlements/maps/1/map.obj.xml');
	if (tf && dataDir) {
		const known = new Set();
		const addClassFile = text => { const m = /<class\b[^>]*\bid\s*=\s*"([^"]+)"/i.exec(text); if (m) known.add(m[1].toLowerCase()); };
		const classDir = fs.readdirSync(dataDir).find(n => n.toLowerCase() === 'classes');
		if (classDir) {
			const walkClasses = d => { for (const e of fs.readdirSync(d, { withFileTypes: true })) { const p = path.join(d, e.name); if (e.isDirectory()) walkClasses(p); else if (/\.sc\.xml$/i.test(e.name)) addClassFile(fs.readFileSync(p, 'latin1')); } };
			walkClasses(path.join(dataDir, classDir));
		}
		for (const x of files.filter(x => /^data\/classes\/.*\.sc\.xml$/i.test(x))) addClassFile(read(x));
		const missing = new Map();
		read(tf).split('\n').forEach((l, i) => { const m = /class="([^"]+)"/.exec(l); if (m && !known.has(m[1].toLowerCase()) && !missing.has(m[1])) missing.set(m[1], i + 1); });
		for (const [c, line] of missing) error(tf, line, T(
			`la clase "${c}" no existe ni en el juego ni en el mod: el mapa aleatorio no se generaría`,
			`the class "${c}" exists neither in the game nor in the mod: the random map would not be generated`,
			`la classe "${c}" non esiste né nel gioco né nella mod: la mappa casuale non verrebbe generata`));
	}
}

// --- civilizations ----------------------------------------------------------------------------------------------------
const myCivs = new Set(), civIndexes = new Map();
for (const f of files.filter(f => /^civs\/[^/]+\.json$/i.test(f))) {
	let j;
	try { j = modernKeys(JSON.parse(read(f))); } catch (e) { error(f, 0, T('no es JSON válido: ', 'not valid JSON: ', 'JSON non valido: ') + e.message); continue; }
	if (!ID_RE.test(j.id || '')) error(f, 0, T(
		`el id "${j.id}" no vale (letras, números y _; empieza por letra; hasta 24)`,
		`the id "${j.id}" is not valid (letters, digits and _; must start with a letter; up to 24)`,
		`l'id "${j.id}" non è valido (lettere, numeri e _; inizia con una lettera; fino a 24)`));
	else myCivs.add(lower(j.id));
	const idx = Number(j.index);
	if (!(idx >= 10 && idx <= 99)) error(f, 0, T(
		'"index" falta o no está entre 10 y 99',
		'"index" is missing or not between 10 and 99',
		'"index" manca o non è tra 10 e 99'));
	else if (civIndexes.has(idx)) error(f, 0, T(
		`el índice ${idx} ya lo usa ${civIndexes.get(idx)}`,
		`index ${idx} is already used by ${civIndexes.get(idx)}`,
		`l'indice ${idx} è già usato da ${civIndexes.get(idx)}`));
	else civIndexes.set(idx, f);
	if (!GAME_CIVS.includes(j.base || 'Gaul')) error(f, 0, T(
		`"base" desconocida: ${j.base} (${GAME_CIVS.join(', ')})`,
		`unknown "base": ${j.base} (${GAME_CIVS.join(', ')})`,
		`"base" sconosciuta: ${j.base} (${GAME_CIVS.join(', ')})`));
	const biome = j.biome;
	if (biome !== undefined && !GAME_CIVS.includes(biome)) error(f, 0, T(
		`"bioma" desconocido: ${biome} (${GAME_CIVS.join(', ')})`,
		`unknown "bioma": ${biome} (${GAME_CIVS.join(', ')})`,
		`"bioma" sconosciuto: ${biome} (${GAME_CIVS.join(', ')})`));
	// Own city and village templates: "plantilla": "Dacian" looks for "Dacian Town N" and "Dacian Village N" in the patch at
	// patches/randommapsettlements/maps/1/map.obj.xml; whatever is missing is filled in from the base template
	const tpl = j.template;
	if (tpl !== undefined) {
		if (typeof tpl !== 'string' || !/^[A-Za-z][A-Za-z ]{1,30}$/.test(tpl)) error(f, 0, T(
			'"plantilla" es un texto (letras y espacios, p. ej. "Dacian")',
			'"plantilla" is a string (letters and spaces, e.g. "Dacian")',
			'"plantilla" è una stringa (lettere e spazi, es. "Dacian")'));
		else {
			const tf = files.find(x => lower(x) === 'patches/randommapsettlements/maps/1/map.obj.xml');
			if (!tf) warn(f, 0, T(
				`"plantilla": "${tpl}" pero el mod no trae patches/randommapsettlements/maps/1/map.obj.xml: se usarán las de la base`,
				`"plantilla": "${tpl}" but the mod has no patches/randommapsettlements/maps/1/map.obj.xml: the base ones will be used`,
				`"plantilla": "${tpl}" ma la mod non ha patches/randommapsettlements/maps/1/map.obj.xml: si useranno quelle della base`));
			else {
				const text = read(tf);
				for (const kind of ['Town', 'Village']) {
					if (!text.includes(`name="${tpl} ${kind}`)) warn(tf, 0, T(
						`no hay ninguna plantilla "${tpl} ${kind} N": las ${kind === 'Town' ? 'ciudades' : 'aldeas'} de esa civilización usarán las de la base`,
						`there is no template "${tpl} ${kind} N": the ${kind === 'Town' ? 'towns' : 'villages'} of that civilization will use the base ones`,
						`non c'è nessun modello "${tpl} ${kind} N": ${kind === 'Town' ? 'le città' : 'i villaggi'} di quella civiltà useranno quelli della base`));
				}
			}
		}
	}
	const letter = j.letter;
	if (letter !== undefined && String(letter).length !== 1) error(f, 0, T('"letra" es una sola letra', '"letra" is a single letter', '"letra" è una sola lettera'));
}

// --- specialties ------------------------------------------------------------------------------------------------------
const mySpecials = new Set(), specialBits = new Map();
for (const f of files.filter(f => /^specials\/[^/]+\.json$/i.test(f))) {
	let j;
	try { j = modernKeys(JSON.parse(read(f))); } catch (e) { error(f, 0, T('no es JSON válido: ', 'not valid JSON: ', 'JSON non valido: ') + e.message); continue; }
	if (!ID_RE.test(j.id || '')) error(f, 0, T(
		`el id "${j.id}" no vale (letras, números y _; empieza por letra; hasta 24)`,
		`the id "${j.id}" is not valid (letters, digits and _; must start with a letter; up to 24)`,
		`l'id "${j.id}" non è valido (lettere, numeri e _; inizia con una lettera; fino a 24)`));
	else if (GAME_SPECIALS.some(g => lower(g) === lower(j.id))) error(f, 0, T(
		`"${j.id}" es una especialidad del juego`,
		`"${j.id}" is a unit special of the game`,
		`"${j.id}" è una specialità del gioco`));
	else mySpecials.add(lower(j.id));
	const bit = Number(j.bit);
	if (!(bit >= 36 && bit <= 63)) error(f, 0, T(
		'"bit" falta o no está entre 36 y 63',
		'"bit" is missing or not between 36 and 63',
		'"bit" manca o non è tra 36 e 63'));
	else if (specialBits.has(bit)) error(f, 0, T(
		`el bit ${bit} ya lo usa ${specialBits.get(bit)}`,
		`bit ${bit} is already used by ${specialBits.get(bit)}`,
		`il bit ${bit} è già usato da ${specialBits.get(bit)}`));
	else specialBits.set(bit, f);
	if (!j.name) warn(f, 0, T('sin "name": se verá el id', 'no "name": the id will be shown', 'senza "name": si vedrà l\'id'));
}

// --- classes ----------------------------------------------------------------------------------------------------------
function checkClassFile(f, text, source) {
	const header = text.match(/<class\b[^>]*>/);
	if (!header) { warn(f, 0, T('no hay ninguna etiqueta <class>', 'there is no <class> tag', 'non c\'è nessun tag <class>')); return; }
	if (!/\bid\s*=\s*"[^"]+"/.test(header[0])) error(f, lineOf(text, header.index), T('<class> sin id', '<class> without an id', '<class> senza id'));
	for (const m of text.matchAll(/\brace\s*=\s*"([^"]*)"/g)) {
		if (!GAME_CIVS.includes(m[1]) && !myCivs.has(lower(m[1])) && !['None', 'Random', 'Mutable'].includes(m[1]))
			warn(f, lineOf(text, m.index), T(
				`race="${m[1]}" no es una civilización del juego ni de este mod`,
				`race="${m[1]}" is not a civilization of the game or of this mod`,
				`race="${m[1]}" non è una civiltà del gioco né di questa mod`));
	}
	for (const m of text.matchAll(/\bunit_specials\s*=\s*"([^"]*)"/g)) {
		for (const name of m[1].split(',').map(s => s.trim()).filter(Boolean)) {
			if (GAME_SPECIALS.includes(name) || mySpecials.has(lower(name))) continue;
			const near = GAME_SPECIALS.find(g => lower(g) === lower(name));
			if (near) error(f, lineOf(text, m.index), T(
				`"${name}": el juego distingue mayúsculas, se escribe "${near}"`,
				`"${name}": the game is case-sensitive, it is written "${near}"`,
				`"${name}": il gioco distingue le maiuscole, si scrive "${near}"`));
			else warn(f, lineOf(text, m.index), T(
				`especialidad "${name}" desconocida: el juego la ignora`,
				`unknown unit special "${name}": the game ignores it`,
				`specialità "${name}" sconosciuta: il gioco la ignora`));
		}
	}
	if (dataDir) {
		for (const m of text.matchAll(/<(?:method|behavior)\b[^>]*?\b(?:vs|script|verify|onfinish)\s*=\s*"([^"]+)"/g)) {
			const rel = m[1].replace(/\\/g, '/');
			if (!has(rel) && !gameFile(rel)) error(f, lineOf(text, m.index), T(
				`el script ${rel} no está en el mod ni en el juego`,
				`the script ${rel} is neither in the mod nor in the game`,
				`lo script ${rel} non è né nella mod né nel gioco`));
		}
	}
}
for (const f of files.filter(f => /^data\/classes\/.+\.sc\.xml$/i.test(f))) {
	const text = read(f);
	const bad = checkXml(text);
	if (bad) error(f, bad.line, T('XML mal formado: ', 'malformed XML: ', 'XML malformato: ') + bad.msg);
	else checkClassFile(f, text);
}

// --- orders -----------------------------------------------------------------------------------------------------------
for (const f of files.filter(f => /^data\/commands\/[^/]+\.xml$/i.test(f))) {
	const text = read(f);
	const bad = checkXml(text);
	if (bad) { error(f, bad.line, T('XML mal formado: ', 'malformed XML: ', 'XML malformato: ') + bad.msg); continue; }
	const names = new Set();
	for (const m of text.matchAll(/<cmd\b[^>]*?\bname\s*=\s*"([^"]*)"/g)) {
		if (names.has(m[1])) error(f, lineOf(text, m.index), T(
			`la orden "${m[1]}" está repetida en este archivo`,
			`the command "${m[1]}" is repeated in this file`,
			`il comando "${m[1]}" è ripetuto in questo file`));
		names.add(m[1]);
	}
}

// --- patches ----------------------------------------------------------------------------------------------------------
for (const f of files.filter(f => /^patches\//i.test(f))) {
	const target = f.slice('patches/'.length);
	const ext = lower(path.extname(f));
	if (ext !== '.xml' && ext !== '.ini' && ext !== '.vs') { error(f, 0, T(
		'solo se parchean archivos .xml, .ini y .vs',
		'only .xml, .ini and .vs files can be patched',
		'si possono patchare solo file .xml, .ini e .vs')); continue; }
	if (dataDir && /^data\//i.test(target) && !gameFile(target)) warn(f, 0, T(
		`el juego no tiene ${target}: el parche no se aplicará`,
		`the game has no ${target}: the patch will not be applied`,
		`il gioco non ha ${target}: la patch non verrà applicata`));
	const text = read(f);
	if (ext === '.ini') {
		// The engine only recognizes comments that take up the whole line: a ';' after a key or a section breaks the patch
		text.split('\n').forEach((l, i) => {
			const t = l.trim();
			if (t && !t.startsWith(';') && t.includes(';')) warn(f, i + 1, T(
				'comentario al final de línea: el motor no lo quita ("' + t.slice(0, 40) + '…"); ponlo en su propia línea',
				'comment at the end of a line: the engine does not strip it ("' + t.slice(0, 40) + '…"); put it on its own line',
				'commento a fine riga: il motore non lo toglie ("' + t.slice(0, 40) + '…"); mettilo su una riga a parte'));
		});
	}
	if (ext === '.vs') {
		let open = null, bodyLines = 0;
		let firstCode = null;
		const closeBlock = () => {
			if (open && !bodyLines) warn(f, open.line, T(`${open.word} sin código debajo`, `${open.word} has no code below it`, `${open.word} senza codice sotto`));
			// scripts declare all their variables at the top: the code of a @top that is not a declaration ends up before the script's own and does not compile
			if (open && open.word === '@top' && firstCode && !/^\w+\s+\w+(\s*,\s*\w+)*\s*;/.test(firstCode)) warn(f, open.line, T(
				'@top pone código antes de las declaraciones de variables del script y no compilaría: usa @before con su primera línea de código',
				'@top puts code before the script\'s variable declarations and would not compile: use @before with its first line of code',
				'@top mette codice prima delle dichiarazioni delle variabili dello script e non compilerebbe: usa @before con la sua prima riga di codice'));
		};
		text.split('\n').forEach((raw, i) => {
			const l = raw.replace(/\r$/, '');
			if (!l.startsWith('@')) { if (open && l.trim()) { bodyLines++; if (!firstCode && !l.trim().startsWith('//')) firstCode = l.trim(); } return; }
			closeBlock();
			const m = l.match(/^@(\S*)\s*(.*)$/);
			const word = LEGACY_DIRECTIVES[m[1]] || m[1];
			if (word === 'end') { open = null; bodyLines = 0; return; }
			if (!['top', 'bottom', 'before', 'after', 'replace'].includes(word)) {
				error(f, i + 1, T(
					`la directiva @${word} no existe (@top, @bottom, @before, @after, @replace, @end)`,
					`the directive @${word} does not exist (@top, @bottom, @before, @after, @replace, @end)`,
					`la direttiva @${word} non esiste (@top, @bottom, @before, @after, @replace, @end)`));
				open = null; bodyLines = 0; return;
			}
			if (['before', 'after', 'replace'].includes(word) && !m[2].trim()) error(f, i + 1, T(
				`@${word} necesita el texto de la línea que se busca`,
				`@${word} needs the text of the line to look for`,
				`@${word} richiede il testo della riga da cercare`));
			open = { word: '@' + word, line: i + 1 };
			bodyLines = 0;
			firstCode = null;
		});
		closeBlock();
		continue;
	}
	if (ext === '.xml') {
		const bad = checkXml(text);
		if (bad) { error(f, bad.line, T('parche mal formado, no se aplicaría entero: ', 'malformed patch, it would not be applied at all: ', 'patch malformata, non verrebbe applicata per intero: ') + bad.msg); continue; }
		if (!/^\s*(<\?[^>]*\?>\s*)?(<!--[\s\S]*?-->\s*)*<patch\b/.test(text)) error(f, 1, T(
			'el parche tiene que empezar por <patch>',
			'the patch has to start with <patch>',
			'la patch deve iniziare con <patch>'));
		for (const m of text.matchAll(/<(set|add|remove|replace)\b([^>]*)>/g)) {
			const sel = m[2].match(/\bselect\s*=\s*("([^"]*)"|'([^']*)')/);
			if (!sel) { error(f, lineOf(text, m.index), T(`<${m[1]}> sin select`, `<${m[1]}> without select`, `<${m[1]}> senza select`)); continue; }
			const s = sel[2] ?? sel[3];
			if (!s.startsWith('/')) warn(f, lineOf(text, m.index), T(
				`el selector "${s}" no empieza por / o //: el juego lo acepta, pero conviene escribirlo con /`,
				`the selector "${s}" does not start with / or //: the game accepts it, but it is better to write it with /`,
				`il selettore "${s}" non inizia con / o //: il gioco lo accetta, ma conviene scriverlo con /`));
			if ((s.match(/\[/g) || []).length !== (s.match(/\]/g) || []).length) error(f, lineOf(text, m.index), T(
				`selector con corchetes sin cerrar: ${s}`,
				`selector with unclosed brackets: ${s}`,
				`selettore con parentesi quadre non chiuse: ${s}`));
		}
		if (/<patch\b[\s\S]*<(?!\/?(?:patch|set|add|remove|replace)\b)/.test(text.replace(/<(add|replace)\b[\s\S]*?<\/\1>/g, '').replace(/<!--[\s\S]*?-->/g, ''))) {
			warn(f, 0, T(
				'dentro de <patch> hay etiquetas que no son operaciones (set, add, remove, replace)',
				'inside <patch> there are tags that are not operations (set, add, remove, replace)',
				'dentro <patch> ci sono tag che non sono operazioni (set, add, remove, replace)'));
		}
	}
	else {
		text.split('\n').forEach((raw, i) => {
			const l = raw.replace(/\r$/, '');
			if (!l.trim() || /^\s*;/.test(l) || /^\s*\[[^\]]+\]\s*(;.*)?$/.test(l) || /^\s*[-+]\S/.test(l) || /^[^=;]+=/.test(l)) return;
			warn(f, i + 1, T(
				`línea de parche INI que no se entiende: "${l.trim().slice(0, 40)}"`,
				`INI patch line that is not understood: "${l.trim().slice(0, 40)}"`,
				`riga di patch INI che non si capisce: "${l.trim().slice(0, 40)}"`));
		});
		// Game bug: a new section in UNIT_SPECIALS.INI overflows its memory
		if (/^unit_specials\.ini$/i.test(path.basename(f))) {
			for (const m of text.matchAll(/^\s*\[([^\]]+)\]/gm)) {
				if (!GAME_SPECIALS.some(g => lower(g) === lower(m[1])))
					error(f, lineOf(text, m.index), T(
						`la sección [${m[1]}] no es de una especialidad del juego: el juego escribe fuera de su memoria y puede cerrarse`,
						`the section [${m[1]}] is not a game unit special: the game writes outside its memory and may crash`,
						`la sezione [${m[1]}] non è di una specialità del gioco: il gioco scrive fuori dalla sua memoria e può chiudersi`));
			}
		}
	}
}
for (const f of files.filter(f => /(^|\/)unit_specials\.ini$/i.test(f) && !/^patches\//i.test(f))) {
	for (const m of read(f).matchAll(/^\s*\[([^\]]+)\]/gm)) {
		if (!GAME_SPECIALS.some(g => lower(g) === lower(m[1])))
			error(f, 0, T(
				`la sección [${m[1]}] no es de una especialidad del juego: el juego escribe fuera de su memoria y puede cerrarse`,
				`the section [${m[1]}] is not a game unit special: the game writes outside its memory and may crash`,
				`la sezione [${m[1]}] non è di una specialità del gioco: il gioco scrive fuori dalla sua memoria e può chiudersi`));
	}
}

// --- texts ------------------------------------------------------------------------------------------------------------
for (const f of files.filter(f => /^loc\/.+\.xml$/i.test(f))) {
	const text = read(f);
	const bad = checkXml(text);
	if (bad) { error(f, bad.line, T('XML mal formado: ', 'malformed XML: ', 'XML malformato: ') + bad.msg); continue; }
	for (const m of text.matchAll(/<translationtableentry\b([^>]*)>/g)) {
		if (!/\btext\s*=/.test(m[1]) || !/\bresult\s*=/.test(m[1])) error(f, lineOf(text, m.index), T(
			'translationtableentry necesita text y result',
			'translationtableentry needs text and result',
			'translationtableentry richiede text e result'));
	}
}

// --- scripts ----------------------------------------------------------------------------------------------------------
const typesOf = sig => sig.split(',').map(s => s.trim().split(/\s+/)[0]).filter(Boolean);
for (const f of files.filter(f => /\.vs$/i.test(f) && !/^patches\//i.test(f))) {  // script patches carry no signature: they are pieces of code with directives
	const text = read(f);
	const first = text.split('\n')[0].replace(/\r$/, '');
	if (!/^\s*\/\/\s*\S+/.test(first)) {
		error(f, 1, T(
			'la primera línea tiene que ser un comentario con la firma, por ejemplo "// void, Obj This"',
			'the first line has to be a comment with the signature, for example "// void, Obj This"',
			'la prima riga deve essere un commento con la firma, per esempio "// void, Obj This"'));
	}
	const ev = f.match(/^events\/([^/]+)\.vs$/i);
	if (ev) {
		const name = Object.keys(EVENTS).find(e => lower(e) === lower(ev[1]));
		if (!name) warn(f, 0, T(
			`"${ev[1]}" no es un evento conocido (${Object.keys(EVENTS).join(', ')})`,
			`"${ev[1]}" is not a known event (${Object.keys(EVENTS).join(', ')})`,
			`"${ev[1]}" non è un evento noto (${Object.keys(EVENTS).join(', ')})`));
		else {
			const got = typesOf(first.replace(/^\s*\/\/\s*/, ''));
			if (got.join(',') !== EVENTS[name].join(',')) error(f, 1, T(
				`la firma de ${name} es "// ${EVENTS[name].join(', ')}"` + ' con los nombres de parámetros que quieras',
				`the signature of ${name} is "// ${EVENTS[name].join(', ')}"` + ' with whatever parameter names you like',
				`la firma di ${name} è "// ${EVENTS[name].join(', ')}"` + ' con i nomi dei parametri che vuoi'));
			if (lower(ev[1]) !== lower(name) || ev[1] !== name) warn(f, 0, T(
				`el nombre del archivo debería ser exactamente ${name}.vs`,
				`the file name should be exactly ${name}.vs`,
				`il nome del file dovrebbe essere esattamente ${name}.vs`));
		}
		if (/\b(Sleep|Wait)\s*\(/.test(text) && name && name !== 'OnGameStart') error(f, 0, T(
			`${name} se ejecuta al momento: no puede usar Sleep`,
			`${name} runs immediately: it cannot use Sleep`,
			`${name} viene eseguito subito: non può usare Sleep`));
	}
	for (const m of text.matchAll(/\b(Mod_[A-Za-z]+)\s*\(/g)) {
		if (!MOD_FUNCTIONS.includes(m[1])) error(f, lineOf(text, m.index), T(
			`la función ${m[1]} no existe en el SDK (${MOD_FUNCTIONS.join(', ')})`,
			`the function ${m[1]} does not exist in the SDK (${MOD_FUNCTIONS.join(', ')})`,
			`la funzione ${m[1]} non esiste nell'SDK (${MOD_FUNCTIONS.join(', ')})`));
	}
	if (/\bMod_(Set)?AttackDamage\b/.test(text) && !(ev && /^OnAttackResolved$/i.test(ev[1]))) {
		warn(f, 0, T(
			'Mod_AttackDamage / Mod_SetAttackDamage solo hacen algo dentro del script del evento OnAttackResolved',
			'Mod_AttackDamage / Mod_SetAttackDamage only do something inside the OnAttackResolved event script',
			'Mod_AttackDamage / Mod_SetAttackDamage fanno qualcosa solo dentro lo script dell\'evento OnAttackResolved'));
	}
	for (const m of text.matchAll(/\bMod_(?:Has|Set)Special\s*\(\s*([A-Za-z_][A-Za-z0-9_]*)\b(?!\s*\.)/g)) {
		if (/^(attacker|victim|killer|this|me|obj)$/i.test(m[1])) warn(f, lineOf(text, m.index), T(
			'Mod_HasSpecial / Mod_SetSpecial reciben una Unit: usa objeto.AsUnit()',
			'Mod_HasSpecial / Mod_SetSpecial take a Unit: use object.AsUnit()',
			'Mod_HasSpecial / Mod_SetSpecial ricevono una Unit: usa oggetto.AsUnit()'));
	}
}

// --- encoding and references (failures the game does not report) ------------------------------------------------------------
// The game reads class XMLs as UTF-8: an accented letter saved as Latin-1 makes the class not load, with no warning other than a line in its log
const readBuf = rel => fs.readFileSync(path.join(modDir, rel));
const validUtf8 = buf => { try { new TextDecoder('utf-8', { fatal: true }).decode(buf); return true; } catch (e) { return false; } };
for (const f of files.filter(f => /\.(xml|vs|json|ini)$/i.test(f))) {
	if (validUtf8(readBuf(f))) continue;
	if (/^data\/classes\/.+\.sc\.xml$/i.test(f)) error(f, 0, T(
		'no está en UTF-8 (letras con tilde en Latin-1): el juego no puede leer la clase y no se carga. Guárdalo como UTF-8',
		'not in UTF-8 (accented letters in Latin-1): the game cannot read the class and it does not load. Save it as UTF-8',
		'non è in UTF-8 (lettere accentate in Latin-1): il gioco non può leggere la classe e non viene caricata. Salvalo come UTF-8'));
	else warn(f, 0, T(
		'no está en UTF-8 (letras con tilde en Latin-1): guárdalo como UTF-8',
		'not in UTF-8 (accented letters in Latin-1): save it as UTF-8',
		'non è in UTF-8 (lettere accentate in Latin-1): salvalo come UTF-8'));
}

// Ids of the mod's classes (and the game's, with --data)
const myClassIds = new Set();
const classHeaders = new Map();  // id -> { file, header }
for (const f of files.filter(f => /^data\/classes\/.+\.sc\.xml$/i.test(f))) {
	const h = read(f).match(/<class\b[^>]*>/);
	const id = h && (h[0].match(/\bid\s*=\s*"([^"]+)"/) || [])[1];
	if (id) { myClassIds.add(lower(id)); classHeaders.set(lower(id), { file: f, header: h[0] }); }
}
let gameClassIds = null;
function classExists(id) {
	if (myClassIds.has(lower(id))) return true;
	if (!dataDir) return null;  // unknown
	if (!gameClassIds) {
		gameClassIds = new Set();
		const dir = gameFile('data/classes');
		if (dir) {
			for (const g of fs.readdirSync(dir)) {
				if (!/\.sc\.xml$/i.test(g)) continue;
				const h = fs.readFileSync(path.join(dir, g), 'latin1').match(/<class\b[^>]*>/);
				const gid = h && (h[0].match(/\b(?:id|altid)\s*=\s*"([^"]+)"/g) || []).map(s => s.replace(/^[a-z]+\s*=\s*"|"$/gi, ''));
				if (gid) gid.forEach(x => gameClassIds.add(lower(x)));
			}
		}
	}
	return gameClassIds.has(lower(id));
}

// A class without an entity is created with no model and disappears (the game does not inherit entity from the parent)
const parents = new Set([...classHeaders.values()].map(c => (c.header.match(/\bparent\s*=\s*"([^"]+)"/) || [])[1]).filter(Boolean).map(lower));
for (const [id, c] of classHeaders) {
	if (/\bentity\w*\s*=/.test(c.header) || parents.has(id)) continue;
	if (/\bcpp_class\s*=\s*"CVX(Unit|Building|Outpost|TownHall|Hero|Gate|Tower)\b/.test(c.header)) {
		warn(c.file, lineOf(read(c.file), read(c.file).indexOf(c.header)), T(
			`la clase ${id} no tiene entity (el modelo): el juego no lo hereda del padre y se crearía sin modelo y desaparecería`,
			`the class ${id} has no entity (the model): the game does not inherit it from the parent, so it would be created without a model and vanish`,
			`la classe ${id} non ha entity (il modello): il gioco non lo eredita dal genitore e verrebbe creata senza modello e sparirebbe`));
	}
}

// Random map generation names classes in placeobjects.xml: if one does not exist, it hangs, with no warning
for (const f of files.filter(f => /^(patches\/)?data\/randommap\/placeobjects\.xml$/i.test(f))) {
	const text = read(f);
	for (const m of text.matchAll(/<GameObj\b[^>]*?\bclass\s*=\s*"([^"]+)"/g)) {
		if (m[1].includes('::')) continue;
		if (classExists(m[1]) === false) error(f, lineOf(text, m.index), T(
			`la clase ${m[1]} no existe: el mapa aleatorio se quedaría colgado al generarse`,
			`the class ${m[1]} does not exist: the random map would hang while being generated`,
			`la classe ${m[1]} non esiste: la mappa casuale si bloccherebbe durante la generazione`));
	}
}

// A civilization's "alias" tables (used by the AI) must point to classes and orders that exist
const myCommandNames = new Set();
for (const f of files.filter(f => /^data\/commands\/[^/]+\.xml$/i.test(f))) {
	for (const m of read(f).matchAll(/<cmd\b[^>]*?\bname\s*=\s*"([^"]*)"/g)) myCommandNames.add(m[1]);
}
for (const f of files.filter(f => /^civs\/[^/]+\.json$/i.test(f))) {
	let j;
	try { j = modernKeys(JSON.parse(read(f))); } catch (e) { continue; }
	if (j.ai === true) {
		if (!j.alias && !j.ownAi) warn(f, 0, T(
			'"ia": true sin tabla "alias": los guiones de IA del juego no encontrarán tus clases ni órdenes. La herramienta de clonado la genera',
			'"ia": true without an "alias" table: the game\'s AI scripts will not find your classes or commands. The cloning tool generates it',
			'"ia": true senza tabella "alias": gli script IA del gioco non troveranno le tue classi né i tuoi comandi. Lo strumento di clonazione la genera'));
	}
	for (const [base, clone] of Object.entries((j.alias && j.alias.classes) || {})) {
		if (!myClassIds.has(lower(clone))) error(f, 0, T(
			`alias.classes: "${base}" -> "${clone}", pero el mod no tiene esa clase`,
			`alias.classes: "${base}" -> "${clone}", but the mod does not have that class`,
			`alias.classes: "${base}" -> "${clone}", ma la mod non ha quella classe`));
	}
	for (const [base, clone] of Object.entries((j.alias && j.alias.commands) || {})) {
		if (!myCommandNames.has(clone)) error(f, 0, T(
			`alias.commands: "${base}" -> "${clone}", pero el mod no tiene esa orden`,
			`alias.commands: "${base}" -> "${clone}", but the mod does not have that command`,
			`alias.commands: "${base}" -> "${clone}", ma la mod non ha quel comando`));
	}
}

// --- graphics: .rle, .pass and entities (.ent.xml) ----------------------------------------------------------------------
// An image that does not exist, a corrupt .rle or a repeated idx makes the game crash (or hang) when loading the class, with no warning.
const rleLib = require('./lib/rle'), passLib = require('./lib/pass');
// Names of the game's graphics .pak files (only with --game): lowercase, with "/"
let gamePaths = null;
function gameHas(rel) {
	if (!gameDir) return null;
	if (!gamePaths) {
		gamePaths = new Set();
		for (const pk of ['Units', 'Buildings', 'MapObjects', 'Visuals']) {
			const pf = path.join(gameDir, 'Packs', pk + '.pak');
			if (!fs.existsSync(pf)) continue;
			const d = fs.readFileSync(pf), count = d.readUInt32LE(0x20);
			let p = 0x28, prev = '';
			for (let i = 0; i < count; i++) {
				const len = d[p], shared = d[p + 1];
				p += 2;
				const name = prev.slice(0, shared) + d.slice(p, p + len - shared).toString('latin1');
				p += len - shared + 8;
				prev = name;
				gamePaths.add(name.toLowerCase().replace(/\\/g, '/'));
			}
		}
		if (!gamePaths.size) { console.log(LABEL_WARN() + T(
			'  --juego: no se encontraron los .pak en ' + path.join(gameDir, 'Packs') + '; no se comprobarán las imágenes del juego',
			'  --juego: the .pak files were not found in ' + path.join(gameDir, 'Packs') + '; the game\'s images will not be checked',
			'  --juego: non si sono trovati i .pak in ' + path.join(gameDir, 'Packs') + '; le immagini del gioco non verranno controllate')); gamePaths = null; return null; }
	}
	return gamePaths.has(lower(rel));
}
const rleCache = new Map(); // mod path (lowercase) -> sheet read | null if it is wrong
for (const f of files.filter(f => /\.rle$/i.test(f))) {
	let sheet = null;
	try {
		sheet = rleLib.parse(readBuf(f));
		if (sheet.cells.some(c => !c.empty && !c.data)) { error(f, 0, T(
			'es un .rle.mmp con otro nombre (sus píxeles están en el rle.mmp del juego): desde un mod no se puede cargar; hace falta un .rle con los píxeles dentro (convert-sprite.js)',
			'it is a .rle.mmp under another name (its pixels are in the game\'s rle.mmp): it cannot be loaded from a mod; you need a .rle with the pixels inside (convert-sprite.js)',
			'è un .rle.mmp con un altro nome (i suoi pixel sono nel rle.mmp del gioco): da una mod non si può caricare; serve un .rle con i pixel dentro (convert-sprite.js)')); sheet = null; }
		else {
			if (sheet.trailing !== 0) warn(f, 0, T(
				`sobran ${sheet.trailing} bytes al final del .rle`,
				`${sheet.trailing} extra bytes at the end of the .rle`,
				`${sheet.trailing} byte in più alla fine del .rle`));
			if (sheet.mode === rleLib.MODE_PAL || sheet.mode === rleLib.MODE_PLAYER) { if (sheet.paletteCount === 0) error(f, 0, T(
				'modo con paleta pero sin colores en la paleta: el juego leería un puntero nulo',
				'palette mode but no colors in the palette: the game would read a null pointer',
				'modalità con tavolozza ma senza colori nella tavolozza: il gioco leggerebbe un puntatore nullo')); }
			if (sheet.cells.every(c => c.empty)) warn(f, 0, T(
				'todos los fotogramas del .rle están vacíos',
				'all the frames of the .rle are empty',
				'tutti i fotogrammi del .rle sono vuoti'));
			for (const c of sheet.cells) if (!c.empty) {
				try { rleLib.decodeFrame(c); } catch (e) { error(f, 0, T(
					'un fotograma no se puede decodificar (.rle corrupto): ',
					'a frame cannot be decoded (corrupt .rle): ',
					'un fotogramma non si può decodificare (.rle corrotto): ') + e.message); sheet = null; break; }
			}
			if (sheet && sheet.rows > 62) warn(f, 0, T(
				`${sheet.rows} filas: el juego admite 64 fotogramas por animación incluidos los 2 centinelas (62 filas)`,
				`${sheet.rows} rows: the game allows 64 frames per animation including the 2 sentinels (62 rows)`,
				`${sheet.rows} righe: il gioco ammette 64 fotogrammi per animazione inclusi i 2 sentinella (62 righe)`));
		}
	}
	catch (e) { error(f, 0, T(
		'.rle no válido (el juego se caería al cargarlo): ',
		'invalid .rle (the game would crash loading it): ',
		'.rle non valido (il gioco si chiuderebbe caricandolo): ') + e.message); }
	rleCache.set(lower(f), sheet);
}
for (const f of files.filter(f => /\.pass$/i.test(f))) {
	try {
		const g = passLib.decode(readBuf(f));
		if (!g.cells.some(Boolean)) warn(f, 0, T(
			'la huella está vacía: el edificio no ocupará sitio en el mapa',
			'the footprint is empty: the building will take up no space on the map',
			'l\'impronta è vuota: l\'edificio non occuperà spazio sulla mappa'));
	} catch (e) { error(f, 0, T('.pass no válido: ', 'invalid .pass: ', '.pass non valido: ') + e.message); }
}

for (const f of files.filter(f => /\.ent\.xml$/i.test(f))) {
	const text = read(f);
	const bad = checkXml(text);
	if (bad) { error(f, bad.line, T('XML mal formado: ', 'malformed XML: ', 'XML malformato: ') + bad.msg); continue; }
	const dir = path.posix.dirname(f);
	const modOwnsSprites = files.some(x => /\.rle$/i.test(x) && path.posix.dirname(x) === dir); // the folder brings its own .rle files: it is a new entity
	const ent = text.match(/<entity\b[^>]*>/);
	const variations = ent && (ent[0].match(/\bvariations\s*=\s*"(\d+)"/) || [])[1];
	const passFile = ent && (ent[0].match(/\bpass_file\s*=\s*"([^"]*)"/) || [])[1];
	if (passFile) {
		const pf = (dir + '/' + passFile).replace(/^\.\//, '');
		if (!has(pf) && gameHas(pf) !== true) warn(f, lineOf(text, ent.index), T(
			`pass_file="${passFile}": no está ${pf} (la huella quedaría vacía y el edificio no ocuparía sitio)`,
			`pass_file="${passFile}": ${pf} is not there (the footprint would be empty and the building would take up no space)`,
			`pass_file="${passFile}": ${pf} non c'è (l'impronta resterebbe vuota e l'edificio non occuperebbe spazio)`));
	}
	// images
	const images = new Map(); // idx -> {rows, columns, file}
	let nImages = 0;
	for (const m of text.matchAll(/<image\b[^>]*>/g)) {
		const line = lineOf(text, m.index);
		nImages++;
		const idx = (m[0].match(/\bidx\s*=\s*"(\d+)"/) || [])[1];
		const file = (m[0].match(/\bfile\s*=\s*"([^"]*)"/) || [])[1];
		const cols = (m[0].match(/\bcolumns\s*=\s*"(\d+)"/) || [])[1];
		const rows = (m[0].match(/\brows\s*=\s*"(\d+)"/) || [])[1];
		if (!idx || Number(idx) < 1 || Number(idx) > 32) error(f, line, T(
			`<image> con idx ${idx ?? '(falta)'}: tiene que estar entre 1 y 32 (el juego se cae al cargar)`,
			`<image> with idx ${idx ?? '(missing)'}: it has to be between 1 and 32 (the game crashes on load)`,
			`<image> con idx ${idx ?? '(manca)'}: deve essere tra 1 e 32 (il gioco si chiude al caricamento)`));
		else if (images.has(idx)) error(f, line, T(
			`<image> con idx ${idx} repetido (el juego se cae al cargar)`,
			`<image> with idx ${idx} repeated (the game crashes on load)`,
			`<image> con idx ${idx} ripetuto (il gioco si chiude al caricamento)`));
		else images.set(idx, { rows: Number(rows), columns: Number(cols), file });
		if (!file) { error(f, line, T(
			'<image> sin file (el juego no carga la entidad)',
			'<image> without file (the game does not load the entity)',
			'<image> senza file (il gioco non carica l\'entità)')); continue; }
		const base = (dir + '/' + file).replace(/^\.\//, '').replace(/\.rle$/i, '');
		// the game tries: the path as is (.rle/.bmp), .rle, .rle.mmp, .mmp, .bmp
		const inMod = ['.rle', '.bmp'].some(ext => has(base + ext)) || has(base) || ['.rle.mmp', '.mmp'].some(ext => has(base + ext));
		const inGame = gameHas(base + '.rle.mmp') === true || gameHas(base + '.rle') === true || gameHas(base + '.bmp') === true;
		if (!inMod && !inGame) {
			const msg = T(
				`no existe la imagen ${base}.rle (ni .bmp) en el mod${gameDir ? ' ni en el juego' : ''}: el juego se CUELGA al cargar la clase`,
				`the image ${base}.rle (or .bmp) does not exist in the mod${gameDir ? ' or in the game' : ''}: the game HANGS when loading the class`,
				`l'immagine ${base}.rle (né .bmp) non esiste nella mod${gameDir ? ' né nel gioco' : ''}: il gioco si BLOCCA caricando la classe`);
			if (modOwnsSprites || gameDir) error(f, line, msg); else warn(f, line, msg + T(
				' (a no ser que sea una imagen del propio juego; con --juego se comprueba)',
				' (unless it is one of the game\'s own images; --juego checks that)',
				' (a meno che non sia un\'immagine del gioco stesso; con --juego si controlla)'));
		}
		else if (gameDir && has(base + '.rle') && gameHas(base + '.rle.mmp') === true) {
			warn(f, line, T(
				`el juego ya tiene ${base}.rle.mmp en un .pak y gana al .rle del mod: tu .rle se ignora. Usa otro nombre de carpeta o de archivo`,
				`the game already has ${base}.rle.mmp in a .pak and it wins over the mod's .rle: your .rle is ignored. Use another folder or file name`,
				`il gioco ha già ${base}.rle.mmp in un .pak e vince sul .rle della mod: il tuo .rle viene ignorato. Usa un altro nome di cartella o di file`));
		}
		if (variations && cols && cols !== variations) warn(f, line, T(
			`columns="${cols}" no coincide con variations="${variations}" de <entity> (el juego espera una columna por dirección)`,
			`columns="${cols}" does not match variations="${variations}" of <entity> (the game expects one column per direction)`,
			`columns="${cols}" non coincide con variations="${variations}" di <entity> (il gioco si aspetta una colonna per direzione)`));
		// rows/columns against the header of the .rle itself
		const sheet = rleCache.get(lower(base + '.rle')) || rleCache.get(lower(base));
		if (sheet) {
			if (cols && Number(cols) !== sheet.cols) warn(f, line, T(
				`columns="${cols}" pero ${base}.rle tiene ${sheet.cols} columnas`,
				`columns="${cols}" but ${base}.rle has ${sheet.cols} columns`,
				`columns="${cols}" ma ${base}.rle ha ${sheet.cols} colonne`));
			if (rows && Number(rows) !== sheet.rows) warn(f, line, T(
				`rows="${rows}" pero ${base}.rle tiene ${sheet.rows} filas`,
				`rows="${rows}" but ${base}.rle has ${sheet.rows} rows`,
				`rows="${rows}" ma ${base}.rle ha ${sheet.rows} righe`));
		}
	}
	if (nImages > 32) error(f, 0, T(`${nImages} imágenes: el motor admite 32`, `${nImages} images: the engine allows 32`, `${nImages} immagini: il motore ne ammette 32`));
	// layers
	let nLayers = 0;
	const layerIdx = new Set();
	for (const m of text.matchAll(/<layer\b[^>]*>/g)) {
		const line = lineOf(text, m.index);
		nLayers++;
		const idx = (m[0].match(/\bidx\s*=\s*"(\d+)"/) || [])[1], img = (m[0].match(/\bimage\s*=\s*"(\d+)"/) || [])[1];
		if (!idx || Number(idx) < 1 || Number(idx) > 32) error(f, line, T(
			`<layer> con idx ${idx ?? '(falta)'}: tiene que estar entre 1 y 32`,
			`<layer> with idx ${idx ?? '(missing)'}: it has to be between 1 and 32`,
			`<layer> con idx ${idx ?? '(manca)'}: deve essere tra 1 e 32`));
		else if (layerIdx.has(idx)) error(f, line, T(`<layer> con idx ${idx} repetido`, `<layer> with idx ${idx} repeated`, `<layer> con idx ${idx} ripetuto`));
		else layerIdx.add(idx);
		if (img && img !== '0' && !images.has(img)) error(f, line, T(
			`<layer> usa image="${img}" y no hay ninguna <image idx="${img}"> (el juego se cae al cargar)`,
			`<layer> uses image="${img}" and there is no <image idx="${img}"> (the game crashes on load)`,
			`<layer> usa image="${img}" e non c'è nessuna <image idx="${img}"> (il gioco si chiude al caricamento)`));
	}
	if (nLayers > 32) error(f, 0, T(`${nLayers} capas: el motor admite 32`, `${nLayers} layers: the engine allows 32`, `${nLayers} livelli: il motore ne ammette 32`));
	// states
	const states = [...text.matchAll(/<state\b[^>]*>/g)];
	if (states.length > 8) error(f, 0, T(`${states.length} estados: el motor admite 8`, `${states.length} states: the engine allows 8`, `${states.length} stati: il motore ne ammette 8`));
	for (const m of states) {
		const idx = (m[0].match(/\bidx\s*=\s*"(\d+)"/) || [])[1];
		if (!idx || Number(idx) < 1 || Number(idx) > 8) error(f, lineOf(text, m.index), T(
			`<state> con idx ${idx ?? '(falta)'}: tiene que estar entre 1 y 8`,
			`<state> with idx ${idx ?? '(missing)'}: it has to be between 1 and 8`,
			`<state> con idx ${idx ?? '(manca)'}: deve essere tra 1 e 8`));
	}
	// animations
	const animIdx = new Set();
	for (const m of text.matchAll(/<anim\b[^>]*>[\s\S]*?<\/anim>/g)) {
		const line = lineOf(text, m.index), head = m[0].match(/<anim\b[^>]*>/)[0];
		const aidx = (head.match(/\bidx\s*=\s*"(\d+)"/) || [])[1];
		if (!aidx || Number(aidx) < 1 || Number(aidx) > 28) error(f, line, T(
			`<anim> con idx ${aidx ?? '(falta)'}: tiene que estar entre 1 y 28`,
			`<anim> with idx ${aidx ?? '(missing)'}: it has to be between 1 and 28`,
			`<anim> con idx ${aidx ?? '(manca)'}: deve essere tra 1 e 28`));
		else if (animIdx.has(aidx)) error(f, line, T(`<anim> con idx ${aidx} repetido`, `<anim> with idx ${aidx} repeated`, `<anim> con idx ${aidx} ripetuto`));
		else animIdx.add(aidx);
		const frames = Number((head.match(/\bframes\s*=\s*"(\d+)"/) || [])[1]);
		const frameTags = [...m[0].matchAll(/<frame\b[^>]*>/g)];
		if (frames > 64) error(f, line, T(
			`frames="${frames}": el motor admite 64 como máximo`,
			`frames="${frames}": the engine allows 64 at most`,
			`frames="${frames}": il motore ne ammette 64 al massimo`));
		if (frames && frameTags.length && frameTags.length !== frames) warn(f, line, T(
			`la animación dice frames="${frames}" pero lleva ${frameTags.length} <frame>`,
			`the animation says frames="${frames}" but has ${frameTags.length} <frame>`,
			`l'animazione dice frames="${frames}" ma ha ${frameTags.length} <frame>`));
		frameTags.forEach((t, k) => {
			const fi = Number((t[0].match(/\bidx\s*=\s*"(\d+)"/) || [])[1]);
			if (frames && (fi < 1 || fi > frames)) error(f, line, T(
				`<frame idx="${fi}" fuera de 1..${frames}: el juego no carga la entidad`,
				`<frame idx="${fi}" outside 1..${frames}: the game does not load the entity`,
				`<frame idx="${fi}" fuori da 1..${frames}: il gioco non carica l'entità`));
		});
		if (frameTags.length >= 2) {
			const d0 = (frameTags[0][0].match(/\bduration\s*=\s*"(\d+)"/) || [])[1], d1 = (frameTags[frameTags.length - 1][0].match(/\bduration\s*=\s*"(\d+)"/) || [])[1];
			if (d0 !== '0' || d1 !== '0') warn(f, line, T(
				'el primer y el último <frame> de una animación son los dos centinelas y llevan duration="0" (así lo hacen todas las del juego)',
				'the first and last <frame> of an animation are the two sentinels and carry duration="0" (all the game\'s animations do this)',
				'il primo e l\'ultimo <frame> di un\'animazione sono i due sentinella e hanno duration="0" (così fanno tutte quelle del gioco)'));
		}
		for (const r of m[0].matchAll(/<replace\b[^>]*>/g)) {
			const lay = (r[0].match(/\blayer\s*=\s*"(\d+)"/) || [])[1], img = (r[0].match(/\bimage\s*=\s*"(\d+)"/) || [])[1];
			if (lay && !layerIdx.has(lay)) error(f, line, T(
				`<replace layer="${lay}"> y no hay ninguna <layer idx="${lay}">`,
				`<replace layer="${lay}"> and there is no <layer idx="${lay}">`,
				`<replace layer="${lay}"> e non c'è nessun <layer idx="${lay}">`));
			if (img && img !== '0') {
				const im = images.get(img);
				if (!im) error(f, line, T(
					`<replace image="${img}"> y no hay ninguna <image idx="${img}"> (el juego se cae al cargar)`,
					`<replace image="${img}"> and there is no <image idx="${img}"> (the game crashes on load)`,
					`<replace image="${img}"> e non c'è nessuna <image idx="${img}"> (il gioco si chiude al caricamento)`));
				else if (frames && im.rows) {
					if (frames - 2 > im.rows) warn(f, line, T(
						`la animación tiene ${frames - 2} fotogramas útiles (frames - 2) y la imagen ${img} solo ${im.rows} filas: leería filas que no existen`,
						`the animation has ${frames - 2} usable frames (frames - 2) and image ${img} only ${im.rows} rows: it would read rows that do not exist`,
						`l'animazione ha ${frames - 2} fotogrammi utili (frames - 2) e l'immagine ${img} solo ${im.rows} righe: leggerebbe righe che non esistono`));
					else if (frames - 2 < im.rows) warn(f, line, T(
						`la animación usa ${frames - 2} fotogramas y la imagen ${img} tiene ${im.rows} filas (lo normal es frames = filas + 2)`,
						`the animation uses ${frames - 2} frames and image ${img} has ${im.rows} rows (normally frames = rows + 2)`,
						`l'animazione usa ${frames - 2} fotogrammi e l'immagine ${img} ha ${im.rows} righe (di norma frames = righe + 2)`));
				}
			}
		}
	}
}
// The class that points to an entity that does not exist (in the mod or, with --game, in the game)
for (const [id, c] of classHeaders) {
	const e = (c.header.match(/\bentity\s*=\s*"([^"]*)"/) || [])[1];
	if (!e) continue;
	const rel = e.replace(/\\/g, '/');
	if (has(rel)) continue;
	if (gameHas(rel) === false) error(c.file, 0, T(
		`la clase ${id}: entity="${e}" no existe en el mod ni en el juego`,
		`the class ${id}: entity="${e}" exists neither in the mod nor in the game`,
		`la classe ${id}: entity="${e}" non esiste né nella mod né nel gioco`));
}

// --- summary ----------------------------------------------------------------------------------------------------------
console.log('');
if (!errors && !warnings) console.log(T(
	`OK: ${files.length} archivos revisados, sin problemas.`,
	`OK: ${files.length} files checked, no problems.`,
	`OK: ${files.length} file controllati, nessun problema.`));
else console.log(T(
	`${files.length} archivos revisados: ${errors} errores, ${warnings} avisos.`,
	`${files.length} files checked: ${errors} errors, ${warnings} warnings.`,
	`${files.length} file controllati: ${errors} errori, ${warnings} avvisi.`));
if (!dataDir) console.log(T(
	'(Sin --datos no se comprueba que los archivos y scripts a los que se refiere el mod existan en el juego.)',
	'(Without --datos, the tool does not check that the files and scripts the mod refers to exist in the game.)',
	'(Senza --datos non si controlla che i file e gli script a cui la mod si riferisce esistano nel gioco.)'));
process.exit(errors ? 1 : 0);
