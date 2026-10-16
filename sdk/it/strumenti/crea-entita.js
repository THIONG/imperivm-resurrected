// Genera l'.ent.xml (l'«entity»: quali immagini ha un'unità o un edificio e come si animano) a partire dai .rle che
// ha prodotto converti-sprite.js (SDK di Imperivm Resurrected). Legge le intestazioni dei .rle per impostare correttamente righe e colonne.
//
//   UNITÀ
//   node crea-entita.js unita <cartella con i .rle> --nome MiSoldado [--piedi 75,100] [--config animaciones.json] [--consenti-incompleta]
//     Cerca nella cartella Idle.rle, Walk.rle, Fight.rle, Death.rle (obbligatori), Transition.rle e Taunt.rle (opzionali) e i loro
//     <Nome>_shadow.rle. Scrive <cartella>\<Nome>.ent.xml con 2 livelli (unità z=1000 e ombra z=800), 2 stati (riposo, combattimento) e
//     le 7 animazioni del gioco (camminata 1, attacco 5, morte 9, Idle 13, ToIdle 18, ToAttack 19, Taunt 20).
//     Senza Transition.rle o senza Taunt.rle si usa il foglio Idle per quelle animazioni (il gioco le trova sempre).
//     Alias ammessi: Attack = Fight, Die = Death.
//   --config: JSON per cambiare il ritmo di ogni animazione: {"Walk": {"duracion": 55, "paso": 42}, "Fight": {"duracion": 71, "golpe": 426}}
//     duracion = millisecondi per fotogramma; paso (step) = lo legge il movimento e influisce sulla giocabilità; golpe (action_time) =
//     millisecondi prima che parta il colpo in Fight (di default, il 43 % dell'animazione, come l'hastatus).
//   --consenti-incompleta: non richiede i quattro fogli obbligatori; genera solo ciò che c'è.
//
//   EDIFICIO
//   node crea-entita.js edificio <cartella con i .rle> --nome MiTorre --piedi 305,330 [--impronta 6x6 | --impronta-png mascara.png]
//                         [--punti-da otro.ent.xml] [--livelli layer1,layer2,...]
//     Cerca shadow.rle (opzionale) e layer1.rle, layer2.rle... (ciascuno, un foglio di 1 colonna con una riga per fase di costruzione;
//     il gioco ne usa 4). --piedi = punto della tela in cui si «inchioda» l'edificio (l'origine sulla mappa). Scrive <Nome>.ent.xml e,
//     con --impronta o --impronta-png, <Nome>.pass.
//   --impronta AnchoxAlto   rettangolo di celle (ognuna = 16 unità di mappa, 1 casella ~ mezzo soldato) centrato sull'origine.
//   --impronta-png f.png    maschera disegnata: 1 pixel = 1 cella, nord in alto; un pixel opaco e scuro = bloccato; trasparente o
//                         bianco = libero. Si centra sull'origine dell'edificio. Massimo circa 120x120 celle.
//   --punti-da f.ent.xml copia il blocco <points> di un altro edificio (ingressi, postazioni, fumo…): senza punti l'edificio non ha
//                         dove far entrare la gente.
//
// I limiti del gioco vengono controllati qui: 32 immagini, 32 livelli, 8 stati, animazioni 1..28, 64 fotogrammi per animazione.
const fs = require('fs'), path = require('path');
const rle = require('./lib/rle');
const pass = require('./lib/pass');
const png = require('./lib/png');
const { T } = require('./lib/lang');

const args = process.argv.slice(2);
const valued = ['--nombre', '--pies', '--config', '--huella', '--huella-png', '--puntos-de', '--capas', '--tipo'];
const opt = (n, d) => { const i = args.indexOf(n); return i >= 0 ? args[i + 1] : d; };
const flag = n => args.includes(n);
const pos = args.filter((a, i) => !a.startsWith('--') && !valued.includes(args[i - 1]));
const kind = ({ unit: 'unidad', unita: 'unidad', building: 'edificio' })[String(pos[0]).toLowerCase()] || pos[0], dir = pos[1];
function fail(msg) { console.error(T('ERROR: ', 'ERROR: ', 'ERRORE: ') + msg); process.exit(1); }
if (!['unidad', 'edificio'].includes(kind) || !dir || !opt('--nombre')) {
	console.error(T('uso: node crea-entita.js unidad|edificio <carpeta con los .rle> --nombre Nombre [--pies X,Y] ...   (ver el principio de este archivo)',
		'usage: node crea-entita.js unidad|edificio <folder with the .rle files> --nombre Name [--pies X,Y] ...   (see the top of this file)',
		'uso: node crea-entita.js unidad|edificio <cartella con i file .rle> --nombre Nome [--pies X,Y] ...   (vedi l\'inizio di questo file)'));
	process.exit(2);
}
if (!fs.existsSync(dir) || !fs.statSync(dir).isDirectory()) fail(T('no existe la carpeta ' + dir, 'folder does not exist: ' + dir, 'la cartella non esiste: ' + dir));
const name = opt('--nombre');
if (!/^[A-Za-z][A-Za-z0-9_]{0,62}$/.test(name)) {
	fail(T('--nombre solo puede llevar letras, números y _ (máximo 63 caracteres) y empezar por letra',
		'--nombre can only contain letters, digits and _ (63 characters at most) and must start with a letter',
		'--nombre può contenere solo lettere, numeri e _ (massimo 63 caratteri) e deve iniziare con una lettera'));
}
const defaultFeet = kind === 'unidad' ? '75,100' : null;
const feetArg = opt('--pies', defaultFeet);
if (!feetArg) {
	fail(T('un edificio necesita --pies X,Y (el punto del lienzo donde se clava el edificio; en los del juego, p. ej. 305,330)',
		'a building needs --pies X,Y (the point of the canvas where the building is anchored; in the game\'s buildings, e.g. 305,330)',
		'un edificio richiede --pies X,Y (il punto della tela in cui è ancorato l\'edificio; in quelli del gioco, p. es. 305,330)'));
}
const feet = feetArg.split(',').map(Number);
if (feet.length !== 2 || feet.some(isNaN)) fail(T('--pies debe ser X,Y', '--pies must be X,Y', '--pies deve essere X,Y'));
const offX = -feet[0], offY = -feet[1];

// ---------- leggere i .rle della cartella --------
const found = new Map(); // nome in minuscolo -> {file, base, sheet}
for (const f of fs.readdirSync(dir)) {
	if (!/\.rle$/i.test(f)) continue;
	let sheet;
	try { sheet = rle.parse(fs.readFileSync(path.join(dir, f))); } catch (e) { fail(`${f}: ${e.message}`); }
	if (sheet.cells.some(c => !c.empty && !c.data)) {
		fail(T(`${f}: es un .rle.mmp del juego (sin los píxeles dentro); el juego no lo cargaría desde un mod`,
			`${f}: is a .rle.mmp from the game (without the pixels inside); the game would not load it from a mod`,
			`${f}: è un .rle.mmp del gioco (senza i pixel all'interno); il gioco non lo caricherebbe da una mod`));
	}
	found.set(f.replace(/\.rle$/i, '').toLowerCase(), { file: f.replace(/\.rle$/i, ''), sheet });
}
const esc = s => String(s).replace(/&/g, '&amp;').replace(/"/g, '&quot;').replace(/</g, '&lt;');
const drawmodeOf = sheet => sheet.mode === rle.MODE_PLAYER ? 'player_color' : sheet.mode === rle.MODE_SHADOW ? 'shadow' : sheet.mode === rle.MODE_PAL ? (kind === 'unidad' ? 'index' : 'normal') : 'normal';

const images = []; // {idx, file, drawmode, remaping, rows, columns}
const addImage = (file, sheet, remaping = 'none') => {
	const ex = images.find(i => i.file === file && i.remaping === remaping);
	if (ex) return ex.idx;
	images.push({ idx: images.length + 1, file, drawmode: drawmodeOf(sheet), remaping, rows: sheet.rows, columns: sheet.cols });
	return images.length;
};
const warnings = [];
const finish = (entityAttrs, body) => {
	if (images.length > 32) fail(T(`la entidad tendría ${images.length} imágenes y el juego admite 32`, `the entity would have ${images.length} images and the game allows 32`, `l'entità avrebbe ${images.length} immagini e il gioco ne ammette 32`));
	const xml = ['<?xml version="1.0"?>', '', `<entity ${entityAttrs}>`, '<images>'];
	for (const i of images) xml.push(`<image idx="${i.idx}" file="${esc(i.file)}" drawmode="${i.drawmode}" remaping="${i.remaping}" rows="${i.rows}" columns="${i.columns}"/>`);
	xml.push('</images>', ...body, '</entity>', '');
	const target = path.join(dir, name + '.ent.xml');
	fs.writeFileSync(target, xml.join('\n'));
	console.log(T(`Escrito ${target}  (${images.length} imágenes)`, `Written ${target}  (${images.length} images)`, `Scritto ${target}  (${images.length} immagini)`));
	for (const w of warnings) console.log(T('AVISO: ', 'WARNING: ', 'AVVISO: ') + w);
};

if (kind === 'unidad') buildUnit(); else buildBuilding();

// =============================================================================================================================
function buildUnit() {
	const cfg = opt('--config') ? JSON.parse(fs.readFileSync(opt('--config'), 'utf8')) : {};
	const alias = { fight: ['fight', 'attack'], death: ['death', 'die'] };
	const pick = n => { for (const a of alias[n] || [n]) if (found.has(a)) return found.get(a); return null; };
	const sheets = {};
	for (const n of ['idle', 'walk', 'fight', 'death', 'transition', 'taunt']) sheets[n] = pick(n);
	const missing = ['idle', 'walk', 'fight', 'death'].filter(n => !sheets[n]);
	if (missing.length && !flag('--permitir-incompleta')) {
		const ml = missing.map(n => n[0].toUpperCase() + n.slice(1) + '.rle').join(', ');
		fail(T(`faltan hojas obligatorias en ${dir}: ${ml} (o usa --permitir-incompleta)`,
			`required sheets are missing in ${dir}: ${ml} (or use --permitir-incompleta)`,
			`mancano fogli obbligatori in ${dir}: ${ml} (oppure usa --permitir-incompleta)`));
	}
	const known = new Set(Object.values(sheets).filter(Boolean).map(s => s.file.toLowerCase()));
	for (const [k, v] of found) {
		if (!k.endsWith('_shadow') && !known.has(k)) {
			warnings.push(T(`${v.file}.rle no es una animación conocida (Idle, Walk, Fight, Death, Transition, Taunt) y se ignora`,
				`${v.file}.rle is not a known animation (Idle, Walk, Fight, Death, Transition, Taunt) and is ignored`,
				`${v.file}.rle non è un'animazione nota (Idle, Walk, Fight, Death, Transition, Taunt) e viene ignorato`));
		}
	}
	const cols = [...new Set(Object.values(sheets).filter(Boolean).map(s => s.sheet.cols))];
	if (cols.length !== 1) {
		const cl = Object.entries(sheets).filter(e => e[1]).map(e => `${e[1].file}: ${e[1].sheet.cols}`).join(', ');
		fail(T('las hojas tienen distinto número de direcciones: ' + cl, 'the sheets have different numbers of directions: ' + cl, 'i fogli hanno un numero diverso di direzioni: ' + cl));
	}
	const variations = cols[0];
	// ombre: o tutte o nessuna
	const shadows = {};
	for (const n of Object.keys(sheets)) if (sheets[n]) shadows[n] = found.get(sheets[n].file.toLowerCase() + '_shadow') || null;
	const withShadow = Object.values(shadows).filter(Boolean).length;
	const hasShadow = withShadow > 0;
	if (hasShadow && withShadow !== Object.keys(shadows).filter(n => sheets[n]).length) {
		fail(T('hay sombra (_shadow.rle) para unas animaciones y para otras no: ponlas para todas o para ninguna',
			'there is a shadow (_shadow.rle) for some animations and not for others: provide it for all or for none',
			'c\'è l\'ombra (_shadow.rle) per alcune animazioni e per altre no: mettila per tutte o per nessuna'));
	}
	for (const n of Object.keys(shadows)) {
		const s = shadows[n];
		if (s && (s.sheet.cols !== sheets[n].sheet.cols || s.sheet.rows !== sheets[n].sheet.rows)) {
			fail(T(`${s.file}.rle es de ${s.sheet.cols}x${s.sheet.rows} y ${sheets[n].file}.rle de ${sheets[n].sheet.cols}x${sheets[n].sheet.rows}: deben tener la misma rejilla`,
				`${s.file}.rle is ${s.sheet.cols}x${s.sheet.rows} and ${sheets[n].file}.rle is ${sheets[n].sheet.cols}x${sheets[n].sheet.rows}: they must have the same grid`,
				`${s.file}.rle è di ${s.sheet.cols}x${s.sheet.rows} e ${sheets[n].file}.rle di ${sheets[n].sheet.cols}x${sheets[n].sheet.rows}: devono avere la stessa griglia`));
		}
	}
	for (const [n, s] of Object.entries(sheets)) if (s) {
		if (s.sheet.rows > 62) fail(T(`${s.file}.rle tiene ${s.sheet.rows} fotogramas; máximo 62`, `${s.file}.rle has ${s.sheet.rows} frames; 62 at most`, `${s.file}.rle ha ${s.sheet.rows} fotogrammi; massimo 62`));
		if (s.sheet.mode !== rle.MODE_PAL && s.sheet.mode !== rle.MODE_PLAYER) {
			warnings.push(T(`${s.file}.rle tiene el modo ${s.sheet.mode}; las unidades usan 1 (paleta) o 2 (paleta + color de jugador)`,
				`${s.file}.rle has mode ${s.sheet.mode}; units use 1 (palette) or 2 (palette + player color)`,
				`${s.file}.rle ha il modo ${s.sheet.mode}; le unità usano 1 (tavolozza) o 2 (tavolozza + colore del giocatore)`));
		}
	}
	const modes = [...new Set(Object.values(sheets).filter(Boolean).map(s => s.sheet.mode))];
	if (modes.length > 1) warnings.push(T('las hojas mezclan modos de color (con y sin color de jugador)', 'the sheets mix color modes (with and without player color)', 'i fogli mescolano modi di colore (con e senza colore del giocatore)'));

	// immagini: prima quelle a colori, poi le ombre
	const imgC = {}, imgS = {};
	for (const n of ['idle', 'walk', 'fight', 'death', 'transition', 'taunt']) if (sheets[n]) imgC[n] = addImage(sheets[n].file + '.rle', sheets[n].sheet);
	if (sheets.transition) imgC.transitionRev = addImage(sheets.transition.file + '.rle', sheets.transition.sheet, 'reverse');
	if (hasShadow) {
		for (const n of ['idle', 'walk', 'fight', 'death', 'transition', 'taunt']) if (sheets[n]) imgS[n] = addImage(shadows[n].file + '.rle', shadows[n].sheet);
		if (sheets.transition) imgS.transitionRev = addImage(shadows.transition.file + '.rle', shadows.transition.sheet, 'reverse');
	}
	const body = [];
	body.push('<points>', '</points>', '<layers>');
	body.push(`<layer idx="1" name="unit" image="${imgC.idle || imgC.walk}" z="1000" offsetx="${offX}" offsety="${offY}" sortoffsetx="0" sortoffsety="0"/>`);
	let nLayers = 1;
	if (hasShadow) { body.push(`<layer idx="2" name="shadow" image="${imgS.idle || imgS.walk}" z="800" offsetx="${offX}" offsety="${offY}" sortoffsetx="0" sortoffsety="0"/>`); nLayers = 2; }
	body.push('</layers>', '<states>',
		'<state idx="1" name="idle" image_idx="0" image_row="0" offsetx="0" offsety="0"/>',
		'<state idx="2" name="fight" image_idx="0" image_row="0" offsetx="0" offsety="0"/>', '</states>', '<anims>');

	const defaults = { walk: 55, fight: 71, death: 80, idle: 150, transition: 72, taunt: 132 };
	const cfgOf = n => cfg[n[0].toUpperCase() + n.slice(1)] || cfg[n] || {};
	// def: idx, name, startstate, endstate, foglio (chiave di sheets), immagine inversa, paso, golpe
	const defs = [
		{ idx: 1, name: 'walk', s: 1, e: 1, key: 'walk', step: 42, base: 'walk' },
		{ idx: 5, name: 'attack', s: 2, e: 2, key: 'fight', step: 0, base: 'fight', hit: true },
		{ idx: 9, name: 'die', s: 1, e: 1, key: 'death', step: 0, base: 'death', die: true },
		{ idx: 13, name: 'Idle', s: 1, e: 1, key: 'idle', step: 0, base: 'idle' },
		{ idx: 18, name: 'ToIdle', s: 2, e: 1, key: 'transition', fallback: 'idle', rev: true, step: 0, base: 'transition' },
		{ idx: 19, name: 'ToAttack', s: 1, e: 2, key: 'transition', fallback: 'idle', step: 0, base: 'transition' },
		{ idx: 20, name: 'Taunt', s: 1, e: 1, key: 'taunt', fallback: 'idle', step: 0, base: 'taunt', taunt: true },
	];
	let count = 0;
	for (const d of defs) {
		let key = d.key, fell = false;
		if (!sheets[key]) { if (!d.fallback || !sheets[d.fallback]) continue; key = d.fallback; fell = true; }
		const sh = sheets[key].sheet, rows = sh.rows, c = cfgOf(d.base);
		if (rows + 2 > 64) fail(T(`${sheets[key].file}.rle: ${rows} fotogramas + 2 centinelas superan 64`, `${sheets[key].file}.rle: ${rows} frames + 2 sentinels exceed 64`, `${sheets[key].file}.rle: ${rows} fotogrammi + 2 sentinelle superano 64`));
		const dur = c.duracion || (fell ? 40 : defaults[d.base]);
		const durs = [];
		for (let r = 0; r < rows; r++) durs.push(dur);
		if (d.die && !c.duracion) { // come l'hastatus: il cadavere resta e le ultime immagini durano sempre di più
			const tail = [180, 500, 1000, 1000, 2000, 3000];
			if (rows >= 7) for (let k = 0; k < 6; k++) durs[rows - 6 + k] = tail[k]; else durs[rows - 1] = 3000;
		}
		const total = durs.reduce((a, b) => a + b, 0);
		const step = c.paso !== undefined ? c.paso : d.step;
		const hit = d.hit ? (c.golpe !== undefined ? c.golpe : Math.round(total * 0.43)) : d.taunt && !fell ? Math.round(total * 0.5) : 0;
		const useRev = d.rev && !fell && imgC.transitionRev;
		const ic = useRev ? imgC.transitionRev : imgC[key], is = useRev ? imgS.transitionRev : imgS[key];
		body.push(`<anim idx="${d.idx}" name="${d.name}" startstate="${d.s}" endstate="${d.e}" frames="${rows + 2}" duration="${total}" default_duration="${dur}" action_time="${hit}" step="${step}">`);
		body.push(`<replace layer="1" image="${ic}" offsetx="${offX}" offsety="${offY}"/>`);
		if (hasShadow) body.push(`<replace layer="2" image="${is}" offsetx="${offX}" offsety="${offY}"/>`);
		body.push('<frame idx="1" duration="0"/>');
		for (let r = 0; r < rows; r++) body.push(`<frame idx="${r + 2}" duration="${durs[r]}"/>`);
		body.push(`<frame idx="${rows + 2}" duration="0"/>`, '</anim>');
		count++;
		if (fell) {
			const baseFile = d.base[0].toUpperCase() + d.base.slice(1);
			warnings.push(T(`${d.name}: no hay ${baseFile}.rle; se usa la hoja ${sheets[key].file}`,
				`${d.name}: there is no ${baseFile}.rle; the ${sheets[key].file} sheet is used`,
				`${d.name}: non c'è ${baseFile}.rle; si usa il foglio ${sheets[key].file}`));
		}
	}
	body.push('</anims>');
	if (nLayers > 32 || count > 28) fail(T('límites del juego superados', 'game limits exceeded', 'limiti del gioco superati'));
	if (variations !== 8) {
		warnings.push(T(`${variations} direcciones: lo habitual son 8 (soldados), 12 (héroes y animales) o 4 (barcos)`,
			`${variations} directions: the usual are 8 (soldiers), 12 (heroes and animals) or 4 (ships)`,
			`${variations} direzioni: di solito sono 8 (soldati), 12 (eroi e animali) o 4 (navi)`));
	}
	finish(`name="${name}" type="${esc(opt('--tipo', 'vx/unit'))}" variations="${variations}" pass_file=""`, body);
	console.log(T(`${count} animaciones, ${variations} direcciones, ${hasShadow ? 'con' : 'sin'} sombra. En la clase (.sc.xml): entity="units/${name}/${name}.ent.xml" si lo dejas en units\\${name}\\ de tu mod.`,
		`${count} animations, ${variations} directions, ${hasShadow ? 'with' : 'without'} shadow. In the class (.sc.xml): entity="units/${name}/${name}.ent.xml" if you leave it in units\\${name}\\ of your mod.`,
		`${count} animazioni, ${variations} direzioni, ${hasShadow ? 'con' : 'senza'} ombra. Nella classe (.sc.xml): entity="units/${name}/${name}.ent.xml" se la lasci in units\\${name}\\ della tua mod.`));
}

// =============================================================================================================================
function buildBuilding() {
	let layerNames;
	if (opt('--capas')) layerNames = opt('--capas').split(',').map(s => s.trim()).filter(Boolean);
	else layerNames = [...found.keys()].filter(k => /^layer\d+$/.test(k)).sort((a, b) => parseInt(a.slice(5)) - parseInt(b.slice(5)));
	if (!layerNames.length) fail(T(`no hay capas (layer1.rle, layer2.rle…) en ${dir}`, `no layers (layer1.rle, layer2.rle…) in ${dir}`, `nessuno strato (layer1.rle, layer2.rle…) in ${dir}`));
	const list = layerNames.map(n => { const f = found.get(n.toLowerCase()); if (!f) fail(T(`no existe ${n}.rle en ${dir}`, `${n}.rle does not exist in ${dir}`, `${n}.rle non esiste in ${dir}`)); return f; });
	const shadow = found.get('shadow');
	const all = shadow ? [...list, shadow] : list;
	const rows = [...new Set(all.map(s => s.sheet.rows))], cols = [...new Set(all.map(s => s.sheet.cols))];
	if (cols.length !== 1 || cols[0] !== 1) {
		const cl = all.map(s => `${s.file} ${s.sheet.cols}`).join(', ');
		fail(T('los .rle de un edificio deben tener 1 columna (una sola dirección); tienen: ' + cl,
			'the .rle files of a building must have 1 column (a single direction); they have: ' + cl,
			'i file .rle di un edificio devono avere 1 colonna (una sola direzione); hanno: ' + cl));
	}
	if (rows.length !== 1) {
		const rl = all.map(s => `${s.file} ${s.sheet.rows}`).join(', ');
		fail(T('todas las capas de un edificio deben tener las mismas filas (fases de construcción): ' + rl,
			'all layers of a building must have the same rows (construction phases): ' + rl,
			'tutti gli strati di un edificio devono avere le stesse righe (fasi di costruzione): ' + rl));
	}
	const nrows = rows[0];
	if (nrows > 8) fail(T(`${nrows} fases de construcción; el juego admite 8 estados como máximo`, `${nrows} construction phases; the game allows 8 states at most`, `${nrows} fasi di costruzione; il gioco ammette al massimo 8 stati`));
	if (nrows !== 4) warnings.push(T(`${nrows} fases de construcción: los edificios del juego usan 4`, `${nrows} construction phases: the game's buildings use 4`, `${nrows} fasi di costruzione: gli edifici del gioco ne usano 4`));
	const lc = {}, ls = {};
	if (shadow) ls.shadow = addImage(shadow.file + '.rle', shadow.sheet);
	list.forEach(s => { lc[s.file] = addImage(s.file + '.rle', s.sheet); });
	const body = [];
	// punti
	if (opt('--puntos-de')) {
		const src = fs.readFileSync(opt('--puntos-de'), 'utf8'), m = /<points>[\s\S]*?<\/points>/.exec(src);
		if (!m) fail(T('--puntos-de: ese archivo no tiene un bloque <points>', '--puntos-de: that file has no <points> block', '--puntos-de: quel file non ha un blocco <points>'));
		body.push(...m[0].split(/\r?\n/));
	} else {
		body.push('<points>', '</points>');
		warnings.push(T('el edificio no tiene <point>: usa --puntos-de <un .ent.xml de un edificio parecido> para copiar los de entrada/puestos',
			'the building has no <point>: use --puntos-de <the .ent.xml of a similar building> to copy the entrance/post points',
			'l\'edificio non ha <point>: usa --puntos-de <il .ent.xml di un edificio simile> per copiare quelli di ingresso/postazioni'));
	}
	body.push('<layers>');
	let li = 1;
	if (shadow) body.push(`<layer idx="${li++}" name="shadow" image="${ls.shadow}" z="800" offsetx="${offX}" offsety="${offY}" sortoffsetx="0" sortoffsety="0" xray="0"/>`);
	list.forEach(s => body.push(`<layer idx="${li++}" name="${esc(s.file)}" image="${lc[s.file]}" z="1000" offsetx="${offX}" offsety="${offY}" sortoffsetx="0" sortoffsety="0" xray="0"/>`));
	if (li - 1 > 32) fail(T('más de 32 capas', 'more than 32 layers', 'più di 32 strati'));
	body.push('</layers>', '<states>');
	for (let i = 0; i < nrows; i++) body.push(`<state idx="${i + 1}" name="${i}" image_idx="0" image_row="0" offsetx="0" offsety="0" anim_idx="65536" anim_frame="65536"/>`);
	body.push('</states>', '<anims>', '</anims>');

	// impronta
	let passFile = '';
	if (opt('--huella') || opt('--huella-png')) {
		let cells, desc;
		if (opt('--huella')) {
			const m = /^(\d+)x(\d+)$/i.exec(opt('--huella'));
			if (!m) fail(T('--huella debe ser AnchoxAlto en celdas, p. ej. 6x6', '--huella must be WidthxHeight in cells, e.g. 6x6', '--huella deve essere LarghezzaxAltezza in celle, p. es. 6x6'));
			try { cells = pass.rectangle(+m[1], +m[2]); } catch (e) { fail(e.message); }
			desc = T(`rectángulo de ${m[1]}x${m[2]} celdas`, `rectangle of ${m[1]}x${m[2]} cells`, `rettangolo di ${m[1]}x${m[2]} celle`);
		} else {
			const img = png.decode(fs.readFileSync(opt('--huella-png')));
			const mask = new Uint8Array(img.width * img.height);
			for (let i = 0; i < mask.length; i++) mask[i] = img.data[i * 4 + 3] >= 128 && (img.data[i * 4] + img.data[i * 4 + 1] + img.data[i * 4 + 2]) / 3 < 200 ? 1 : 0;
			try { cells = pass.fromMask(mask, img.width, img.height); } catch (e) { fail(e.message); }
			desc = T(`máscara ${img.width}x${img.height}`, `mask ${img.width}x${img.height}`, `maschera ${img.width}x${img.height}`);
		}
		const count = cells.reduce((a, b) => a + b, 0);
		if (!count) fail(T('la huella está vacía (ninguna celda bloqueada)', 'the footprint is empty (no blocked cell)', 'l\'impronta è vuota (nessuna cella bloccata)'));
		passFile = name + '.pass';
		fs.writeFileSync(path.join(dir, passFile), pass.encode(cells));
		console.log(T(`Escrito ${path.join(dir, passFile)}  (${desc}, ${count} celdas bloqueadas; cada celda = 16 unidades de mapa)`,
			`Written ${path.join(dir, passFile)}  (${desc}, ${count} blocked cells; each cell = 16 map units)`,
			`Scritto ${path.join(dir, passFile)}  (${desc}, ${count} celle bloccate; ogni cella = 16 unità di mappa)`));
	} else warnings.push(T('sin --huella ni --huella-png el edificio no ocupa sitio en el mapa (la gente lo atraviesa)',
		'without --huella or --huella-png the building takes up no space on the map (people walk through it)',
		'senza --huella né --huella-png l\'edificio non occupa spazio sulla mappa (la gente lo attraversa)'));
	finish(`name="${name}" type="vx/building" variations="1" pass_file="${esc(passFile)}"`, body);
	console.log(T(`${list.length} capas${shadow ? ' + sombra' : ''}, ${nrows} fases de construcción. En la clase (.sc.xml): entity="buildings/${name}/${name}.ent.xml" si lo dejas en buildings\\${name}\\ de tu mod.`,
		`${list.length} layers${shadow ? ' + shadow' : ''}, ${nrows} construction phases. In the class (.sc.xml): entity="buildings/${name}/${name}.ent.xml" if you leave it in buildings\\${name}\\ of your mod.`,
		`${list.length} strati${shadow ? ' + ombra' : ''}, ${nrows} fasi di costruzione. Nella classe (.sc.xml): entity="buildings/${name}/${name}.ent.xml" se lo lasci in buildings\\${name}\\ della tua mod.`));
}
