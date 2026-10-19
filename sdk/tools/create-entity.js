// Generates the .ent.xml (the "entity": which images a unit or building has and how they animate) from the .rle files that
// convert-sprite.js produced (Imperivm Resurrected SDK). Reads the .rle headers to set rows and columns correctly.
//
//   UNIT
//   node create-entity.js unit <folder with the .rle files> --name MiSoldado [--feet 75,100] [--config animaciones.json] [--allow-incomplete]
//     Looks in the folder for Idle.rle, Walk.rle, Fight.rle, Death.rle (required), Transition.rle and Taunt.rle (optional) and their
//     <Name>_shadow.rle. Writes <folder>\<Name>.ent.xml with 2 layers (unit z=1000 and shadow z=800), 2 states (idle, combat) and
//     the game's 7 animations (walk 1, attack 5, die 9, Idle 13, ToIdle 18, ToAttack 19, Taunt 20).
//     Without Transition.rle or Taunt.rle, the Idle sheet is used for those animations (the game always finds them).
//     Accepted aliases: Attack = Fight, Die = Death.
//   --config: JSON to change the pace of each animation: {"Walk": {"duration": 55, "step": 42}, "Fight": {"duration": 71, "hit": 426}}
//     (the Spanish keys duracion, paso and golpe are still accepted)
//     duration = milliseconds per frame; step = read by movement and affects gameplay; hit (action_time) =
//     milliseconds until the blow lands in Fight (by default, 43 % of the animation, like the hastatus).
//   --allow-incomplete: does not require the four mandatory sheets; only generates what is there.
//
//   BUILDING
//   node create-entity.js building <folder with the .rle files> --name MiTorre --feet 305,330 [--footprint 6x6 | --footprint-png mascara.png]
//                         [--points-from other.ent.xml] [--layers layer1,layer2,...]
//     Looks for shadow.rle (optional) and layer1.rle, layer2.rle... (each one a 1-column sheet with one row per construction stage;
//     the game uses 4 stages). --feet = canvas point where the building is "pinned" (the map origin). Writes <Name>.ent.xml and,
//     with --footprint or --footprint-png, <Name>.pass.
//   --footprint WidthxHeight   rectangle of cells (each = 16 map units, about half a soldier's width) centered on the origin.
//   --footprint-png f.png    drawn mask: 1 pixel = 1 cell, north up; an opaque dark pixel = blocked; transparent or
//                         white = free. Centered on the building's origin. At most about 120x120 cells.
//   --points-from f.ent.xml copies the <points> block of another building (entrances, posts, smoke...): without points the building has
//                         nowhere for people to enter.
//
// The game's limits are checked here: 32 images, 32 layers, 8 states, animations 1..28, 64 frames per animation.
const fs = require('fs'), path = require('path');
const rle = require('./lib/rle');
const pass = require('./lib/pass');
const png = require('./lib/png');

const args = process.argv.slice(2);
const valued = ['--name', '--feet', '--config', '--footprint', '--footprint-png', '--points-from', '--layers', '--type'];
const opt = (n, d) => { const i = args.indexOf(n); return i >= 0 ? args[i + 1] : d; };
const flag = n => args.includes(n);
const pos = args.filter((a, i) => !a.startsWith('--') && !valued.includes(args[i - 1]));
const kind = ({ unit: 'unit', building: 'building' })[String(pos[0]).toLowerCase()] || pos[0], dir = pos[1];
function fail(msg) { console.error('ERROR: ' + msg); process.exit(1); }
if (!['unit', 'building'].includes(kind) || !dir || !opt('--name')) {
	console.error('usage: node create-entity.js unit|building <folder with the .rle files> --name Name [--feet X,Y] ...   (see the top of this file)');
	process.exit(2);
}
if (!fs.existsSync(dir) || !fs.statSync(dir).isDirectory()) fail('folder does not exist: ' + dir);
const name = opt('--name');
if (!/^[A-Za-z][A-Za-z0-9_]{0,62}$/.test(name)) {
	fail('--name can only contain letters, digits and _ (63 characters at most) and must start with a letter');
}
const defaultFeet = kind === 'unit' ? '75,100' : null;
const feetArg = opt('--feet', defaultFeet);
if (!feetArg) {
	fail('a building needs --feet X,Y (the point of the canvas where the building is anchored; in the game\'s buildings, e.g. 305,330)');
}
const feet = feetArg.split(',').map(Number);
if (feet.length !== 2 || feet.some(isNaN)) fail('--feet must be X,Y');
const offX = -feet[0], offY = -feet[1];

// ---------- reading the .rle files in the folder ----------
const found = new Map(); // lowercase name -> {file, base, sheet}
for (const f of fs.readdirSync(dir)) {
	if (!/\.rle$/i.test(f)) continue;
	let sheet;
	try { sheet = rle.parse(fs.readFileSync(path.join(dir, f))); } catch (e) { fail(`${f}: ${e.message}`); }
	if (sheet.cells.some(c => !c.empty && !c.data)) {
		fail(`${f}: is a .rle.mmp from the game (without the pixels inside); the game would not load it from a mod`);
	}
	found.set(f.replace(/\.rle$/i, '').toLowerCase(), { file: f.replace(/\.rle$/i, ''), sheet });
}
const esc = s => String(s).replace(/&/g, '&amp;').replace(/"/g, '&quot;').replace(/</g, '&lt;');
const drawmodeOf = sheet => sheet.mode === rle.MODE_PLAYER ? 'player_color' : sheet.mode === rle.MODE_SHADOW ? 'shadow' : sheet.mode === rle.MODE_PAL ? (kind === 'unit' ? 'index' : 'normal') : 'normal';

const images = []; // {idx, file, drawmode, remaping, rows, columns}
const addImage = (file, sheet, remaping = 'none') => {
	const ex = images.find(i => i.file === file && i.remaping === remaping);
	if (ex) return ex.idx;
	images.push({ idx: images.length + 1, file, drawmode: drawmodeOf(sheet), remaping, rows: sheet.rows, columns: sheet.cols });
	return images.length;
};
const warnings = [];
const finish = (entityAttrs, body) => {
	if (images.length > 32) fail(`the entity would have ${images.length} images and the game allows 32`);
	const xml = ['<?xml version="1.0"?>', '', `<entity ${entityAttrs}>`, '<images>'];
	for (const i of images) xml.push(`<image idx="${i.idx}" file="${esc(i.file)}" drawmode="${i.drawmode}" remaping="${i.remaping}" rows="${i.rows}" columns="${i.columns}"/>`);
	xml.push('</images>', ...body, '</entity>', '');
	const target = path.join(dir, name + '.ent.xml');
	fs.writeFileSync(target, xml.join('\n'));
	console.log(`Written ${target}  (${images.length} images)`);
	for (const w of warnings) console.log('WARNING: ' + w);
};

if (kind === 'unit') buildUnit(); else buildBuilding();

// =============================================================================================================================
function buildUnit() {
	const cfg = opt('--config') ? JSON.parse(fs.readFileSync(opt('--config'), 'utf8')) : {};
	const alias = { fight: ['fight', 'attack'], death: ['death', 'die'] };
	const pick = n => { for (const a of alias[n] || [n]) if (found.has(a)) return found.get(a); return null; };
	const sheets = {};
	for (const n of ['idle', 'walk', 'fight', 'death', 'transition', 'taunt']) sheets[n] = pick(n);
	const missing = ['idle', 'walk', 'fight', 'death'].filter(n => !sheets[n]);
	if (missing.length && !flag('--allow-incomplete')) {
		const ml = missing.map(n => n[0].toUpperCase() + n.slice(1) + '.rle').join(', ');
		fail(`required sheets are missing in ${dir}: ${ml} (or use --allow-incomplete)`);
	}
	const known = new Set(Object.values(sheets).filter(Boolean).map(s => s.file.toLowerCase()));
	for (const [k, v] of found) {
		if (!k.endsWith('_shadow') && !known.has(k)) {
			warnings.push(`${v.file}.rle is not a known animation (Idle, Walk, Fight, Death, Transition, Taunt) and is ignored`);
		}
	}
	const cols = [...new Set(Object.values(sheets).filter(Boolean).map(s => s.sheet.cols))];
	if (cols.length !== 1) {
		const cl = Object.entries(sheets).filter(e => e[1]).map(e => `${e[1].file}: ${e[1].sheet.cols}`).join(', ');
		fail('the sheets have different numbers of directions: ' + cl);
	}
	const variations = cols[0];
	// shadows: either all or none
	const shadows = {};
	for (const n of Object.keys(sheets)) if (sheets[n]) shadows[n] = found.get(sheets[n].file.toLowerCase() + '_shadow') || null;
	const withShadow = Object.values(shadows).filter(Boolean).length;
	const hasShadow = withShadow > 0;
	if (hasShadow && withShadow !== Object.keys(shadows).filter(n => sheets[n]).length) {
		fail('there is a shadow (_shadow.rle) for some animations and not for others: provide it for all or for none');
	}
	for (const n of Object.keys(shadows)) {
		const s = shadows[n];
		if (s && (s.sheet.cols !== sheets[n].sheet.cols || s.sheet.rows !== sheets[n].sheet.rows)) {
			fail(`${s.file}.rle is ${s.sheet.cols}x${s.sheet.rows} and ${sheets[n].file}.rle is ${sheets[n].sheet.cols}x${sheets[n].sheet.rows}: they must have the same grid`);
		}
	}
	for (const [n, s] of Object.entries(sheets)) if (s) {
		if (s.sheet.rows > 62) fail(`${s.file}.rle has ${s.sheet.rows} frames; 62 at most`);
		if (s.sheet.mode !== rle.MODE_PAL && s.sheet.mode !== rle.MODE_PLAYER) {
			warnings.push(`${s.file}.rle has mode ${s.sheet.mode}; units use 1 (palette) or 2 (palette + player color)`);
		}
	}
	const modes = [...new Set(Object.values(sheets).filter(Boolean).map(s => s.sheet.mode))];
	if (modes.length > 1) warnings.push('the sheets mix color modes (with and without player color)');

	// images: color ones first, then the shadows
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
	// def: idx, name, startstate, endstate, sheet (key into sheets), reverse image, step, hit
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
		const sh = sheets[key].sheet, rows = sh.rows, raw = cfgOf(d.base);
		const c = { duration: raw.duration !== undefined ? raw.duration : raw.duracion, step: raw.step !== undefined ? raw.step : raw.paso, hit: raw.hit !== undefined ? raw.hit : raw.golpe };
		if (rows + 2 > 64) fail(`${sheets[key].file}.rle: ${rows} frames + 2 sentinels exceed 64`);
		const dur = c.duration || (fell ? 40 : defaults[d.base]);
		const durs = [];
		for (let r = 0; r < rows; r++) durs.push(dur);
		if (d.die && !c.duration) { // like the hastatus: the corpse stays and the last images last longer and longer
			const tail = [180, 500, 1000, 1000, 2000, 3000];
			if (rows >= 7) for (let k = 0; k < 6; k++) durs[rows - 6 + k] = tail[k]; else durs[rows - 1] = 3000;
		}
		const total = durs.reduce((a, b) => a + b, 0);
		const step = c.step !== undefined ? c.step : d.step;
		const hit = d.hit ? (c.hit !== undefined ? c.hit : Math.round(total * 0.43)) : d.taunt && !fell ? Math.round(total * 0.5) : 0;
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
			warnings.push(`${d.name}: there is no ${baseFile}.rle; the ${sheets[key].file} sheet is used`);
		}
	}
	body.push('</anims>');
	if (nLayers > 32 || count > 28) fail('game limits exceeded');
	if (variations !== 8) {
		warnings.push(`${variations} directions: the usual are 8 (soldiers), 12 (heroes and animals) or 4 (ships)`);
	}
	finish(`name="${name}" type="${esc(opt('--type', 'vx/unit'))}" variations="${variations}" pass_file=""`, body);
	console.log(`${count} animations, ${variations} directions, ${hasShadow ? 'with' : 'without'} shadow. In the class (.sc.xml): entity="units/${name}/${name}.ent.xml" if you leave it in units\\${name}\\ of your mod.`);
}

// =============================================================================================================================
function buildBuilding() {
	let layerNames;
	if (opt('--layers')) layerNames = opt('--layers').split(',').map(s => s.trim()).filter(Boolean);
	else layerNames = [...found.keys()].filter(k => /^layer\d+$/.test(k)).sort((a, b) => parseInt(a.slice(5)) - parseInt(b.slice(5)));
	if (!layerNames.length) fail(`no layers (layer1.rle, layer2.rle…) in ${dir}`);
	const list = layerNames.map(n => { const f = found.get(n.toLowerCase()); if (!f) fail(`${n}.rle does not exist in ${dir}`); return f; });
	const shadow = found.get('shadow');
	const all = shadow ? [...list, shadow] : list;
	const rows = [...new Set(all.map(s => s.sheet.rows))], cols = [...new Set(all.map(s => s.sheet.cols))];
	if (cols.length !== 1 || cols[0] !== 1) {
		const cl = all.map(s => `${s.file} ${s.sheet.cols}`).join(', ');
		fail('the .rle files of a building must have 1 column (a single direction); they have: ' + cl);
	}
	if (rows.length !== 1) {
		const rl = all.map(s => `${s.file} ${s.sheet.rows}`).join(', ');
		fail('all layers of a building must have the same rows (construction phases): ' + rl);
	}
	const nrows = rows[0];
	if (nrows > 8) fail(`${nrows} construction phases; the game allows 8 states at most`);
	if (nrows !== 4) warnings.push(`${nrows} construction phases: the game's buildings use 4`);
	const lc = {}, ls = {};
	if (shadow) ls.shadow = addImage(shadow.file + '.rle', shadow.sheet);
	list.forEach(s => { lc[s.file] = addImage(s.file + '.rle', s.sheet); });
	const body = [];
	// points
	if (opt('--points-from')) {
		const src = fs.readFileSync(opt('--points-from'), 'utf8'), m = /<points>[\s\S]*?<\/points>/.exec(src);
		if (!m) fail('--points-from: that file has no <points> block');
		body.push(...m[0].split(/\r?\n/));
	} else {
		body.push('<points>', '</points>');
		warnings.push('the building has no <point>: use --points-from <the .ent.xml of a similar building> to copy the entrance/post points');
	}
	body.push('<layers>');
	let li = 1;
	if (shadow) body.push(`<layer idx="${li++}" name="shadow" image="${ls.shadow}" z="800" offsetx="${offX}" offsety="${offY}" sortoffsetx="0" sortoffsety="0" xray="0"/>`);
	list.forEach(s => body.push(`<layer idx="${li++}" name="${esc(s.file)}" image="${lc[s.file]}" z="1000" offsetx="${offX}" offsety="${offY}" sortoffsetx="0" sortoffsety="0" xray="0"/>`));
	if (li - 1 > 32) fail('more than 32 layers');
	body.push('</layers>', '<states>');
	for (let i = 0; i < nrows; i++) body.push(`<state idx="${i + 1}" name="${i}" image_idx="0" image_row="0" offsetx="0" offsety="0" anim_idx="65536" anim_frame="65536"/>`);
	body.push('</states>', '<anims>', '</anims>');

	// footprint
	let passFile = '';
	if (opt('--footprint') || opt('--footprint-png')) {
		let cells, desc;
		if (opt('--footprint')) {
			const m = /^(\d+)x(\d+)$/i.exec(opt('--footprint'));
			if (!m) fail('--footprint must be WidthxHeight in cells, e.g. 6x6');
			try { cells = pass.rectangle(+m[1], +m[2]); } catch (e) { fail(e.message); }
			desc = `rectangle of ${m[1]}x${m[2]} cells`;
		} else {
			const img = png.decode(fs.readFileSync(opt('--footprint-png')));
			const mask = new Uint8Array(img.width * img.height);
			for (let i = 0; i < mask.length; i++) mask[i] = img.data[i * 4 + 3] >= 128 && (img.data[i * 4] + img.data[i * 4 + 1] + img.data[i * 4 + 2]) / 3 < 200 ? 1 : 0;
			try { cells = pass.fromMask(mask, img.width, img.height); } catch (e) { fail(e.message); }
			desc = `mask ${img.width}x${img.height}`;
		}
		const count = cells.reduce((a, b) => a + b, 0);
		if (!count) fail('the footprint is empty (no blocked cell)');
		passFile = name + '.pass';
		fs.writeFileSync(path.join(dir, passFile), pass.encode(cells));
		console.log(`Written ${path.join(dir, passFile)}  (${desc}, ${count} blocked cells; each cell = 16 map units)`);
	} else warnings.push('without --footprint or --footprint-png the building takes up no space on the map (people walk through it)');
	finish(`name="${name}" type="vx/building" variations="1" pass_file="${esc(passFile)}"`, body);
	console.log(`${list.length} layers${shadow ? ' + shadow' : ''}, ${nrows} construction phases. In the class (.sc.xml): entity="buildings/${name}/${name}.ent.xml" if you leave it in buildings\\${name}\\ of your mod.`);
}
