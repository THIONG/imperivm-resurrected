// Creates the files for a new formation for the heroes: the formation, its two buttons and the script patch (Imperivm Resurrected SDK).
//
//   node create-formation.js <DATA folder of the extracted data.pak> <mod folder> <Name>
//        [--damage N] [--armor N] [--pierce-defense N] [--slash-defense N] [--range N] [--level N]
//        [--key x] [--like <game formation>] [--classes "Unit=wings,Horse=front,..."]
//        [--width N] [--height N] [--front N] [--wing-x N] [--wing-y N] [--description "Text"] [--button <path>]
//
// Writes to the mod folder (guide: formations.md). If patches for another formation already exist, it adds to them without overwriting:
//   patches\data\formations.xml                     the formation (bonuses, size and where each unit class goes)
//   patches\data\commands\Hero.xml                  its two buttons: form-<name>-menu and form-<name>
//   patches\data\subai\hero_formation_verify.vs     patch of the script that lets you pick the formation
//   mod.json                                        only if the folder does not have one
// --like copies from a game formation (Front, Line, Block, Cavalry core, Horse Wings, Ranged Wings) the size, the spacing and the list of classes,
// and the button image. Without --like it starts from Front. Whatever you set with --width, --height, --front, --wing-x, --wing-y and --classes overrides it.
const { fs, path, lower, ci, readLatin1, readUtf8, stripComments, xmlEsc, attrs, fail, parseArgs, integer, onlyKnown, dataFolder, Plan, beforeClosing,
	readClasses, readCommands, keyClashes, minimalModJson, run } = require('./lib/mod-common');

const USAGE = 'usage: node create-formation.js <DATA> <mod folder> <Name> [--damage N] [--armor N] [--pierce-defense N] [--slash-defense N] [--range N]\n' +
	'       [--level N] [--key x] [--like <game formation>] [--classes "Unit=wings,Horse=front"] [--width N] [--height N] [--front N]\n' +
	'       [--wing-x N] [--wing-y N] [--description "Text"] [--button <path>]';
const PLACES = { front: 'FrontLine', frontline: 'FrontLine', wings: 'Wings', center: 'CentralBlock', centre: 'CentralBlock', centralblock: 'CentralBlock' };
const BONUSES = [['damage', 'BonusDamage'], ['armor', 'BonusArmor'], ['pierce-defense', 'BonusDefencePierce'], ['slash-defense', 'BonusDefenceSlash'], ['range', 'BonusRange'], ['level', 'BonusLevel']];

run(() => {
	const { pos, opt } = parseArgs(process.argv.slice(2));
	onlyKnown(opt, [...BONUSES.map(b => b[0]), 'key', 'like', 'classes', 'width', 'height', 'front', 'wing-x', 'wing-y', 'description', 'button']);
	const [dataDir, modDir, name] = pos;
	if (!dataDir || !modDir || !name) fail(USAGE);
	if (pos.length > 3) fail('extra arguments: ' + pos.slice(3).join(' ') + ' (if the name has spaces, put it in quotes; no spaces is better)' + '\n' + USAGE);
	dataFolder(dataDir);
	if (!/^[A-Za-z][A-Za-z0-9]{0,23}$/.test(name)) fail('invalid name: "' + name + '" (letters and digits, no spaces; must start with a letter; up to 24). It is the formation\'s Name and appears in the command names.');
	const bonuses = {};
	for (const [k, attr] of BONUSES) { const v = integer(opt, k, -1000, 1000); if (v !== undefined) bonuses[attr] = v; }
	const measures = {};
	for (const [k, attr, min, max] of [['width', 'Width', 1, 20], ['height', 'Height', 1, 20], ['front', 'OffsetFrontLineByY', -500, 500], ['wing-x', 'OffsetWingsByX', -500, 500], ['wing-y', 'OffsetWingsByY', -500, 500]]) {
		const v = integer(opt, k, min, max);
		if (v !== undefined) measures[attr] = v;
	}
	if (opt.key !== undefined && !/^[A-Za-z]$/.test(opt.key)) fail('--key must be a single letter (you gave "' + opt.key + '")');
	for (const k of ['description', 'button', 'like', 'classes']) if (opt[k] !== undefined && !String(opt[k]).trim()) fail('--' + k + ' is empty');
	const slug = lower(name);

	// The game formation being copied from
	const fdir = ci(dataDir, 'formations.xml');
	if (!fdir) fail('the DATA folder has no formations.xml');
	const fxml = stripComments(readLatin1(path.join(dataDir, fdir)));
	const game = [...fxml.matchAll(/<FormationClass\b([^>]*)>([\s\S]*?)<\/FormationClass>/g)].map(m => ({
		a: attrs(m[1]), classes: [...m[2].matchAll(/<Class\b([^>]*?)\/?>/g)].map(c => attrs(c[1])) }));
	const base = game.find(f => lower(f.a.name) === lower(opt.like || 'Front'));
	if (!base) fail('there is no formation "' + opt.like + '" in the game. Available: ' + game.map(f => f.a.name).join(', ') + '.');
	if (game.some(f => lower(f.a.name) === slug)) fail('there is already a formation "' + name + '" in the game: pick another name.');

	// Where each class goes
	const classes = readClasses(dataDir, modDir);
	let list = base.classes.map(c => {
		const place = ['FrontLine', 'Wings', 'CentralBlock'].find(s => c[lower(s)] === '1');
		return { name: c.name, place };
	});
	if (opt.classes) {
		list = [];
		for (const par of opt.classes.split(',')) {
			const m = par.trim().match(/^([A-Za-z][A-Za-z0-9_]*)\s*=\s*([A-Za-z]+)$/);
			if (!m) fail('--classes: "' + par.trim() + '" is not valid. Write Class=place, with place front, wings or center (e.g. "Unit=wings,Horse=front").');
			const place = PLACES[lower(m[2])];
			if (!place) fail('--classes: the place "' + m[2] + '" does not exist (front, wings or center)');
			if (!classes.get(lower(m[1]))) fail('--classes: the class "' + m[1] + '" does not exist in the game or in the mod');
			if (list.some(c => lower(c.name) === lower(m[1]))) fail('--classes: the class "' + m[1] + '" is repeated');
			list.push({ name: classes.get(lower(m[1])).id, place });
		}
	}
	if (!list.length) fail('the formation has no unit class: give --classes or pick another base with --like');

	// Names already used in the mod (formation and orders) and in the game (orders)
	const commands = readCommands(dataDir, modDir);
	for (const n of ['form-' + slug + '-menu', 'form-' + slug]) {
		const r = commands.find(o => lower(o.name) === n);
		if (r) fail('there is already a command "' + r.name + '" ' + (r.mod ? 'in your mod (' + r.file + ')' : 'in the game (' + r.file + ')') + ': that formation already exists or its name clashes. Pick another name.');
	}
	const plan = new Plan();
	const warnings = [], missing = [];
	const rFormations = path.join(modDir, 'patches', 'data', 'formations.xml');
	const rHero = path.join(modDir, 'patches', 'data', 'commands', 'Hero.xml');
	const rScript = path.join(modDir, 'patches', 'data', 'subai', 'hero_formation_verify.vs');
	for (const r of [rFormations, rHero, rScript]) {
		const dir = path.dirname(r);
		if (fs.existsSync(dir) && !fs.readdirSync(dir).some(f => f === path.basename(r)) && ci(dir, path.basename(r))) fail(path.join(dir, ci(dir, path.basename(r))) + ' already exists with different capitalization: rename it to ' + path.basename(r) + '.');
	}
	const prevF = plan.text(rFormations);
	if (prevF !== null && new RegExp('Name\\s*=\\s*"' + name + '"', 'i').test(prevF)) fail(rFormations + ' already has a formation "' + name + '".');
	const prevS = plan.text(rScript);
	if (prevS !== null && prevS.includes('form-' + slug + '-menu')) fail(rScript + ' already handles the formation "' + name + '".');

	// The hotkey: free for the hero (which inherits those of Unit and Military)
	let key = opt.key ? lower(opt.key) : null;
	const free = t => !keyClashes(commands, classes, t, 'Hero').length;
	if (!key) {
		key = 'ijkqz'.split('').concat('abcdefghijklmnopqrstuvwxyz'.split('')).find(free);
		if (!key) fail('no free letter left for the hero (the game and your mod already use all 26). Give --key with the letter you prefer: it is generated anyway and the clash is reported.');
		warnings.push('no --key given: I picked "' + key + '", which is free for the hero.');
	}
	else if (!free(key)) {
		const freeKeys = 'abcdefghijklmnopqrstuvwxyz'.split('').filter(free);
		const who = keyClashes(commands, classes, key, 'Hero').map(o => o.name + ' (' + o.file + ')').join(', ');
		warnings.push('the key "' + key + '" is already used by the hero: ' + who + '. It is generated anyway; free keys: ' + (freeKeys.join(' ') || 'none') + '.');
	}

	// Button: that of the base formation, or the one you ask for
	const buttons = new Set(commands.filter(o => o.button).map(o => lower(o.button)));
	const fromBase = commands.find(o => /^form-/.test(o.name) && o.param && lower(o.param) === lower(base.a.name) && o.button);
	const button = opt.button || (fromBase && fromBase.button) || 'States/front form.bmp';
	if (opt.button) {
		if (!buttons.has(lower(button)) && !fs.existsSync(path.join(modDir, 'gameres', 'cmdbar', ...button.split('/')))) warnings.push('the button "' + button + '" is not used by any game command nor is it in gameres\\cmdbar of your mod: if it does not exist, it will have no picture.');
	}
	else missing.push('your own picture for the buttons (now they use "' + button + '"): see images.md and pass --button States/<your button>.bmp');

	// 1. formations.xml
	const med = k => measures[k] !== undefined ? measures[k] : base.a[lower(k)];
	const f = ['    <FormationClass Name="' + xmlEsc(name) + '"', '        Description="' + xmlEsc(opt.description || 'The warriors form a ' + name + ' formation.') + '"'];
	for (const k of ['Width', 'Height']) if (med(k) !== undefined) f.push('        ' + k + '="' + med(k) + '"');
	for (const [, attr] of BONUSES) if (bonuses[attr] !== undefined) f.push('        ' + attr + '="' + bonuses[attr] + '"');
	for (const k of ['OffsetFrontLineByY', 'OffsetWingsByX', 'OffsetWingsByY']) f.push('        ' + k + '="' + (med(k) !== undefined ? med(k) : 0) + '"');
	f[f.length - 1] += '>';
	f.push('');
	for (const c of list) f.push('        <Class Name="' + xmlEsc(c.name) + '"' + (c.place ? ' ' + c.place + '="1"' : '') + '/>');
	f.push('    </FormationClass>');
	if (!Object.keys(bonuses).length) warnings.push('the formation gives no bonus (no --damage, --armor, --pierce-defense, --slash-defense, --range or --level): it only changes the placement.');
	const blockF = '  ' + '<!-- A new formation. Width/Height: size of the grid of the first line. Bonus*: bonuses while the troop is standing still (stand ground).' + '\n' +
		'       ' + 'Description is not shown on the button: the game uses its own text.' + '\n' +
		'       ' + 'Each <Class> says where the units of that class go: FrontLine (in front), Wings (on the sides) or CentralBlock (in the center) -->' + '\n' +
		'  <add select="/Formations">\n' + f.join('\n') + '\n  </add>\n';
	plan.put(rFormations, prevF === null ? '<patch>\n' + blockF + '</patch>\n' : beforeClosing(prevF, '</patch>', blockF, rFormations));

	// 2. Hero.xml: the two buttons
	const blockH = '  ' + ('<!-- The two buttons of the formation ' + name + ': the one shown when it is the current one ("-menu", opens the list) and the one that selects it.') + '\n' +
		(opt.button ? '       ' + ('They use the picture ' + button + '. -->') + '\n' : '       ' + 'Here they use the picture of another formation; for your own, see the images guide. -->' + '\n') +
		'  <add select="/commands">\n' +
		'    <cmd name="form-' + slug + '-menu" priority="1"\n        method="move"\n        button="' + xmlEsc(button) + '"\n        key="f"\n        rollover="Select Formation"\n' +
		'        param="' + xmlEsc(name) + '"\n        groupverifier="data/subai/hero_formation_menu_verify.vs"\n        menu="1">\n      <src obj="Hero"/>\n    </cmd>\n\n' +
		'    <cmd name="form-' + slug + '" priority="1"\n        method="formation"\n        button="' + xmlEsc(button) + '"\n        immediate="yes"\n        key="' + key + '"\n' +
		'        rollover="Select ' + xmlEsc(name) + ' formation"\n        param="' + xmlEsc(name) + '"\n        groupverifier="data/subai/hero_formation_verify.vs"\n        menu="1">\n      <src obj="Hero"/>\n    </cmd>\n' +
		'  </add>\n';
	const prevH = plan.text(rHero);
	plan.put(rHero, prevH === null ? '<patch>\n' + blockH + '</patch>\n' : beforeClosing(prevH, '</patch>', blockH, rHero));

	// 3. The script: the game only lets you choose a formation when one of its menus is open, named in a list
	const anchor = 'if (cmd_name != "form-front-menu")';
	const vsDir = ci(dataDir, 'subai');
	const vsGame = vsDir && ci(path.join(dataDir, vsDir), 'hero_formation_verify.vs');
	if (!vsGame) fail('the DATA folder has no subai\\hero_formation_verify.vs');
	if (!readLatin1(path.join(dataDir, vsDir, vsGame)).includes(anchor)) fail('the game script subai\\hero_formation_verify.vs no longer has the line ' + anchor + ': this tool does not know how to patch it.');
	const blockS = '@replace ' + anchor + '\n' + anchor + '\n\tif (cmd_name != "form-' + slug + '-menu")\n@end\n';
	if (prevS === null) {
		plan.put(rScript, '// The game script only lets you choose a formation when one of its four menus ("form-xxx-menu") is open:\n// the menu of each new formation is added to that list. Each @replace block adds to the others.\n' + blockS);
	}
	else {
		let t = prevS.replace(/\s*$/, '\n');
		if (!/@(end|fin)\s*$/.test(t)) warnings.push('the script patch that was already there does not end in @end: I closed it with another @end when adding your formation.');
		plan.put(rScript, t + '\n' + blockS);
	}

	const makeModJson = minimalModJson(plan, modDir, 'Gameplay & Balance', 'New formation "' + name + '" for heroes. It changes gameplay: in multiplayer everyone must have it.');
	plan.write();

	console.log('formation ' + name + ' (placement of ' + base.a.name + (opt.classes ? ' changed by --classes' : '') + ', select button: key ' + key.toUpperCase() + '): written to ' + modDir);
	for (const [r, it] of plan.items) console.log('  ' + (it.fresh ? 'new       ' : 'extended  ') + path.relative(modDir, r));
	if (makeModJson) console.log('  (minimal mod.json: change its name, authors and description)');
	for (const a of warnings) console.log('warning: ' + a);
	console.log('To do:');
	for (const x of missing) console.log('  - ' + x);
	console.log('  - test in game: open the formations menu of a hero with troops (bonuses only apply while the troop is standing still)');
	console.log('  - run the validator: node validate-mod.js "' + modDir + '" --data "' + dataDir + '"');
});
