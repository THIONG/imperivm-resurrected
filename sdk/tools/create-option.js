// Creates a custom game option (a switch or a list of values in the lobby's RULES) for a mod (Imperivm Resurrected SDK).
//
//   node create-option.js <mod folder> <id> --name "Text" [--description "Text"] [--name-es "Texto"] [--description-es "Texto"]
//        [--name-it "Testo"] [--description-it "Testo"] [--values "Normal,Fast,Very fast"] [--values-es "Normal,Rápida,Muy rápida"]
//        [--values-it "Normale,Veloce,Molto veloce"] [--default <true|false | number or text of the value>] [--script]
//
// Writes to the mod folder (guide: options.md):
//   options\<id>.json        the option: a switch, or a list if you pass --values (2 to 16, comma-separated)
//   data\mod\<id>.vs         with --script: a behavior skeleton with Mod_GetOption / Mod_GetOptionValue ready to fill in
//   mod.json                 only if the folder does not have one
// Warns how many of the 8 reserved bits it uses. It overwrites nothing.
const { fs, path, lower, readUtf8, fail, parseArgs, onlyKnown, Plan, minimalModJson, run } = require('./lib/mod-common');

const USAGE = 'usage: node create-option.js <mod folder> <id> --name "Text" [--description "Text"] [--name-es "Texto"] [--description-es "Texto"]\n' +
	'       [--name-it "Testo"] [--description-it "Testo"] [--values "Normal,Fast,Very fast"] [--values-es "…"] [--values-it "…"]\n' +
	'       [--default <true|false or the value chosen by default>] [--script]';

// Bits used: a switch 1; a list, as many as its number of values needs (2: 1, 3-4: 2, 5-8: 3, 9-16: 4)
const bitsFor = n => { let w = 1; while ((1 << w) < n) w++; return w; };

// A short identifier from the text of a value: no accents, without what is in parentheses, with hyphens
function slug(t) {
	return t.replace(/\(.*$/, '').normalize('NFD').replace(/[̀-ͯ]/g, '').toLowerCase().replace(/[^a-z0-9]+/g, '-').replace(/^-+|-+$/g, '').slice(0, 32);
}
// One text per language: a plain text if there is only one, an object if there are several
function perLanguage(english, spanish, italian) {
	const o = {};
	if (english !== undefined) o.english = english;
	if (spanish !== undefined) o.spanish = spanish;
	if (italian !== undefined) o.italian = italian;
	return Object.keys(o).length > 1 || (Object.keys(o).length === 1 && !o.english) ? o : english;
}
const lit = v => typeof v === 'object' && v !== null && !Array.isArray(v) ? '{ ' + Object.entries(v).map(([k, x]) => JSON.stringify(k) + ': ' + lit(x)).join(', ') + ' }' : JSON.stringify(v);

run(() => {
	const { pos, opt } = parseArgs(process.argv.slice(2), ['script']);
	onlyKnown(opt, ['name', 'description', 'name-es', 'description-es', 'name-it', 'description-it', 'values', 'values-es', 'values-it', 'default', 'script']);
	const [modDir, idArg] = pos;
	if (!modDir || !idArg) fail(USAGE);
	if (pos.length > 2) fail('extra arguments: ' + pos.slice(2).join(' ') + '\n' + USAGE);
	if (!opt.name || opt.name === true) fail('missing --name "Text" (what is shown in the lobby)' + '\n' + USAGE);
	if (!/^[A-Za-z0-9_-]{1,32}$/.test(idArg)) fail('invalid id: "' + idArg + '" (letters without accents, digits, _ and -; up to 32). It is what scripts use.');
	const id = lower(idArg);
	for (const k of ['name', 'description', 'name-es', 'description-es', 'name-it', 'description-it', 'values', 'values-es', 'values-it', 'default']) {
		if (opt[k] !== undefined && (opt[k] === true || !String(opt[k]).trim())) fail('--' + k + ' is empty');
	}

	// The options the mod already has
	const dir = path.join(modDir, 'options');
	let used = 0;
	if (fs.existsSync(dir)) {
		for (const f of fs.readdirSync(dir)) {
			if (!/\.json$/i.test(f)) continue;
			let j;
			try { j = JSON.parse(readUtf8(path.join(dir, f))); } catch (e) { continue; }
			const other = lower(j.id !== undefined ? j.id : path.basename(f, '.json'));
			if (other === id) fail('there is already an option with the id "' + id + '" in your mod (options\\' + f + '): pick another id.');
			const v = j.values || j.values;
			used += Array.isArray(v) ? bitsFor(v.length) : 1;
		}
	}

	// Switch or list
	const isList = opt.values !== undefined;
	let values = null;
	if (!isList) {
		for (const k of ['values-es', 'values-it']) if (opt[k] !== undefined) fail('--' + k + ' without --values: do you want a list? Give --values too.');
	}
	else {
		const split = (k, s) => s.split(',').map(x => x.trim()).map((x, i) => { if (!x) fail('--' + k + ': value ' + (i + 1) + ' is empty (an extra comma?)'); return x; });
		const english = split('values', opt.values);
		if (english.length < 2) fail('a list needs at least 2 values (you gave ' + english.length + '). For a yes/no, do not give --values: you get a switch.');
		if (english.length > 16) fail('a list takes up to 16 values (you gave ' + english.length + ').');
		const spanish = opt['values-es'] !== undefined ? split('values-es', opt['values-es']) : null;
		const italian = opt['values-it'] !== undefined ? split('values-it', opt['values-it']) : null;
		if (spanish && spanish.length !== english.length) fail('--values-es has ' + spanish.length + ' values and --values has ' + english.length + ': they must be the same, in the same order.');
		if (italian && italian.length !== english.length) fail('--values-it has ' + italian.length + ' values and --values has ' + english.length + ': they must be the same, in the same order.');
		values = english.map((t, i) => ({ id: slug(t), name: perLanguage(t, spanish ? spanish[i] : undefined, italian ? italian[i] : undefined), _text: t }));
		values.forEach((v, i) => { if (!v.id) fail('the value "' + v._text + '" has no letters or digits: I cannot give it an id.'); });
		const seen = new Map();
		values.forEach((v, i) => {
			if (seen.has(v.id)) fail('values ' + (seen.get(v.id) + 1) + ' and ' + (i + 1) + ' would get the same id ("' + v.id + '"): change their text.');
			seen.set(v.id, i);
		});
	}

	// The default value
	let defaultValue;
	if (!isList) {
		if (opt.default === undefined) defaultValue = false;
		else if (/^(true|si|sí|yes|1)$/i.test(opt.default)) defaultValue = true;
		else if (/^(false|no|0)$/i.test(opt.default)) defaultValue = false;
		else fail('--default of a switch is true or false (you gave "' + opt.default + '")');
	}
	else if (opt.default === undefined) defaultValue = values[0].id;
	else if (/^\d+$/.test(opt.default)) {
		const n = Number(opt.default);
		if (n >= values.length) fail('--default ' + n + ': the list has ' + values.length + ' values, numbered from 0 (the last one is ' + (values.length - 1) + ').');
		defaultValue = values[n].id;
	}
	else {
		const q = lower(opt.default);
		const v = values.find(x => x.id === q || lower(x._text) === q);
		if (!v) fail('--default "' + opt.default + '" is none of the values (' + values.map(x => x.id).join(', ') + ').');
		defaultValue = v.id;
	}

	const plan = new Plan();
	const rJson = path.join(dir, id + '.json');
	const rScript = path.join(modDir, 'data', 'mod', id + '.vs');
	if (plan.exists(rJson)) fail(rJson + ' already exists: not overwriting it.');
	if (opt.script && plan.exists(rScript)) fail(rScript + ' already exists: not overwriting it. Remove --script or pick another id.');
	const bits = isList ? bitsFor(values.length) : 1;

	// options\<id>.json
	const rows = ['  "id": ' + JSON.stringify(id), '  "name": ' + lit(perLanguage(opt.name, opt['name-es'], opt['name-it']))];
	if (opt.description !== undefined || opt['description-es'] !== undefined || opt['description-it'] !== undefined) {
		rows.push('  "description": ' + lit(perLanguage(opt.description, opt['description-es'], opt['description-it'])));
	}
	if (isList) rows.push('  "values": [\n' + values.map(v => '    ' + lit({ id: v.id, name: v.name })).join(',\n') + '\n  ]');
	rows.push('  "default": ' + JSON.stringify(defaultValue));
	const text = '{\n' + rows.join(',\n') + '\n}\n';
	JSON.parse(text);  // just in case: what is written is JSON
	plan.put(rJson, text);

	// data\mod\<id>.vs
	if (opt.script) {
		const vs = ['// void, Obj This', ''];
		if (isList) vs.push('int nValue;', '');
		vs.push('Sleep(rand(500) + 100);', '');
		if (!isList) {
			vs.push('// Mod_GetOption("' + id + '") says whether the host ticked the switch in the lobby. It gives the same on all PCs.',
				'if (Mod_GetOption("' + id + '")) {',
				'\t// WRITE HERE what the option changes when it is ticked. "This" is the object of the class where you put this behavior.',
				'\tMod_Log("' + 'Option ' + id + ': enabled' + '");',
				'}');
		}
		else {
			vs.push('// Mod_GetOptionValue("' + id + '") gives the number of the value chosen in the lobby (0 is the first). It gives the same on all PCs.',
				'nValue = Mod_GetOptionValue("' + id + '");');
			values.forEach((v, i) => vs.push('// ' + i + ': ' + v._text));
			vs.push('if (nValue == 1) {',
				'\t// WRITE HERE what value 1 changes (copy the block for the others)',
				'\tMod_LogInt("' + 'Option ' + id + ': value' + '", nValue);',
				'}');
		}
		vs.push('', '// A behavior script must not end', 'while(true) Sleep(50000);', '');
		plan.put(rScript, vs.join('\n'));
	}

	const makeModJson = minimalModJson(plan, modDir, 'Gameplay & Balance', 'Game option "' + id + '". It changes gameplay: in multiplayer everyone must have it.');
	plan.write();

	console.log('option ' + id + ' (' + (isList ? 'list of ' + values.length + ' values' : 'switch') + ', default ' + JSON.stringify(defaultValue) + '): written to ' + modDir);
	for (const [r, it] of plan.items) console.log('  ' + (it.fresh ? 'new       ' : 'extended  ') + path.relative(modDir, r));
	if (makeModJson) console.log('  (minimal mod.json: change its name, authors and description)');
	console.log('Uses ' + bits + ' of the 8 bits reserved for options (this mod already used ' + used + '; with this one, ' + (used + bits) + ' of 8). The 8 are shared among all active mods.');
	if (used + bits > 8) console.log('warning: this mod alone already exceeds 8 bits: the last options (in alphabetical order of id) will be ignored.');
	if (idArg !== id) console.log('warning: the id is saved in lowercase ("' + id + '"); scripts are case-insensitive.');
	if (!opt.description) console.log('warning: without --description the option has no help text in the lobby.');
	if (!opt['name-es'] && !opt['name-it']) console.log('warning: without --name-es or --name-it, all languages see the same text.');
	console.log('To do:');
	if (opt.script) {
		console.log('  - complete data\\mod\\' + id + '.vs and hook it to a class (or an event) with a patch, for example in patches\\data\\classes\\Hero.sc.xml:');
		console.log('      <patch><add select="/class"><behavior script="data/mod/' + id + '.vs"/></add></patch>');
	}
	else console.log('  - a script that reads it: if (Mod_GetOption("' + id + '")) ' + (isList ? 'or Mod_GetOptionValue("' + id + '") ' : '') + '(with --script I generate the skeleton for you)');
	console.log('  - test it in the lobby: it appears under RULES; it only works in games with a lobby (not in adventures or campaigns)');
	console.log('  - run the validator: node validate-mod.js "' + modDir + '" --data <DATA>');
});
