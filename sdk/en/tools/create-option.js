// Creates a custom game option (a switch or a list of values in the lobby's RULES) for a mod (Imperivm Resurrected SDK).
//
//   node create-option.js <mod folder> <id> --name "Text" [--description "Text"] [--name-en "Text"] [--description-en "Text"]
//        [--name-it "Testo"] [--description-it "Testo"] [--values "Normal,Fast,Very fast"] [--values-en "Normal,Fast,Very fast"]
//        [--values-it "Normale,Veloce,Molto veloce"] [--default <true|false | number or text of the value>] [--script]
//
// Writes to the mod folder (guide: options.md):
//   options\<id>.json        the option: a switch, or a list if you pass --values (2 to 16, comma-separated)
//   data\mod\<id>.vs         with --script: a behavior skeleton with Mod_GetOption / Mod_GetOptionValue ready to fill in
//   mod.json                 only if the folder does not have one
// Warns how many of the 8 reserved bits it uses. It overwrites nothing.
const { fs, path, lower, readUtf8, fallo, parseArgs, soloConocidas, Plan, modJsonMinimo, ejecutar } = require('./lib/mod-common');
const { T } = require('./lib/lang');

const USO = T('uso: node create-option.js <carpeta del mod> <id> --nombre "Texto" [--descripcion "Texto"] [--nombre-en "Text"] [--descripcion-en "Text"]\n' +
	'       [--nombre-it "Testo"] [--descripcion-it "Testo"] [--valores "Normal,Rápida,Muy rápida"] [--valores-en "…"] [--valores-it "…"]\n' +
	'       [--defecto <true|false o el valor elegido por defecto>] [--script]',
	'usage: node create-option.js <mod folder> <id> --nombre "Text" [--descripcion "Text"] [--nombre-en "Text"] [--descripcion-en "Text"]\n' +
	'       [--nombre-it "Testo"] [--descripcion-it "Testo"] [--valores "Normal,Fast,Very fast"] [--valores-en "…"] [--valores-it "…"]\n' +
	'       [--defecto <true|false or the value chosen by default>] [--script]',
	'uso: node create-option.js <cartella della mod> <id> --nombre "Testo" [--descripcion "Testo"] [--nombre-en "Text"] [--descripcion-en "Text"]\n' +
	'       [--nombre-it "Testo"] [--descripcion-it "Testo"] [--valores "Normale,Veloce,Molto veloce"] [--valores-en "…"] [--valores-it "…"]\n' +
	'       [--defecto <true|false o il valore scelto di default>] [--script]');

// Bits used: a switch 1; a list, as many as its number of values needs (2: 1, 3-4: 2, 5-8: 3, 9-16: 4)
const bitsDe = n => { let w = 1; while ((1 << w) < n) w++; return w; };

// A short identifier from the text of a value: no accents, without what is in parentheses, with hyphens
function slug(t) {
	return t.replace(/\(.*$/, '').normalize('NFD').replace(/[̀-ͯ]/g, '').toLowerCase().replace(/[^a-z0-9]+/g, '-').replace(/^-+|-+$/g, '').slice(0, 32);
}
// One text per language: a plain text if there is only one, an object if there are several
function porIdioma(es, en, it) {
	const o = {};
	if (es !== undefined) o.spanish = es;
	if (en !== undefined) o.english = en;
	if (it !== undefined) o.italian = it;
	return Object.keys(o).length > 1 || (Object.keys(o).length === 1 && !o.spanish) ? o : es;
}
const lit = v => typeof v === 'object' && v !== null && !Array.isArray(v) ? '{ ' + Object.entries(v).map(([k, x]) => JSON.stringify(k) + ': ' + lit(x)).join(', ') + ' }' : JSON.stringify(v);

ejecutar(() => {
	const { pos, opt } = parseArgs(process.argv.slice(2), ['script']);
	soloConocidas(opt, ['nombre', 'descripcion', 'nombre-en', 'descripcion-en', 'nombre-it', 'descripcion-it', 'valores', 'valores-en', 'valores-it', 'defecto', 'script']);
	const [modDir, idArg] = pos;
	if (!modDir || !idArg) fallo(USO);
	if (pos.length > 2) fallo(T('sobran argumentos: ' + pos.slice(2).join(' '), 'extra arguments: ' + pos.slice(2).join(' '), 'argomenti in più: ' + pos.slice(2).join(' ')) + '\n' + USO);
	if (!opt.nombre || opt.nombre === true) fallo(T('falta --nombre "Texto" (lo que se ve en la sala)', 'missing --nombre "Text" (what is shown in the lobby)', 'manca --nombre "Testo" (quello che si vede nella stanza)') + '\n' + USO);
	if (!/^[A-Za-z0-9_-]{1,32}$/.test(idArg)) fallo(T('id no válido: "' + idArg + '" (letras sin acentos, números, _ y -; hasta 32). Es lo que usan los scripts.', 'invalid id: "' + idArg + '" (letters without accents, digits, _ and -; up to 32). It is what scripts use.', 'id non valido: "' + idArg + '" (lettere senza accenti, numeri, _ e -; fino a 32). È quello che usano gli script.'));
	const id = lower(idArg);
	for (const k of ['nombre', 'descripcion', 'nombre-en', 'descripcion-en', 'nombre-it', 'descripcion-it', 'valores', 'valores-en', 'valores-it', 'defecto']) {
		if (opt[k] !== undefined && (opt[k] === true || !String(opt[k]).trim())) fallo(T('--' + k + ' está vacío', '--' + k + ' is empty', '--' + k + ' è vuoto'));
	}

	// The options the mod already has
	const dir = path.join(modDir, 'options');
	let usados = 0;
	if (fs.existsSync(dir)) {
		for (const f of fs.readdirSync(dir)) {
			if (!/\.json$/i.test(f)) continue;
			let j;
			try { j = JSON.parse(readUtf8(path.join(dir, f))); } catch (e) { continue; }
			const otro = lower(j.id !== undefined ? j.id : path.basename(f, '.json'));
			if (otro === id) fallo(T('ya hay una opción con el id "' + id + '" en tu mod (options\\' + f + '): elige otro id.', 'there is already an option with the id "' + id + '" in your mod (options\\' + f + '): pick another id.', 'c\'è già un\'opzione con l\'id "' + id + '" nella tua mod (options\\' + f + '): scegli un altro id.'));
			const v = j.valores || j.values;
			usados += Array.isArray(v) ? bitsDe(v.length) : 1;
		}
	}

	// Switch or list
	const esLista = opt.valores !== undefined;
	let valores = null;
	if (!esLista) {
		for (const k of ['valores-en', 'valores-it']) if (opt[k] !== undefined) fallo(T('--' + k + ' sin --valores: ¿quieres una lista? Pon también --valores.', '--' + k + ' without --valores: do you want a list? Give --valores too.', '--' + k + ' senza --valores: vuoi un elenco? Metti anche --valores.'));
	}
	else {
		const partir = (k, s) => s.split(',').map(x => x.trim()).map((x, i) => { if (!x) fallo(T('--' + k + ': el valor ' + (i + 1) + ' está vacío (¿una coma de más?)', '--' + k + ': value ' + (i + 1) + ' is empty (an extra comma?)', '--' + k + ': il valore ' + (i + 1) + ' è vuoto (una virgola di troppo?)')); return x; });
		const es = partir('valores', opt.valores);
		if (es.length < 2) fallo(T('una lista necesita al menos 2 valores (has puesto ' + es.length + '). Para un sí/no no pongas --valores: sale un interruptor.', 'a list needs at least 2 values (you gave ' + es.length + '). For a yes/no, do not give --valores: you get a switch.', 'un elenco ha bisogno di almeno 2 valori (ne hai messi ' + es.length + '). Per un sì/no non mettere --valores: esce un interruttore.'));
		if (es.length > 16) fallo(T('una lista admite hasta 16 valores (has puesto ' + es.length + ').', 'a list takes up to 16 values (you gave ' + es.length + ').', 'un elenco ammette fino a 16 valori (ne hai messi ' + es.length + ').'));
		const en = opt['valores-en'] !== undefined ? partir('valores-en', opt['valores-en']) : null;
		const it = opt['valores-it'] !== undefined ? partir('valores-it', opt['valores-it']) : null;
		if (en && en.length !== es.length) fallo(T('--valores-en tiene ' + en.length + ' valores y --valores ' + es.length + ': tienen que ser los mismos, en el mismo orden.', '--valores-en has ' + en.length + ' values and --valores has ' + es.length + ': they must be the same, in the same order.', '--valores-en ha ' + en.length + ' valori e --valores ' + es.length + ': devono essere gli stessi, nello stesso ordine.'));
		if (it && it.length !== es.length) fallo(T('--valores-it tiene ' + it.length + ' valores y --valores ' + es.length + ': tienen que ser los mismos, en el mismo orden.', '--valores-it has ' + it.length + ' values and --valores has ' + es.length + ': they must be the same, in the same order.', '--valores-it ha ' + it.length + ' valori e --valores ' + es.length + ': devono essere gli stessi, nello stesso ordine.'));
		valores = es.map((t, i) => ({ id: slug(t), nombre: porIdioma(t, en ? en[i] : undefined, it ? it[i] : undefined), _texto: t }));
		valores.forEach((v, i) => { if (!v.id) fallo(T('el valor "' + v._texto + '" no tiene letras ni números: no puedo darle un id.', 'the value "' + v._texto + '" has no letters or digits: I cannot give it an id.', 'il valore "' + v._texto + '" non ha lettere né numeri: non posso dargli un id.')); });
		const vistos = new Map();
		valores.forEach((v, i) => {
			if (vistos.has(v.id)) fallo(T('los valores ' + (vistos.get(v.id) + 1) + ' y ' + (i + 1) + ' darían el mismo id ("' + v.id + '"): cámbiales el texto.', 'values ' + (vistos.get(v.id) + 1) + ' and ' + (i + 1) + ' would get the same id ("' + v.id + '"): change their text.', 'i valori ' + (vistos.get(v.id) + 1) + ' e ' + (i + 1) + ' avrebbero lo stesso id ("' + v.id + '"): cambia il loro testo.'));
			vistos.set(v.id, i);
		});
	}

	// The default value
	let defecto;
	if (!esLista) {
		if (opt.defecto === undefined) defecto = false;
		else if (/^(true|si|sí|yes|1)$/i.test(opt.defecto)) defecto = true;
		else if (/^(false|no|0)$/i.test(opt.defecto)) defecto = false;
		else fallo(T('--defecto de un interruptor es true o false (has puesto "' + opt.defecto + '")', '--defecto of a switch is true or false (you gave "' + opt.defecto + '")', '--defecto di un interruttore è true o false (hai messo "' + opt.defecto + '")'));
	}
	else if (opt.defecto === undefined) defecto = valores[0].id;
	else if (/^\d+$/.test(opt.defecto)) {
		const n = Number(opt.defecto);
		if (n >= valores.length) fallo(T('--defecto ' + n + ': la lista tiene ' + valores.length + ' valores, numerados desde 0 (el último es el ' + (valores.length - 1) + ').', '--defecto ' + n + ': the list has ' + valores.length + ' values, numbered from 0 (the last one is ' + (valores.length - 1) + ').', '--defecto ' + n + ': l\'elenco ha ' + valores.length + ' valori, numerati da 0 (l\'ultimo è il ' + (valores.length - 1) + ').'));
		defecto = valores[n].id;
	}
	else {
		const q = lower(opt.defecto);
		const v = valores.find(x => x.id === q || lower(x._texto) === q);
		if (!v) fallo(T('--defecto "' + opt.defecto + '" no es ninguno de los valores (' + valores.map(x => x.id).join(', ') + ').', '--defecto "' + opt.defecto + '" is none of the values (' + valores.map(x => x.id).join(', ') + ').', '--defecto "' + opt.defecto + '" non è nessuno dei valori (' + valores.map(x => x.id).join(', ') + ').'));
		defecto = v.id;
	}

	const plan = new Plan();
	const rJson = path.join(dir, id + '.json');
	const rScript = path.join(modDir, 'data', 'mod', id + '.vs');
	if (plan.existe(rJson)) fallo(T('ya existe ' + rJson + ': no lo piso.', rJson + ' already exists: not overwriting it.', rJson + ' esiste già: non lo sovrascrivo.'));
	if (opt.script && plan.existe(rScript)) fallo(T('ya existe ' + rScript + ': no lo piso. Quita --script o elige otro id.', rScript + ' already exists: not overwriting it. Remove --script or pick another id.', rScript + ' esiste già: non lo sovrascrivo. Togli --script o scegli un altro id.'));
	const bits = esLista ? bitsDe(valores.length) : 1;

	// options\<id>.json
	const filas = ['  "id": ' + JSON.stringify(id), '  "name": ' + lit(porIdioma(opt.nombre, opt['nombre-en'], opt['nombre-it']))];
	if (opt.descripcion !== undefined || opt['descripcion-en'] !== undefined || opt['descripcion-it'] !== undefined) {
		filas.push('  "description": ' + lit(porIdioma(opt.descripcion, opt['descripcion-en'], opt['descripcion-it'])));
	}
	if (esLista) filas.push('  "values": [\n' + valores.map(v => '    ' + lit({ id: v.id, name: v.nombre })).join(',\n') + '\n  ]');
	filas.push('  "default": ' + JSON.stringify(defecto));
	const texto = '{\n' + filas.join(',\n') + '\n}\n';
	JSON.parse(texto);  // just in case: what is written is JSON
	plan.poner(rJson, texto);

	// data\mod\<id>.vs
	if (opt.script) {
		const vs = ['// void, Obj This', ''];
		if (esLista) vs.push('int nValor;', '');
		vs.push('Sleep(rand(500) + 100);', '');
		if (!esLista) {
			vs.push(T('// Mod_GetOption("' + id + '") dice si el anfitrión marcó el interruptor en la sala. Da lo mismo en todos los PCs.', '// Mod_GetOption("' + id + '") says whether the host ticked the switch in the lobby. It gives the same on all PCs.', '// Mod_GetOption("' + id + '") dice se l\'host ha attivato l\'interruttore nella stanza. Dà lo stesso valore su tutti i PC.'),
				'if (Mod_GetOption("' + id + '")) {',
				T('\t// ESCRIBE AQUÍ lo que cambia la opción cuando está marcada. "This" es el objeto de la clase donde pongas este comportamiento.', '\t// WRITE HERE what the option changes when it is ticked. "This" is the object of the class where you put this behavior.', '\t// SCRIVI QUI cosa cambia l\'opzione quando è attivata. "This" è l\'oggetto della classe dove metti questo comportamento.'),
				'\tMod_Log("' + T('Opcion ', 'Option ', 'Opzione ') + id + T(': activada', ': enabled', ': attivata') + '");',
				'}');
		}
		else {
			vs.push(T('// Mod_GetOptionValue("' + id + '") da el número del valor elegido en la sala (0 es el primero). Da lo mismo en todos los PCs.', '// Mod_GetOptionValue("' + id + '") gives the number of the value chosen in the lobby (0 is the first). It gives the same on all PCs.', '// Mod_GetOptionValue("' + id + '") dà il numero del valore scelto nella stanza (0 è il primo). Dà lo stesso valore su tutti i PC.'),
				'nValor = Mod_GetOptionValue("' + id + '");');
			valores.forEach((v, i) => vs.push('// ' + i + ': ' + v._texto));
			vs.push('if (nValor == 1) {',
				T('\t// ESCRIBE AQUÍ lo que cambia el valor 1 (copia el bloque para los demás)', '\t// WRITE HERE what value 1 changes (copy the block for the others)', '\t// SCRIVI QUI cosa cambia il valore 1 (copia il blocco per gli altri)'),
				'\tMod_LogInt("' + T('Opcion ', 'Option ', 'Opzione ') + id + T(': valor', ': value', ': valore') + '", nValor);',
				'}');
		}
		vs.push('', T('// Un script de comportamiento no debe terminar', '// A behavior script must not end', '// Uno script di comportamento non deve terminare'), 'while(true) Sleep(50000);', '');
		plan.poner(rScript, vs.join('\n'));
	}

	const creaModJson = modJsonMinimo(plan, modDir, 'Gameplay & Balance', T('Opción de partida "' + id + '". Cambia la jugabilidad: en multijugador todos tienen que llevarlo.', 'Game option "' + id + '". It changes gameplay: in multiplayer everyone must have it.', 'Opzione di partita "' + id + '". Cambia il gameplay: in multigiocatore tutti devono averla.'));
	plan.escribir();

	console.log(T('opción ' + id + ' (' + (esLista ? 'lista de ' + valores.length + ' valores' : 'interruptor') + ', por defecto ' + JSON.stringify(defecto) + '): escritos en ' + modDir,
		'option ' + id + ' (' + (esLista ? 'list of ' + valores.length + ' values' : 'switch') + ', default ' + JSON.stringify(defecto) + '): written to ' + modDir,
		'opzione ' + id + ' (' + (esLista ? 'elenco di ' + valores.length + ' valori' : 'interruttore') + ', predefinito ' + JSON.stringify(defecto) + '): scritti in ' + modDir));
	for (const [r, it] of plan.items) console.log('  ' + (it.nuevo ? T('nuevo     ', 'new       ', 'nuovo     ') : T('ampliado  ', 'extended  ', 'ampliato  ')) + path.relative(modDir, r));
	if (creaModJson) console.log(T('  (mod.json mínimo: cámbiale el nombre, los autores y la descripción)', '  (minimal mod.json: change its name, authors and description)', '  (mod.json minimo: cambia nome, autori e descrizione)'));
	console.log(T('Gasta ' + bits + ' de los 8 bits reservados para opciones (este mod ya gastaba ' + usados + '; con esta, ' + (usados + bits) + ' de 8). Los 8 se reparten entre todos los mods activos.',
		'Uses ' + bits + ' of the 8 bits reserved for options (this mod already used ' + usados + '; with this one, ' + (usados + bits) + ' of 8). The 8 are shared among all active mods.',
		'Usa ' + bits + ' degli 8 bit riservati alle opzioni (questa mod ne usava già ' + usados + '; con questa, ' + (usados + bits) + ' su 8). Gli 8 si dividono tra tutte le mod attive.'));
	if (usados + bits > 8) console.log(T('aviso: este mod solo ya pasa de 8 bits: las últimas opciones (por orden alfabético de id) se ignorarán.', 'warning: this mod alone already exceeds 8 bits: the last options (in alphabetical order of id) will be ignored.', 'avviso: questa mod da sola supera già gli 8 bit: le ultime opzioni (in ordine alfabetico di id) verranno ignorate.'));
	if (idArg !== id) console.log(T('aviso: el id se guarda en minúsculas ("' + id + '"); los scripts no distinguen mayúsculas.', 'warning: the id is saved in lowercase ("' + id + '"); scripts are case-insensitive.', 'avviso: l\'id viene salvato in minuscolo ("' + id + '"); gli script non distinguono le maiuscole.'));
	if (!opt.descripcion) console.log(T('aviso: sin --descripcion la opción sale sin texto de ayuda en la sala.', 'warning: without --descripcion the option has no help text in the lobby.', 'avviso: senza --descripcion l\'opzione esce senza testo di aiuto nella stanza.'));
	if (!opt['nombre-en']) console.log(T('aviso: sin --nombre-en, todos los idiomas ven el mismo texto.', 'warning: without --nombre-en, all languages see the same text.', 'avviso: senza --nombre-en, tutte le lingue vedono lo stesso testo.'));
	console.log(T('Falta:', 'To do:', 'Da fare:'));
	if (opt.script) {
		console.log(T('  - completar data\\mod\\' + id + '.vs y engancharlo a una clase (o a un evento) con un parche, por ejemplo en patches\\data\\classes\\Hero.sc.xml:', '  - complete data\\mod\\' + id + '.vs and hook it to a class (or an event) with a patch, for example in patches\\data\\classes\\Hero.sc.xml:', '  - completare data\\mod\\' + id + '.vs e agganciarlo a una classe (o a un evento) con una patch, per esempio in patches\\data\\classes\\Hero.sc.xml:'));
		console.log('      <patch><add select="/class"><behavior script="data/mod/' + id + '.vs"/></add></patch>');
	}
	else console.log(T('  - un script que la lea: if (Mod_GetOption("' + id + '")) ' + (esLista ? 'o Mod_GetOptionValue("' + id + '") ' : '') + '(con --script te genero el esqueleto)', '  - a script that reads it: if (Mod_GetOption("' + id + '")) ' + (esLista ? 'or Mod_GetOptionValue("' + id + '") ' : '') + '(with --script I generate the skeleton for you)', '  - uno script che la legga: if (Mod_GetOption("' + id + '")) ' + (esLista ? 'o Mod_GetOptionValue("' + id + '") ' : '') + '(con --script ti genero lo scheletro)'));
	console.log(T('  - probarla en la sala: aparece en REGLAS; solo funciona en partidas con sala (no en aventuras ni campañas)', '  - test it in the lobby: it appears under RULES; it only works in games with a lobby (not in adventures or campaigns)', '  - provarla nella stanza: compare in REGOLE; funziona solo nelle partite con stanza (non nelle avventure né nelle campagne)'));
	console.log(T('  - pasar el validador: node validate-mod.js "' + modDir + '" --datos <DATA>', '  - run the validator: node validate-mod.js "' + modDir + '" --datos <DATA>', '  - passare il validatore: node validate-mod.js "' + modDir + '" --datos <DATA>'));
});
