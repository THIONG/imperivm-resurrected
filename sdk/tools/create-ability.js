// Creates the files for a new troop ability: the order (button and hotkey), the class method and a skeleton script (Imperivm Resurrected SDK).
//
//   node create-ability.js <DATA folder of the extracted data.pak> <mod folder> <id> --class <class> --key <letter>
//        [--energy-cost N] [--name "Text"] [--name-es "Texto"] [--description "Text"] [--description-es "Texto"] [--button <path>]
//
// Writes to the mod folder (guide: abilities.md):
//   data\commands\<Id>.xml                       the order: <cmd> with its <src obj="<class>"/>
//   patches\data\classes\<Class>.sc.xml          the patch that adds <method sig="<id>" vs="data/mod/<id>.vs"/> to the class (if a patch already exists, it is added to)
//   data\mod\<id>.vs                             a skeleton script with the correct signature; you write the body
//   loc\spanish.loc.xml                          the name and description in Spanish (if you pass --name-es or --description-es; added to whatever is already there)
//   mod.json                                     only if the folder does not have one
// It overwrites nothing: if a new file already exists, it stops without writing anything.
const { fs, path, lower, ci, readUtf8, xmlEsc, fail, parseArgs, integer, onlyKnown, dataFolder, Plan, beforeClosing, readClasses,
	readCommands, keyClashes, minimalModJson, run } = require('./lib/mod-common');

const USAGE = 'usage: node create-ability.js <DATA> <mod folder> <id> --class <class> --key <letter> [--energy-cost N] [--name "Text"]\n' +
	'       [--name-es "Texto"] [--description "Text"] [--description-es "Texto"] [--button <path of a game button, e.g. Actions/Mass Healing.bmp>]';

run(() => {
	const { pos, opt } = parseArgs(process.argv.slice(2));
	onlyKnown(opt, ['class', 'key', 'energy-cost', 'name', 'name-es', 'description', 'description-es', 'button']);
	const [dataDir, modDir, id] = pos;
	if (!dataDir || !modDir || !id) fail(USAGE);
	if (pos.length > 3) fail('extra arguments: ' + pos.slice(3).join(' ') + '\n' + USAGE);
	if (!opt.class) fail('missing --class (the class that receives the ability, e.g. Melee, RHastatus or Hero)' + '\n' + USAGE);
	if (!opt.key) fail('missing --key (a letter)' + '\n' + USAGE);
	dataFolder(dataDir);
	if (!/^[A-Za-z][A-Za-z0-9_]{0,31}$/.test(id)) fail('invalid id: "' + id + '" (letters, digits and _; must start with a letter; up to 32). It is the name of the command and of the method.');
	if (!/^[A-Za-z]$/.test(opt.key)) fail('--key must be a single letter (you gave "' + opt.key + '")');
	const key = lower(opt.key);
	const cost = integer(opt, 'energy-cost', 1, 9);
	if (opt['energy-cost'] !== undefined && cost === undefined) fail('--energy-cost: a number from 1 to 9');
	for (const k of ['name', 'name-es', 'description', 'description-es', 'button']) if (opt[k] !== undefined && !String(opt[k]).trim()) fail('--' + k + ' is empty');

	const classes = readClasses(dataDir, modDir);
	const info = classes.get(lower(opt.class));
	if (!info) {
		const similar = [...new Set([...classes.values()].map(c => c.id))].filter(c => lower(c).includes(lower(opt.class)) || lower(opt.class).includes(lower(c))).slice(0, 8);
		fail('the class "' + opt.class + '" does not exist in the game or in the mod.' + (similar.length ? ' Maybe ' + similar.join(', ') + '?' : ' Some common ones: Melee, Ranged, Military, Unit, Hero.'));
	}
	const commands = readCommands(dataDir, modDir);
	const repeated = commands.find(o => lower(o.name) === lower(id));
	if (repeated) fail('there is already a command "' + repeated.name + '" ' + (repeated.mod ? 'in your mod (' + repeated.file + ')' : 'in the game (' + repeated.file + ')') + ': pick another id.');

	const plan = new Plan();
	const warnings = [], missing = [];
	const cap = id[0].toUpperCase() + id.slice(1);
	const commandFile = path.join(modDir, 'data', 'commands', cap + '.xml');
	const scriptFile = path.join(modDir, 'data', 'mod', id + '.vs');
	for (const f of [commandFile, scriptFile]) if (plan.exists(f)) fail(f + ' already exists: not overwriting it. Delete the file or pick another id.');
	if (ci(path.join(modDir, 'data', 'commands'), cap + '.xml') || ci(path.join(modDir, 'data', 'mod'), id + '.vs')) fail('a file with that name already exists in your mod (different capitalization): pick another id.');

	// The class patch. A class that exists only in the mod is not patched: the <method> goes in the class's own file
	const method = '<method sig="' + id + '" vs="data/mod/' + id + '.vs"/>';
	if (info.mod) fail('the class ' + info.id + ' is from your mod (data\\classes\\' + info.file + '): add this line by hand inside <class>\n  ' + method + '\n  and run again with --class of a game class, or without that class.');
	const patchName = lower(info.file) === lower(info.id + '.sc.xml') ? info.id + '.sc.xml' : lower(info.file);
	const patchFile = path.join(modDir, 'patches', 'data', 'classes', patchName);
	const block = '  ' + ('<!-- The method that runs the command "' + id + '". Methods are inherited: every class under ' + info.id + ' has it -->') + '\n' +
		'  <add select="/class">\n    ' + method + '\n  </add>\n';
	const previous = plan.text(patchFile);
	if (previous === null) plan.put(patchFile, '<patch>\n' + block + '</patch>\n');
	else {
		if (new RegExp('sig\\s*=\\s*"' + id + '"', 'i').test(previous)) fail(patchFile + ' already adds a method "' + id + '".');
		plan.put(patchFile, beforeClosing(previous, '</patch>', block, patchFile));
	}

	// The hotkey
	const clashes = keyClashes(commands, classes, key, info.id);
	if (clashes.length) {
		const freeKeys = 'abcdefghijklmnopqrstuvwxyz'.split('').filter(t => !keyClashes(commands, classes, t, info.id).length);
		const who = clashes.map(o => o.name + ' (' + o.file + ', src ' + o.srcs.join('/') + ')').join(', ');
		warnings.push('the key "' + key + '" is already used in ' + info.id + ' (or in a related class): ' + who + '. It is generated anyway; free keys for ' + info.id + ': ' + (freeKeys.join(' ') || 'none') + '.');
	}

	// Texts: the rollover and the description are the English text; the translation goes in loc with the order as context
	const nameEs = opt['name-es'], descEs = opt['description-es'];
	const rollover = opt.name || cap;
	const description = opt.description || '';
	if (descEs && !description) missing.push('the English description (--description), or the command comes out without one');
	if (!opt.name) missing.push('the name (--name), or the button will say "' + rollover + '"');

	// Button
	const button = opt.button || 'Actions/Mass Healing.bmp';
	const gameButtons = new Set(commands.filter(o => o.button).map(o => lower(o.button)));
	const own = fs.existsSync(path.join(modDir, 'gameres', 'cmdbar', ...button.split('/')));
	if (!opt.button) missing.push('your own button (now it uses "' + button + '", the Mass Healing one): see images.md and pass --button Actions/<your button>.bmp');
	else if (!gameButtons.has(lower(button)) && !own) warnings.push('the button "' + button + '" is not used by any game command nor is it in gameres\\cmdbar of your mod: if it does not exist, the command will have no picture.');

	// The order
	const cmd = ['<commands>', '',
		'<!-- The command (the button). It is a new file: the game loads every .xml in data\\commands.',
		'     The command name ("' + id + '") is the same as the "sig" of the method that runs it (see patches\\data\\classes\\' + patchName + ').',
		'     It is an immediate command, like "stand_position" in data\\commands\\UNIT.XML -->',
		'<cmd name="' + xmlEsc(id) + '" priority="10"',
		'\toffset="1"',
		'\tbutton="' + xmlEsc(button) + '"',
		'\tkey="' + key + '"',
		'\trollover="' + xmlEsc(rollover) + '"'];
	if (description) cmd.push('\tdescription="' + xmlEsc(description) + '"');
	cmd.push('\tgroupverifier=""', '\tgroupdispatch=""', '\tvoidgroupdispatch="1"');
	if (cost) cmd.push('\tcoststamina="' + cost + '"');
	cmd.push('\t>', '\t' + ('<!-- Every class under ' + info.id + ' has the button -->'), '\t<src obj="' + info.id + '"/>', '</cmd>', '', '</commands>', '');
	plan.put(commandFile, cmd.join('\n'));

	// The skeleton script
	const vs = ['// void, Obj me', '',
		'// Ability "' + id + '" (class ' + info.id + ', key ' + key.toUpperCase() + '). The game runs this script when the player presses the button.',
		'// "me" is the unit that runs it. In a script, all variables are declared at the top.', '',
		'Unit this;'];
	if (cost) vs.push('int cost;');
	vs.push('', 'this = me.AsUnit();', 'if (!.IsAlive)', '\treturn;', '');
	if (cost) vs.push('// The game does not charge the energy of commands: the script does it', 'cost = GetCmdStaminaCost("' + id + '");', 'if (.stamina < cost)', '\treturn;', '.SetStamina(.stamina - cost);', '');
	vs.push('// WRITE HERE what the ability does. Some unit functions (full list in game-functions.txt):',
		'//   .Heal(60);                        heals 60 health',
		'//   .SetStamina(.stamina - 1);        changes the energy',
		'//   .SetSpeedFactor(150);             changes the speed (100 is normal)',
		'// A command with a target (a click on another unit) also receives the target: "// void, Obj This, Obj target"; see data\\subai\\druid_*.vs.',
		'',
		'Mod_Log("' + id + ': executed' + '");', '');
	plan.put(scriptFile, vs.join('\n'));
	missing.unshift('the body of the ability in data\\mod\\' + id + '.vs');

	// Spanish texts
	if (nameEs || (descEs && description && descEs !== description)) {
		const filePath = path.join(modDir, 'loc', 'spanish.loc.xml');
		const entries = [];
		if (nameEs && nameEs !== rollover) entries.push('  <translationtableentry text="' + xmlEsc(rollover) + '" context="' + xmlEsc(id) + '" result="' + xmlEsc(nameEs) + '"/>');
		if (descEs && description && descEs !== description) entries.push('  <translationtableentry text="' + xmlEsc(description) + '" context="' + xmlEsc(id) + '" result="' + xmlEsc(descEs) + '"/>');
		if (entries.length) {
			const previousLoc = plan.text(filePath);
			const fresh = '  ' + ('<!-- The name and description of a command are translated with the command name as context ("' + id + '") -->') + '\n' + entries.join('\n') + '\n';
			if (previousLoc === null) plan.put(filePath, '<translationtable>\n' + fresh + '</translationtable>\n');
			else plan.put(filePath, beforeClosing(previousLoc, '</translationtable>', fresh, filePath));
		}
	}

	const makeModJson = minimalModJson(plan, modDir, 'Units & Heroes', 'New ability "' + id + '". It changes gameplay: in multiplayer everyone must have it.');
	plan.write();

	console.log('ability ' + id + ' (class ' + info.id + ', key ' + key.toUpperCase() + (cost ? ', costs ' + cost + ' energy' : '') + '): written to ' + modDir);
	for (const [r, it] of plan.items) console.log('  ' + (it.fresh ? 'new       ' : 'extended  ') + path.relative(modDir, r));
	if (makeModJson) console.log('  (minimal mod.json: change its name, authors and description)');
	for (const a of warnings) console.log('warning: ' + a);
	console.log('To do:');
	for (const f of missing) console.log('  - ' + f);
	console.log('  - run the validator: node validate-mod.js "' + modDir + '" --data "' + dataDir + '"');
});
