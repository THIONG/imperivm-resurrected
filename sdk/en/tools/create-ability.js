// Creates the files for a new troop ability: the order (button and hotkey), the class method and a skeleton script (Imperivm Resurrected SDK).
//
//   node create-ability.js <DATA folder of the extracted data.pak> <mod folder> <id> --class <class> --key <letter>
//        [--energy-cost N] [--name "Text"] [--name-en "Text"] [--description "Text"] [--description-en "Text"] [--button <path>]
//
// Writes to the mod folder (guide: abilities.md):
//   data\commands\<Id>.xml                       the order: <cmd> with its <src obj="<class>"/>
//   patches\data\classes\<Class>.sc.xml          the patch that adds <method sig="<id>" vs="data/mod/<id>.vs"/> to the class (if a patch already exists, it is added to)
//   data\mod\<id>.vs                             a skeleton script with the correct signature; you write the body
//   loc\spanish.loc.xml                          the name and description in Spanish (if you pass --name; added to whatever is already there)
//   mod.json                                     only if the folder does not have one
// It overwrites nothing: if a new file already exists, it stops without writing anything.
const { fs, path, lower, ci, readUtf8, xmlEsc, fallo, parseArgs, entero, soloConocidas, carpetaData, Plan, antesDelCierre, leerClases,
	leerOrdenes, chocaTecla, modJsonMinimo, ejecutar } = require('./lib/mod-common');
const { T } = require('./lib/lang');

const USO = T('uso: node create-ability.js <DATA> <carpeta del mod> <id> --clase <clase> --tecla <letra> [--coste-energia N] [--nombre "Texto"]\n' +
	'       [--nombre-en "Text"] [--descripcion "Texto"] [--descripcion-en "Text"] [--boton <ruta de un botón del juego, p. ej. Actions/Mass Healing.bmp>]',
	'usage: node create-ability.js <DATA> <mod folder> <id> --clase <class> --tecla <letter> [--coste-energia N] [--nombre "Text"]\n' +
	'       [--nombre-en "Text"] [--descripcion "Text"] [--descripcion-en "Text"] [--boton <path of a game button, e.g. Actions/Mass Healing.bmp>]',
	'uso: node create-ability.js <DATA> <cartella della mod> <id> --clase <classe> --tecla <lettera> [--coste-energia N] [--nombre "Testo"]\n' +
	'       [--nombre-en "Text"] [--descripcion "Testo"] [--descripcion-en "Text"] [--boton <percorso di un pulsante del gioco, es. Actions/Mass Healing.bmp>]');

ejecutar(() => {
	const { pos, opt } = parseArgs(process.argv.slice(2));
	soloConocidas(opt, ['clase', 'tecla', 'coste-energia', 'nombre', 'nombre-en', 'descripcion', 'descripcion-en', 'boton']);
	const [dataDir, modDir, id] = pos;
	if (!dataDir || !modDir || !id) fallo(USO);
	if (pos.length > 3) fallo(T('sobran argumentos: ' + pos.slice(3).join(' '), 'extra arguments: ' + pos.slice(3).join(' '), 'argomenti in più: ' + pos.slice(3).join(' ')) + '\n' + USO);
	if (!opt.clase) fallo(T('falta --clase (la clase que recibe la habilidad, p. ej. Melee, RHastatus o Hero)', 'missing --clase (the class that receives the ability, e.g. Melee, RHastatus or Hero)', 'manca --clase (la classe che riceve l\'abilità, es. Melee, RHastatus o Hero)') + '\n' + USO);
	if (!opt.tecla) fallo(T('falta --tecla (una letra)', 'missing --tecla (a letter)', 'manca --tecla (una lettera)') + '\n' + USO);
	carpetaData(dataDir);
	if (!/^[A-Za-z][A-Za-z0-9_]{0,31}$/.test(id)) fallo(T('id no válido: "' + id + '" (letras, números y _; empieza por letra; hasta 32). Es el nombre de la orden y del método.', 'invalid id: "' + id + '" (letters, digits and _; must start with a letter; up to 32). It is the name of the command and of the method.', 'id non valido: "' + id + '" (lettere, numeri e _; inizia con una lettera; fino a 32). È il nome del comando e del metodo.'));
	if (!/^[A-Za-z]$/.test(opt.tecla)) fallo(T('--tecla tiene que ser una sola letra (has puesto "' + opt.tecla + '")', '--tecla must be a single letter (you gave "' + opt.tecla + '")', '--tecla deve essere una sola lettera (hai messo "' + opt.tecla + '")'));
	const tecla = lower(opt.tecla);
	const coste = entero(opt, 'coste-energia', 1, 9);
	if (opt['coste-energia'] !== undefined && coste === undefined) fallo(T('--coste-energia: número de 1 a 9', '--coste-energia: a number from 1 to 9', '--coste-energia: un numero da 1 a 9'));
	for (const k of ['nombre', 'nombre-en', 'descripcion', 'descripcion-en', 'boton']) if (opt[k] !== undefined && !String(opt[k]).trim()) fallo(T('--' + k + ' está vacío', '--' + k + ' is empty', '--' + k + ' è vuoto'));

	const clases = leerClases(dataDir, modDir);
	const info = clases.get(lower(opt.clase));
	if (!info) {
		const parecidas = [...new Set([...clases.values()].map(c => c.id))].filter(c => lower(c).includes(lower(opt.clase)) || lower(opt.clase).includes(lower(c))).slice(0, 8);
		fallo(T('la clase "' + opt.clase + '" no existe en el juego ni en el mod.' + (parecidas.length ? ' ¿Quizá ' + parecidas.join(', ') + '?' : ' Algunas habituales: Melee, Ranged, Military, Unit, Hero.'),
			'the class "' + opt.clase + '" does not exist in the game or in the mod.' + (parecidas.length ? ' Maybe ' + parecidas.join(', ') + '?' : ' Some common ones: Melee, Ranged, Military, Unit, Hero.'),
			'la classe "' + opt.clase + '" non esiste nel gioco né nella mod.' + (parecidas.length ? ' Forse ' + parecidas.join(', ') + '?' : ' Alcune comuni: Melee, Ranged, Military, Unit, Hero.')));
	}
	const ordenes = leerOrdenes(dataDir, modDir);
	const repetida = ordenes.find(o => lower(o.name) === lower(id));
	if (repetida) fallo(T('ya hay una orden "' + repetida.name + '" ' + (repetida.mod ? 'en tu mod (' + repetida.file + ')' : 'en el juego (' + repetida.file + ')') + ': elige otro id.',
		'there is already a command "' + repetida.name + '" ' + (repetida.mod ? 'in your mod (' + repetida.file + ')' : 'in the game (' + repetida.file + ')') + ': pick another id.',
		'c\'è già un comando "' + repetida.name + '" ' + (repetida.mod ? 'nella tua mod (' + repetida.file + ')' : 'nel gioco (' + repetida.file + ')') + ': scegli un altro id.'));

	const plan = new Plan();
	const avisos = [], falta = [];
	const cap = id[0].toUpperCase() + id.slice(1);
	const ficheroOrden = path.join(modDir, 'data', 'commands', cap + '.xml');
	const ficheroScript = path.join(modDir, 'data', 'mod', id + '.vs');
	for (const f of [ficheroOrden, ficheroScript]) if (plan.existe(f)) fallo(T('ya existe ' + f + ': no lo piso. Borra el archivo o elige otro id.', f + ' already exists: not overwriting it. Delete the file or pick another id.', f + ' esiste già: non lo sovrascrivo. Cancella il file o scegli un altro id.'));
	if (ci(path.join(modDir, 'data', 'commands'), cap + '.xml') || ci(path.join(modDir, 'data', 'mod'), id + '.vs')) fallo(T('ya existe un archivo con ese nombre en tu mod (otra mayúscula): elige otro id.', 'a file with that name already exists in your mod (different capitalization): pick another id.', 'esiste già un file con quel nome nella tua mod (maiuscole diverse): scegli un altro id.'));

	// The class patch. A class that exists only in the mod is not patched: the <method> goes in the class's own file
	const metodo = '<method sig="' + id + '" vs="data/mod/' + id + '.vs"/>';
	if (info.mod) fallo(T('la clase ' + info.id + ' es de tu mod (data\\classes\\' + info.file + '): añade a mano dentro de <class> la línea\n  ' + metodo + '\n  y vuelve a ejecutar con --clase de una clase del juego, o sin esa clase.',
		'the class ' + info.id + ' is from your mod (data\\classes\\' + info.file + '): add this line by hand inside <class>\n  ' + metodo + '\n  and run again with --clase of a game class, or without that class.',
		'la classe ' + info.id + ' è della tua mod (data\\classes\\' + info.file + '): aggiungi a mano dentro <class> la riga\n  ' + metodo + '\n  e rilancia con --clase di una classe del gioco, o senza quella classe.'));
	const parcheNombre = lower(info.file) === lower(info.id + '.sc.xml') ? info.id + '.sc.xml' : lower(info.file);
	const ficheroParche = path.join(modDir, 'patches', 'data', 'classes', parcheNombre);
	const bloque = '  ' + T('<!-- El método que ejecuta la orden "' + id + '". Los métodos se heredan: lo tienen todas las clases que cuelgan de ' + info.id + ' -->',
		'<!-- The method that runs the command "' + id + '". Methods are inherited: every class under ' + info.id + ' has it -->',
		'<!-- Il metodo che esegue il comando "' + id + '". I metodi si ereditano: ce l\'hanno tutte le classi che discendono da ' + info.id + ' -->') + '\n' +
		'  <add select="/class">\n    ' + metodo + '\n  </add>\n';
	const previo = plan.texto(ficheroParche);
	if (previo === null) plan.poner(ficheroParche, '<patch>\n' + bloque + '</patch>\n');
	else {
		if (new RegExp('sig\\s*=\\s*"' + id + '"', 'i').test(previo)) fallo(T(ficheroParche + ' ya añade un método "' + id + '".', ficheroParche + ' already adds a method "' + id + '".', ficheroParche + ' aggiunge già un metodo "' + id + '".'));
		plan.poner(ficheroParche, antesDelCierre(previo, '</patch>', bloque, ficheroParche));
	}

	// The hotkey
	const choques = chocaTecla(ordenes, clases, tecla, info.id);
	if (choques.length) {
		const libres = 'abcdefghijklmnopqrstuvwxyz'.split('').filter(t => !chocaTecla(ordenes, clases, t, info.id).length);
		const quien = choques.map(o => o.name + ' (' + o.file + ', src ' + o.srcs.join('/') + ')').join(', ');
		avisos.push(T('la tecla "' + tecla + '" ya la usa en ' + info.id + ' (o en una clase emparentada): ' + quien + '. Se genera igual; teclas libres para ' + info.id + ': ' + (libres.join(' ') || 'ninguna') + '.',
			'the key "' + tecla + '" is already used in ' + info.id + ' (or in a related class): ' + quien + '. It is generated anyway; free keys for ' + info.id + ': ' + (libres.join(' ') || 'none') + '.',
			'il tasto "' + tecla + '" è già usato in ' + info.id + ' (o in una classe imparentata): ' + quien + '. Viene generato comunque; tasti liberi per ' + info.id + ': ' + (libres.join(' ') || 'nessuno') + '.'));
	}

	// Texts: the rollover and the description are the English text; the translation goes in loc with the order as context
	const ascii = s => /^[\x20-\x7e]*$/.test(s);
	const nombre = opt.nombre, desc = opt.descripcion;
	const rollover = opt['nombre-en'] || (nombre && ascii(nombre) ? nombre : cap);
	if (nombre && !opt['nombre-en'] && !ascii(nombre)) avisos.push(T('--nombre lleva acentos y no has puesto --nombre-en: el botón dice "' + rollover + '" en inglés y "' + nombre + '" en español (por loc).', '--nombre has accents and you did not give --nombre-en: the button says "' + rollover + '" in English and "' + nombre + '" in Spanish (through loc).', '--nombre ha degli accenti e non hai messo --nombre-en: il pulsante dice "' + rollover + '" in inglese e "' + nombre + '" in spagnolo (tramite loc).'));
	let description = opt['descripcion-en'] || (desc && ascii(desc) ? desc : '');
	if (desc && !description) avisos.push(T('--descripcion lleva acentos y no has puesto --descripcion-en: la orden sale sin descripción. Pon --descripcion-en "…".', '--descripcion has accents and you did not give --descripcion-en: the command comes out without a description. Give --descripcion-en "…".', '--descripcion ha degli accenti e non hai messo --descripcion-en: il comando esce senza descrizione. Metti --descripcion-en "…".'));
	if (opt['descripcion-en'] && !desc) falta.push(T('la descripción en español (--descripcion), o saldrá en inglés', 'the description in Spanish (--descripcion), or it will show in English', 'la descrizione in spagnolo (--descripcion), o uscirà in inglese'));
	if (!nombre) falta.push(T('el nombre en español (--nombre), o el botón dirá "' + rollover + '"', 'the name in Spanish (--nombre), or the button will say "' + rollover + '"', 'il nome in spagnolo (--nombre), o il pulsante dirà "' + rollover + '"'));

	// Button
	const boton = opt.boton || 'Actions/Mass Healing.bmp';
	const botonesDelJuego = new Set(ordenes.filter(o => o.button).map(o => lower(o.button)));
	const propio = fs.existsSync(path.join(modDir, 'gameres', 'cmdbar', ...boton.split('/')));
	if (!opt.boton) falta.push(T('un botón propio (ahora usa "' + boton + '", el de Curación en masa): ver images.md y pasa --boton Actions/<tu botón>.bmp', 'your own button (now it uses "' + boton + '", the Mass Healing one): see images.md and pass --boton Actions/<your button>.bmp', 'un pulsante tuo (ora usa "' + boton + '", quello di Cura di massa): vedi images.md e passa --boton Actions/<tuo pulsante>.bmp'));
	else if (!botonesDelJuego.has(lower(boton)) && !propio) avisos.push(T('el botón "' + boton + '" no lo usa ninguna orden del juego ni está en gameres\\cmdbar de tu mod: si no existe, la orden saldrá sin dibujo.', 'the button "' + boton + '" is not used by any game command nor is it in gameres\\cmdbar of your mod: if it does not exist, the command will have no picture.', 'il pulsante "' + boton + '" non è usato da nessun comando del gioco e non è in gameres\\cmdbar della tua mod: se non esiste, il comando uscirà senza immagine.'));

	// The order
	const cmd = ['<commands>', '',
		T('<!-- La orden (el botón). Es un archivo nuevo: el juego carga todos los .xml de data\\commands.', '<!-- The command (the button). It is a new file: the game loads every .xml in data\\commands.', '<!-- Il comando (il pulsante). È un file nuovo: il gioco carica tutti i .xml di data\\commands.'),
		T('     El nombre de la orden ("' + id + '") es el mismo que el "sig" del método que la ejecuta (ver patches\\data\\classes\\' + parcheNombre + ').', '     The command name ("' + id + '") is the same as the "sig" of the method that runs it (see patches\\data\\classes\\' + parcheNombre + ').', '     Il nome del comando ("' + id + '") è lo stesso del "sig" del metodo che lo esegue (vedi patches\\data\\classes\\' + parcheNombre + ').'),
		T('     Es una orden inmediata, como "stand_position" de data\\commands\\UNIT.XML -->', '     It is an immediate command, like "stand_position" in data\\commands\\UNIT.XML -->', '     È un comando immediato, come "stand_position" di data\\commands\\UNIT.XML -->'),
		'<cmd name="' + xmlEsc(id) + '" priority="10"',
		'\toffset="1"',
		'\tbutton="' + xmlEsc(boton) + '"',
		'\tkey="' + tecla + '"',
		'\trollover="' + xmlEsc(rollover) + '"'];
	if (description) cmd.push('\tdescription="' + xmlEsc(description) + '"');
	cmd.push('\tgroupverifier=""', '\tgroupdispatch=""', '\tvoidgroupdispatch="1"');
	if (coste) cmd.push('\tcoststamina="' + coste + '"');
	cmd.push('\t>', '\t' + T('<!-- Todas las clases que cuelgan de ' + info.id + ' tienen el botón -->', '<!-- Every class under ' + info.id + ' has the button -->', '<!-- Tutte le classi che discendono da ' + info.id + ' hanno il pulsante -->'), '\t<src obj="' + info.id + '"/>', '</cmd>', '', '</commands>', '');
	plan.poner(ficheroOrden, cmd.join('\n'));

	// The skeleton script
	const vs = ['// void, Obj me', '',
		T('// Habilidad "' + id + '" (clase ' + info.id + ', tecla ' + tecla.toUpperCase() + '). El juego ejecuta este script cuando el jugador pulsa el botón.', '// Ability "' + id + '" (class ' + info.id + ', key ' + tecla.toUpperCase() + '). The game runs this script when the player presses the button.', '// Abilità "' + id + '" (classe ' + info.id + ', tasto ' + tecla.toUpperCase() + '). Il gioco esegue questo script quando il giocatore preme il pulsante.'),
		T('// "me" es la unidad que la ejecuta. En un script, las variables se declaran todas al principio.', '// "me" is the unit that runs it. In a script, all variables are declared at the top.', '// "me" è l\'unità che la esegue. In uno script, le variabili si dichiarano tutte all\'inizio.'), '',
		'Unit this;'];
	if (coste) vs.push('int cost;');
	vs.push('', 'this = me.AsUnit();', 'if (!.IsAlive)', '\treturn;', '');
	if (coste) vs.push(T('// El juego no cobra la energía de las órdenes: lo hace el script', '// The game does not charge the energy of commands: the script does it', '// Il gioco non scala l\'energia dei comandi: lo fa lo script'), 'cost = GetCmdStaminaCost("' + id + '");', 'if (.stamina < cost)', '\treturn;', '.SetStamina(.stamina - cost);', '');
	vs.push(T('// ESCRIBE AQUÍ lo que hace la habilidad. Algunas funciones de unidad (lista completa en funciones-del-juego.txt):', '// WRITE HERE what the ability does. Some unit functions (full list in funciones-del-juego.txt):', '// SCRIVI QUI cosa fa l\'abilità. Alcune funzioni di unità (elenco completo in funciones-del-juego.txt):'),
		T('//   .Heal(60);                        cura 60 de vida', '//   .Heal(60);                        heals 60 health', '//   .Heal(60);                        cura 60 di vita'),
		T('//   .SetStamina(.stamina - 1);        cambia la energía', '//   .SetStamina(.stamina - 1);        changes the energy', '//   .SetStamina(.stamina - 1);        cambia l\'energia'),
		T('//   .SetSpeedFactor(150);             cambia la velocidad (100 es la normal)', '//   .SetSpeedFactor(150);             changes the speed (100 is normal)', '//   .SetSpeedFactor(150);             cambia la velocità (100 è quella normale)'),
		T('// Una orden con objetivo (un clic sobre otra unidad) recibe además el objetivo: "// void, Obj This, Obj target"; mira data\\subai\\druid_*.vs.', '// A command with a target (a click on another unit) also receives the target: "// void, Obj This, Obj target"; see data\\subai\\druid_*.vs.', '// Un comando con bersaglio (un clic su un\'altra unità) riceve anche il bersaglio: "// void, Obj This, Obj target"; guarda data\\subai\\druid_*.vs.'),
		'',
		'Mod_Log("' + id + T(': ejecutada', ': executed', ': eseguita') + '");', '');
	plan.poner(ficheroScript, vs.join('\n'));
	falta.unshift(T('el cuerpo de la habilidad en data\\mod\\' + id + '.vs', 'the body of the ability in data\\mod\\' + id + '.vs', 'il corpo dell\'abilità in data\\mod\\' + id + '.vs'));

	// Spanish texts
	if (nombre || (desc && description && desc !== description)) {
		const ruta = path.join(modDir, 'loc', 'spanish.loc.xml');
		const entradas = [];
		if (nombre && nombre !== rollover) entradas.push('  <translationtableentry text="' + xmlEsc(rollover) + '" context="' + xmlEsc(id) + '" result="' + xmlEsc(nombre) + '"/>');
		if (desc && description && desc !== description) entradas.push('  <translationtableentry text="' + xmlEsc(description) + '" context="' + xmlEsc(id) + '" result="' + xmlEsc(desc) + '"/>');
		if (entradas.length) {
			const previoLoc = plan.texto(ruta);
			const nuevo = '  ' + T('<!-- El nombre y la descripción de una orden se traducen con el nombre de la orden como contexto ("' + id + '") -->', '<!-- The name and description of a command are translated with the command name as context ("' + id + '") -->', '<!-- Il nome e la descrizione di un comando si traducono con il nome del comando come contesto ("' + id + '") -->') + '\n' + entradas.join('\n') + '\n';
			if (previoLoc === null) plan.poner(ruta, '<translationtable>\n' + nuevo + '</translationtable>\n');
			else plan.poner(ruta, antesDelCierre(previoLoc, '</translationtable>', nuevo, ruta));
		}
	}

	const creaModJson = modJsonMinimo(plan, modDir, 'Units & Heroes', T('Habilidad nueva "' + id + '". Cambia la jugabilidad: en multijugador todos tienen que llevarlo.', 'New ability "' + id + '". It changes gameplay: in multiplayer everyone must have it.', 'Nuova abilità "' + id + '". Cambia il gameplay: in multigiocatore tutti devono averla.'));
	plan.escribir();

	console.log(T('habilidad ' + id + ' (clase ' + info.id + ', tecla ' + tecla.toUpperCase() + (coste ? ', cuesta ' + coste + ' de energía' : '') + '): escritos en ' + modDir,
		'ability ' + id + ' (class ' + info.id + ', key ' + tecla.toUpperCase() + (coste ? ', costs ' + coste + ' energy' : '') + '): written to ' + modDir,
		'abilità ' + id + ' (classe ' + info.id + ', tasto ' + tecla.toUpperCase() + (coste ? ', costa ' + coste + ' di energia' : '') + '): scritti in ' + modDir));
	for (const [r, it] of plan.items) console.log('  ' + (it.nuevo ? T('nuevo     ', 'new       ', 'nuovo     ') : T('ampliado  ', 'extended  ', 'ampliato  ')) + path.relative(modDir, r));
	if (creaModJson) console.log(T('  (mod.json mínimo: cámbiale el nombre, los autores y la descripción)', '  (minimal mod.json: change its name, authors and description)', '  (mod.json minimo: cambia nome, autori e descrizione)'));
	for (const a of avisos) console.log(T('aviso: ', 'warning: ', 'avviso: ') + a);
	console.log(T('Falta:', 'To do:', 'Da fare:'));
	for (const f of falta) console.log('  - ' + f);
	console.log(T('  - pasar el validador: node validate-mod.js "' + modDir + '" --datos "' + dataDir + '"', '  - run the validator: node validate-mod.js "' + modDir + '" --datos "' + dataDir + '"', '  - passare il validatore: node validate-mod.js "' + modDir + '" --datos "' + dataDir + '"'));
});
