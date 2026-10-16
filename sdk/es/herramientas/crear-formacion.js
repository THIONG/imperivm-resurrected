// Crea los archivos de una formación nueva para los héroes: la formación, sus dos botones y el parche del script (SDK de Imperivm Resurrected).
//
//   node crear-formacion.js <carpeta DATA del data.pak extraído> <carpeta del mod> <Nombre>
//        [--dano N] [--armadura N] [--defensa-pierce N] [--defensa-slash N] [--alcance N] [--nivel N]
//        [--tecla x] [--como <formación del juego>] [--clases "Unit=flancos,Horse=frente,..."]
//        [--ancho N] [--alto N] [--frente N] [--flanco-x N] [--flanco-y N] [--descripcion "Text"] [--boton <ruta>]
//
// Escribe en la carpeta del mod (guía: formaciones.md). Si ya hay parches de otra formación, añade sin pisar:
//   patches\data\formations.xml                     la formación (bonos, tamaño y dónde va cada clase de unidad)
//   patches\data\commands\Hero.xml                  sus dos botones: form-<nombre>-menu y form-<nombre>
//   patches\data\subai\hero_formation_verify.vs     parche del script que deja elegir la formación
//   mod.json                                        solo si la carpeta no tiene
// --como copia de una formación del juego (Front, Line, Block, Cavalry core, Horse Wings, Ranged Wings) el tamaño, la separación y la lista de clases,
// y el dibujo del botón. Sin --como se parte de Front. Lo que pongas con --ancho, --alto, --frente, --flanco-x, --flanco-y y --clases lo cambia.
const { fs, path, lower, ci, readLatin1, readUtf8, stripComments, xmlEsc, attrs, fallo, parseArgs, entero, soloConocidas, carpetaData, Plan, antesDelCierre,
	leerClases, leerOrdenes, chocaTecla, modJsonMinimo, ejecutar } = require('./lib/mod-comun');
const { T } = require('./lib/lang');

const USO = T('uso: node crear-formacion.js <DATA> <carpeta del mod> <Nombre> [--dano N] [--armadura N] [--defensa-pierce N] [--defensa-slash N] [--alcance N]\n' +
	'       [--nivel N] [--tecla x] [--como <formación del juego>] [--clases "Unit=flancos,Horse=frente"] [--ancho N] [--alto N] [--frente N]\n' +
	'       [--flanco-x N] [--flanco-y N] [--descripcion "Text"] [--boton <ruta>]',
	'usage: node crear-formacion.js <DATA> <mod folder> <Name> [--dano N] [--armadura N] [--defensa-pierce N] [--defensa-slash N] [--alcance N]\n' +
	'       [--nivel N] [--tecla x] [--como <game formation>] [--clases "Unit=flancos,Horse=frente"] [--ancho N] [--alto N] [--frente N]\n' +
	'       [--flanco-x N] [--flanco-y N] [--descripcion "Text"] [--boton <path>]',
	'uso: node crear-formacion.js <DATA> <cartella della mod> <Nome> [--dano N] [--armadura N] [--defensa-pierce N] [--defensa-slash N] [--alcance N]\n' +
	'       [--nivel N] [--tecla x] [--como <formazione del gioco>] [--clases "Unit=flancos,Horse=frente"] [--ancho N] [--alto N] [--frente N]\n' +
	'       [--flanco-x N] [--flanco-y N] [--descripcion "Text"] [--boton <percorso>]');
const SITIOS = { frente: 'FrontLine', delante: 'FrontLine', frontline: 'FrontLine', flancos: 'Wings', wings: 'Wings', centro: 'CentralBlock', centralblock: 'CentralBlock', front: 'FrontLine', fronte: 'FrontLine', ali: 'Wings', center: 'CentralBlock', centre: 'CentralBlock' };
const BONOS = [['dano', 'BonusDamage'], ['armadura', 'BonusArmor'], ['defensa-pierce', 'BonusDefencePierce'], ['defensa-slash', 'BonusDefenceSlash'], ['alcance', 'BonusRange'], ['nivel', 'BonusLevel']];

ejecutar(() => {
	const { pos, opt } = parseArgs(process.argv.slice(2));
	soloConocidas(opt, [...BONOS.map(b => b[0]), 'tecla', 'como', 'clases', 'ancho', 'alto', 'frente', 'flanco-x', 'flanco-y', 'descripcion', 'boton']);
	const [dataDir, modDir, nombre] = pos;
	if (!dataDir || !modDir || !nombre) fallo(USO);
	if (pos.length > 3) fallo(T('sobran argumentos: ' + pos.slice(3).join(' ') + ' (si el nombre lleva espacios, ponlo entre comillas, y sin espacios es mejor)', 'extra arguments: ' + pos.slice(3).join(' ') + ' (if the name has spaces, put it in quotes; no spaces is better)', 'argomenti in più: ' + pos.slice(3).join(' ') + ' (se il nome ha spazi, mettilo tra virgolette; meglio senza spazi)') + '\n' + USO);
	carpetaData(dataDir);
	if (!/^[A-Za-z][A-Za-z0-9]{0,23}$/.test(nombre)) fallo(T('nombre no válido: "' + nombre + '" (letras y números, sin espacios; empieza por letra; hasta 24). Es el Name de la formación y sale en los nombres de las órdenes.', 'invalid name: "' + nombre + '" (letters and digits, no spaces; must start with a letter; up to 24). It is the formation\'s Name and appears in the command names.', 'nome non valido: "' + nombre + '" (lettere e numeri, senza spazi; inizia con una lettera; fino a 24). È il Name della formazione e compare nei nomi dei comandi.'));
	const bonos = {};
	for (const [k, attr] of BONOS) { const v = entero(opt, k, -1000, 1000); if (v !== undefined) bonos[attr] = v; }
	const medidas = {};
	for (const [k, attr, min, max] of [['ancho', 'Width', 1, 20], ['alto', 'Height', 1, 20], ['frente', 'OffsetFrontLineByY', -500, 500], ['flanco-x', 'OffsetWingsByX', -500, 500], ['flanco-y', 'OffsetWingsByY', -500, 500]]) {
		const v = entero(opt, k, min, max);
		if (v !== undefined) medidas[attr] = v;
	}
	if (opt.tecla !== undefined && !/^[A-Za-z]$/.test(opt.tecla)) fallo(T('--tecla tiene que ser una sola letra (has puesto "' + opt.tecla + '")', '--tecla must be a single letter (you gave "' + opt.tecla + '")', '--tecla deve essere una sola lettera (hai messo "' + opt.tecla + '")'));
	for (const k of ['descripcion', 'boton', 'como', 'clases']) if (opt[k] !== undefined && !String(opt[k]).trim()) fallo(T('--' + k + ' está vacío', '--' + k + ' is empty', '--' + k + ' è vuoto'));
	const slug = lower(nombre);

	// La formación del juego de la que se copia
	const fdir = ci(dataDir, 'formations.xml');
	if (!fdir) fallo(T('la carpeta DATA no tiene formations.xml', 'the DATA folder has no formations.xml', 'la cartella DATA non ha formations.xml'));
	const fxml = stripComments(readLatin1(path.join(dataDir, fdir)));
	const juego = [...fxml.matchAll(/<FormationClass\b([^>]*)>([\s\S]*?)<\/FormationClass>/g)].map(m => ({
		a: attrs(m[1]), clases: [...m[2].matchAll(/<Class\b([^>]*?)\/?>/g)].map(c => attrs(c[1])) }));
	const base = juego.find(f => lower(f.a.name) === lower(opt.como || 'Front'));
	if (!base) fallo(T('no hay ninguna formación "' + opt.como + '" en el juego. Hay: ' + juego.map(f => f.a.name).join(', ') + '.', 'there is no formation "' + opt.como + '" in the game. Available: ' + juego.map(f => f.a.name).join(', ') + '.', 'non c\'è nessuna formazione "' + opt.como + '" nel gioco. Ci sono: ' + juego.map(f => f.a.name).join(', ') + '.'));
	if (juego.some(f => lower(f.a.name) === slug)) fallo(T('ya hay una formación "' + nombre + '" en el juego: elige otro nombre.', 'there is already a formation "' + nombre + '" in the game: pick another name.', 'c\'è già una formazione "' + nombre + '" nel gioco: scegli un altro nome.'));

	// Dónde va cada clase
	const clases = leerClases(dataDir, modDir);
	let lista = base.clases.map(c => {
		const sitio = ['FrontLine', 'Wings', 'CentralBlock'].find(s => c[lower(s)] === '1');
		return { name: c.name, sitio };
	});
	if (opt.clases) {
		lista = [];
		for (const par of opt.clases.split(',')) {
			const m = par.trim().match(/^([A-Za-z][A-Za-z0-9_]*)\s*=\s*([A-Za-z]+)$/);
			if (!m) fallo(T('--clases: "' + par.trim() + '" no vale. Se escribe Clase=sitio, con sitio frente, flancos o centro (p. ej. "Unit=flancos,Horse=frente").', '--clases: "' + par.trim() + '" is not valid. Write Class=place, with place front, wings or center (e.g. "Unit=wings,Horse=front").', '--clases: "' + par.trim() + '" non vale. Si scrive Classe=posto, con posto fronte, ali o centro (es. "Unit=ali,Horse=fronte").'));
			const sitio = SITIOS[lower(m[2])];
			if (!sitio) fallo(T('--clases: el sitio "' + m[2] + '" no existe (frente, flancos o centro)', '--clases: the place "' + m[2] + '" does not exist (front, wings or center)', '--clases: il posto "' + m[2] + '" non esiste (fronte, ali o centro)'));
			if (!clases.get(lower(m[1]))) fallo(T('--clases: la clase "' + m[1] + '" no existe en el juego ni en el mod', '--clases: the class "' + m[1] + '" does not exist in the game or in the mod', '--clases: la classe "' + m[1] + '" non esiste nel gioco né nella mod'));
			if (lista.some(c => lower(c.name) === lower(m[1]))) fallo(T('--clases: la clase "' + m[1] + '" está repetida', '--clases: the class "' + m[1] + '" is repeated', '--clases: la classe "' + m[1] + '" è ripetuta'));
			lista.push({ name: clases.get(lower(m[1])).id, sitio });
		}
	}
	if (!lista.length) fallo(T('la formación no tiene ninguna clase de unidad: pon --clases o elige otra base con --como', 'the formation has no unit class: give --clases or pick another base with --como', 'la formazione non ha nessuna classe di unità: metti --clases o scegli un\'altra base con --como'));

	// Nombres repetidos en el mod (formación y órdenes) y en el juego (órdenes)
	const ordenes = leerOrdenes(dataDir, modDir);
	for (const n of ['form-' + slug + '-menu', 'form-' + slug]) {
		const r = ordenes.find(o => lower(o.name) === n);
		if (r) fallo(T('ya hay una orden "' + r.name + '" ' + (r.mod ? 'en tu mod (' + r.file + ')' : 'en el juego (' + r.file + ')') + ': esa formación ya existe o su nombre choca. Elige otro nombre.',
			'there is already a command "' + r.name + '" ' + (r.mod ? 'in your mod (' + r.file + ')' : 'in the game (' + r.file + ')') + ': that formation already exists or its name clashes. Pick another name.',
			'c\'è già un comando "' + r.name + '" ' + (r.mod ? 'nella tua mod (' + r.file + ')' : 'nel gioco (' + r.file + ')') + ': quella formazione esiste già o il nome si scontra. Scegli un altro nome.'));
	}
	const plan = new Plan();
	const avisos = [], falta = [];
	const rFormaciones = path.join(modDir, 'patches', 'data', 'formations.xml');
	const rHero = path.join(modDir, 'patches', 'data', 'commands', 'Hero.xml');
	const rScript = path.join(modDir, 'patches', 'data', 'subai', 'hero_formation_verify.vs');
	for (const r of [rFormaciones, rHero, rScript]) {
		const dir = path.dirname(r);
		if (fs.existsSync(dir) && !fs.readdirSync(dir).some(f => f === path.basename(r)) && ci(dir, path.basename(r))) fallo(T('ya existe ' + path.join(dir, ci(dir, path.basename(r))) + ' con otras mayúsculas: renómbralo a ' + path.basename(r) + '.', path.join(dir, ci(dir, path.basename(r))) + ' already exists with different capitalization: rename it to ' + path.basename(r) + '.', path.join(dir, ci(dir, path.basename(r))) + ' esiste già con maiuscole diverse: rinominalo in ' + path.basename(r) + '.'));
	}
	const prevF = plan.texto(rFormaciones);
	if (prevF !== null && new RegExp('Name\\s*=\\s*"' + nombre + '"', 'i').test(prevF)) fallo(T(rFormaciones + ' ya tiene una formación "' + nombre + '".', rFormaciones + ' already has a formation "' + nombre + '".', rFormaciones + ' ha già una formazione "' + nombre + '".'));
	const prevS = plan.texto(rScript);
	if (prevS !== null && prevS.includes('form-' + slug + '-menu')) fallo(T(rScript + ' ya trata la formación "' + nombre + '".', rScript + ' already handles the formation "' + nombre + '".', rScript + ' gestisce già la formazione "' + nombre + '".'));

	// La tecla: libre para el héroe (que hereda las de Unit y Military)
	let tecla = opt.tecla ? lower(opt.tecla) : null;
	const libre = t => !chocaTecla(ordenes, clases, t, 'Hero').length;
	if (!tecla) {
		tecla = 'ijkqz'.split('').concat('abcdefghijklmnopqrstuvwxyz'.split('')).find(libre);
		if (!tecla) fallo(T('no queda ninguna letra libre para el héroe (el juego y tu mod ya usan las 26). Pon --tecla con la letra que prefieras: se genera igual y se avisa del choque.', 'no free letter left for the hero (the game and your mod already use all 26). Give --tecla with the letter you prefer: it is generated anyway and the clash is reported.', 'non resta nessuna lettera libera per l\'eroe (il gioco e la tua mod le usano già tutte e 26). Metti --tecla con la lettera che preferisci: viene generato comunque e si avvisa dello scontro.'));
		avisos.push(T('sin --tecla: he elegido "' + tecla + '", que está libre para el héroe.', 'no --tecla given: I picked "' + tecla + '", which is free for the hero.', 'senza --tecla: ho scelto "' + tecla + '", che è libero per l\'eroe.'));
	}
	else if (!libre(tecla)) {
		const libres = 'abcdefghijklmnopqrstuvwxyz'.split('').filter(libre);
		const quien = chocaTecla(ordenes, clases, tecla, 'Hero').map(o => o.name + ' (' + o.file + ')').join(', ');
		avisos.push(T('la tecla "' + tecla + '" ya la usa el héroe: ' + quien + '. Se genera igual; teclas libres: ' + (libres.join(' ') || 'ninguna') + '.', 'the key "' + tecla + '" is already used by the hero: ' + quien + '. It is generated anyway; free keys: ' + (libres.join(' ') || 'none') + '.', 'il tasto "' + tecla + '" è già usato dall\'eroe: ' + quien + '. Viene generato comunque; tasti liberi: ' + (libres.join(' ') || 'nessuno') + '.'));
	}

	// Botón: el de la formación base, o el que pidas
	const botones = new Set(ordenes.filter(o => o.button).map(o => lower(o.button)));
	const deBase = ordenes.find(o => /^form-/.test(o.name) && o.param && lower(o.param) === lower(base.a.name) && o.button);
	const boton = opt.boton || (deBase && deBase.button) || 'States/front form.bmp';
	if (opt.boton) {
		if (!botones.has(lower(boton)) && !fs.existsSync(path.join(modDir, 'gameres', 'cmdbar', ...boton.split('/')))) avisos.push(T('el botón "' + boton + '" no lo usa ninguna orden del juego ni está en gameres\\cmdbar de tu mod: si no existe, saldrá sin dibujo.', 'the button "' + boton + '" is not used by any game command nor is it in gameres\\cmdbar of your mod: if it does not exist, it will have no picture.', 'il pulsante "' + boton + '" non è usato da nessun comando del gioco e non è in gameres\\cmdbar della tua mod: se non esiste, uscirà senza immagine.'));
	}
	else falta.push(T('un dibujo propio para los botones (ahora usan "' + boton + '"): ver imagenes.md y pasa --boton States/<tu botón>.bmp', 'your own picture for the buttons (now they use "' + boton + '"): see imagenes.md and pass --boton States/<your button>.bmp', 'un\'immagine tua per i pulsanti (ora usano "' + boton + '"): vedi imagenes.md e passa --boton States/<tuo pulsante>.bmp'));

	// 1. formations.xml
	const med = k => medidas[k] !== undefined ? medidas[k] : base.a[lower(k)];
	const f = ['    <FormationClass Name="' + xmlEsc(nombre) + '"', '        Description="' + xmlEsc(opt.descripcion || 'The warriors form a ' + nombre + ' formation.') + '"'];
	for (const k of ['Width', 'Height']) if (med(k) !== undefined) f.push('        ' + k + '="' + med(k) + '"');
	for (const [, attr] of BONOS) if (bonos[attr] !== undefined) f.push('        ' + attr + '="' + bonos[attr] + '"');
	for (const k of ['OffsetFrontLineByY', 'OffsetWingsByX', 'OffsetWingsByY']) f.push('        ' + k + '="' + (med(k) !== undefined ? med(k) : 0) + '"');
	f[f.length - 1] += '>';
	f.push('');
	for (const c of lista) f.push('        <Class Name="' + xmlEsc(c.name) + '"' + (c.sitio ? ' ' + c.sitio + '="1"' : '') + '/>');
	f.push('    </FormationClass>');
	if (!Object.keys(bonos).length) avisos.push(T('la formación no da ningún bono (ni --dano, --armadura, --defensa-pierce, --defensa-slash, --alcance ni --nivel): solo cambia la colocación.', 'the formation gives no bonus (no --dano, --armadura, --defensa-pierce, --defensa-slash, --alcance or --nivel): it only changes the placement.', 'la formazione non dà nessun bonus (né --dano, --armadura, --defensa-pierce, --defensa-slash, --alcance né --nivel): cambia solo la disposizione.'));
	const bloqueF = '  ' + T('<!-- Una formación nueva. Width/Height: tamaño de la rejilla de la primera línea. Bonus*: bonos mientras la tropa está parada (stand ground).', '<!-- A new formation. Width/Height: size of the grid of the first line. Bonus*: bonuses while the troop is standing still (stand ground).', '<!-- Una formazione nuova. Width/Height: dimensione della griglia della prima linea. Bonus*: bonus mentre la truppa è ferma (stand ground).') + '\n' +
		'       ' + T('Description no se muestra en el botón: el juego pone su propio texto.', 'Description is not shown on the button: the game uses its own text.', 'Description non compare sul pulsante: il gioco usa un suo testo.') + '\n' +
		'       ' + T('Cada <Class> dice dónde van las unidades de esa clase: FrontLine (delante), Wings (a los lados) o CentralBlock (en el centro) -->', 'Each <Class> says where the units of that class go: FrontLine (in front), Wings (on the sides) or CentralBlock (in the center) -->', 'Ogni <Class> dice dove vanno le unità di quella classe: FrontLine (davanti), Wings (ai lati) o CentralBlock (al centro) -->') + '\n' +
		'  <add select="/Formations">\n' + f.join('\n') + '\n  </add>\n';
	plan.poner(rFormaciones, prevF === null ? '<patch>\n' + bloqueF + '</patch>\n' : antesDelCierre(prevF, '</patch>', bloqueF, rFormaciones));

	// 2. Hero.xml: los dos botones
	const bloqueH = '  ' + T('<!-- Los dos botones de la formación ' + nombre + ': el que se ve cuando es la actual ("-menu", abre la lista) y el que la elige.', '<!-- The two buttons of the formation ' + nombre + ': the one shown when it is the current one ("-menu", opens the list) and the one that selects it.', '<!-- I due pulsanti della formazione ' + nombre + ': quello che si vede quando è quella attuale ("-menu", apre l\'elenco) e quello che la sceglie.') + '\n' +
		(opt.boton ? '       ' + T('Usan el dibujo ' + boton + '. -->', 'They use the picture ' + boton + '. -->', 'Usano l\'immagine ' + boton + '. -->') + '\n' : '       ' + T('Aquí usan el dibujo de otra formación; con uno propio, ver la guía de imágenes. -->', 'Here they use the picture of another formation; for your own, see the images guide. -->', 'Qui usano l\'immagine di un\'altra formazione; per una tua, vedi la guida alle immagini. -->') + '\n') +
		'  <add select="/commands">\n' +
		'    <cmd name="form-' + slug + '-menu" priority="1"\n        method="move"\n        button="' + xmlEsc(boton) + '"\n        key="f"\n        rollover="Select Formation"\n' +
		'        param="' + xmlEsc(nombre) + '"\n        groupverifier="data/subai/hero_formation_menu_verify.vs"\n        menu="1">\n      <src obj="Hero"/>\n    </cmd>\n\n' +
		'    <cmd name="form-' + slug + '" priority="1"\n        method="formation"\n        button="' + xmlEsc(boton) + '"\n        immediate="yes"\n        key="' + tecla + '"\n' +
		'        rollover="Select ' + xmlEsc(nombre) + ' formation"\n        param="' + xmlEsc(nombre) + '"\n        groupverifier="data/subai/hero_formation_verify.vs"\n        menu="1">\n      <src obj="Hero"/>\n    </cmd>\n' +
		'  </add>\n';
	const prevH = plan.texto(rHero);
	plan.poner(rHero, prevH === null ? '<patch>\n' + bloqueH + '</patch>\n' : antesDelCierre(prevH, '</patch>', bloqueH, rHero));

	// 3. El script: el juego solo deja elegir formación cuando está abierto uno de sus menús, nombrados en una lista
	const ancla = 'if (cmd_name != "form-front-menu")';
	const vsDir = ci(dataDir, 'subai');
	const vsGame = vsDir && ci(path.join(dataDir, vsDir), 'hero_formation_verify.vs');
	if (!vsGame) fallo(T('la carpeta DATA no tiene subai\\hero_formation_verify.vs', 'the DATA folder has no subai\\hero_formation_verify.vs', 'la cartella DATA non ha subai\\hero_formation_verify.vs'));
	if (!readLatin1(path.join(dataDir, vsDir, vsGame)).includes(ancla)) fallo(T('el script del juego subai\\hero_formation_verify.vs ya no tiene la línea ' + ancla + ': esta herramienta no sabe parchearlo.', 'the game script subai\\hero_formation_verify.vs no longer has the line ' + ancla + ': this tool does not know how to patch it.', 'lo script del gioco subai\\hero_formation_verify.vs non ha più la riga ' + ancla + ': questo strumento non sa come patcharlo.'));
	const bloqueS = '@replace ' + ancla + '\n' + ancla + '\n\tif (cmd_name != "form-' + slug + '-menu")\n@end\n';
	if (prevS === null) {
		plan.poner(rScript, T('// El script del juego solo deja elegir formación cuando está abierto uno de sus cuatro menús ("form-xxx-menu"):\n// se añade el de cada formación nueva a esa lista. Cada bloque @replace se suma a los demás.\n',
			'// The game script only lets you choose a formation when one of its four menus ("form-xxx-menu") is open:\n// the menu of each new formation is added to that list. Each @replace block adds to the others.\n',
			'// Lo script del gioco permette di scegliere la formazione solo quando è aperto uno dei suoi quattro menu ("form-xxx-menu"):\n// si aggiunge a quell\'elenco il menu di ogni formazione nuova. Ogni blocco @replace si somma agli altri.\n') + bloqueS);
	}
	else {
		let t = prevS.replace(/\s*$/, '\n');
		if (!/@(end|fin)\s*$/.test(t)) avisos.push(T('el parche del script que ya había no termina en @end: lo he cerrado con otro @end al añadir tu formación.', 'the script patch that was already there does not end in @end: I closed it with another @end when adding your formation.', 'la patch dello script che c\'era già non finisce con @end: l\'ho chiusa con un altro @end aggiungendo la tua formazione.'));
		plan.poner(rScript, t + '\n' + bloqueS);
	}

	const creaModJson = modJsonMinimo(plan, modDir, 'Gameplay & Balance', T('Formación nueva "' + nombre + '" para los héroes. Cambia la jugabilidad: en multijugador todos tienen que llevarlo.', 'New formation "' + nombre + '" for heroes. It changes gameplay: in multiplayer everyone must have it.', 'Nuova formazione "' + nombre + '" per gli eroi. Cambia il gameplay: in multigiocatore tutti devono averla.'));
	plan.escribir();

	console.log(T('formación ' + nombre + ' (colocación de ' + base.a.name + (opt.clases ? ' cambiada por --clases' : '') + ', botón de elegir: tecla ' + tecla.toUpperCase() + '): escritos en ' + modDir,
		'formation ' + nombre + ' (placement of ' + base.a.name + (opt.clases ? ' changed by --clases' : '') + ', select button: key ' + tecla.toUpperCase() + '): written to ' + modDir,
		'formazione ' + nombre + ' (disposizione di ' + base.a.name + (opt.clases ? ' cambiata da --clases' : '') + ', pulsante di scelta: tasto ' + tecla.toUpperCase() + '): scritti in ' + modDir));
	for (const [r, it] of plan.items) console.log('  ' + (it.nuevo ? T('nuevo     ', 'new       ', 'nuovo     ') : T('ampliado  ', 'extended  ', 'ampliato  ')) + path.relative(modDir, r));
	if (creaModJson) console.log(T('  (mod.json mínimo: cámbiale el nombre, los autores y la descripción)', '  (minimal mod.json: change its name, authors and description)', '  (mod.json minimo: cambia nome, autori e descrizione)'));
	for (const a of avisos) console.log(T('aviso: ', 'warning: ', 'avviso: ') + a);
	console.log(T('Falta:', 'To do:', 'Da fare:'));
	for (const x of falta) console.log('  - ' + x);
	console.log(T('  - probar en partida: abre el menú de formaciones de un héroe con tropas (los bonos solo valen con la tropa parada)', '  - test in game: open the formations menu of a hero with troops (bonuses only apply while the troop is standing still)', '  - provare in partita: apri il menu delle formazioni di un eroe con truppe (i bonus valgono solo con la truppa ferma)'));
	console.log(T('  - pasar el validador: node validar-mod.js "' + modDir + '" --datos "' + dataDir + '"', '  - run the validator: node validar-mod.js "' + modDir + '" --datos "' + dataDir + '"', '  - passare il validatore: node validar-mod.js "' + modDir + '" --datos "' + dataDir + '"'));
});
