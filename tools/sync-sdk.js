// Copies to sdk\en and sdk\it what is not translated by hand: tools, examples (without their LEEME), the template mod.json and the function list, from sdk\es.
// The guides and the LEEME of each language are written by hand. Run it after changing something in sdk\es\herramientas, sdk\es\ejemplos or the function list:
//   node dll\tools\sync-sdk.js
// The comments of the tools and examples of en/it are translated by hand. A .js, .vs, .xml or .ini file is only copied again
// from es if its code changed (not counting comments or Mod_Log texts); in that case the notice says which files have to be translated again.
// Then run  node dll\tools\check-sdk.js  to see what is missing.
const fs = require('fs');
const path = require('path');
const { SDK, ES, NAMES, mapToolRel, mapExampleRel, walk, codeOf, makeFix } = require('./sdk-common');

const HEADER = {
	en: [
		"# Functions of the game's scripting language (for writing mods)",
		'#',
		'# List of the functions, types and constants the game gives to scripts (.vs), taken from the executable.',
		'# Each line: id | signature | origin. What matters when writing a script is the name and the signature:',
		'#   Name   | return_type(parameter types)',
		"# A 'Type::Method' is called as  object.Method(...)  ('u.Damage()').  'L' marks latent functions",
		"# (they can make the script wait, like Sleep). The ids are internal; you don't need to use them.",
		"# The functions of the Imperivm Resurrected API (Mod_...) aren't here: see %SCRIPTS%.",
		"# The ids and the internal names in the last column were deduced from the game's code; the signature is the part you can trust.",
	],
	it: [
		'# Funzioni del linguaggio di script del gioco (per scrivere mod)',
		'#',
		"# Elenco delle funzioni, dei tipi e delle costanti che il gioco mette a disposizione degli script (.vs), ricavato dall'eseguibile.",
		'# Ogni riga: id | firma | origine. Quello che conta quando scrivi uno script è il nome e la firma:',
		'#   Nome   | tipo_di_ritorno(tipi dei parametri)',
		"# Un 'Tipo::Metodo' si chiama come  oggetto.Metodo(...)  ('u.Damage()').  'L' indica le funzioni latenti",
		'# (possono far attendere lo script, come Sleep). Gli id sono interni e non serve usarli.',
		"# Le funzioni dell'API di Imperivm Resurrected (Mod_...) non sono qui: vedi %SCRIPTS%.",
		"# Gli id e i nomi interni dell'ultima colonna sono stati dedotti dal codice del gioco; la firma è l'unico dato affidabile.",
	],
};
const TRAILER = {
	en: [
		[/^# Resumen por registrador \(nativas \+ conversiones\):/, '# Summary by registration function (natives + conversions):'],
		[/^#  TOTAL (\d+) ; siguiente id libre \(inferido\) (\S+) ; tope (\S+) \(quedan (\d+)\)/, '#  TOTAL $1 ; next free id (inferred) $2 ; limit $3 ($4 left)'],
		[/^# Tipos registrados:/, '# Registered types:'],
		[/^# Firmas con tipos leidos de bytes: (\d+) ; sin tipos \(escaner no pudo\): (\d+)/, "# Signatures with types read from bytes: $1 ; without types (the scanner couldn't): $2"],
	],
	it: [
		[/^# Resumen por registrador \(nativas \+ conversiones\):/, '# Riepilogo per funzione di registrazione (native + conversioni):'],
		[/^#  TOTAL (\d+) ; siguiente id libre \(inferido\) (\S+) ; tope (\S+) \(quedan (\d+)\)/, '#  TOTALE $1 ; prossimo id libero (dedotto) $2 ; limite $3 (ne restano $4)'],
		[/^# Tipos registrados:/, '# Tipi registrati:'],
		[/^# Firmas con tipos leidos de bytes: (\d+) ; sin tipos \(escaner no pudo\): (\d+)/, '# Firme con tipi letti dai byte: $1 ; senza tipi (lo scanner non è riuscito): $2'],
	],
};

const removeEmptyDirs = d => {
	for (const e of fs.readdirSync(d, { withFileTypes: true })) if (e.isDirectory()) removeEmptyDirs(path.join(d, e.name));
	if (!fs.readdirSync(d).length) fs.rmdirSync(d);
};

const textExt = new Set(['.xml', '.vs', '.js', '.ini', '.json']);
const keepExt = new Set(['.xml', '.vs', '.js', '.ini']);
let retranslate = 0;

const syncTree = (lg, from, to, mapRel, skip) => {
	const fix = makeFix(lg);
	const wanted = new Set();
	for (const f of walk(from)) {
		if (skip(f)) continue;
		const rel = mapRel(path.relative(from, f).split(path.sep));
		const dest = path.join(to, ...rel);
		wanted.add(dest);
		fs.mkdirSync(path.dirname(dest), { recursive: true });
		const ext = path.extname(f).toLowerCase();
		if (!textExt.has(ext)) {
			fs.copyFileSync(f, dest);
			continue;
		}
		const fresh = fix(fs.readFileSync(f, 'utf8'), ext);
		if (keepExt.has(ext) && fs.existsSync(dest)) {
			if (codeOf(fresh, ext) === codeOf(fs.readFileSync(dest, 'utf8'), ext)) continue;
			console.log('  translate the comments again (' + lg + '): ' + path.relative(path.join(SDK, lg), dest));
			retranslate++;
		}
		else if (keepExt.has(ext) && /[a-záéíóúñ]{3,} [a-záéíóúñ]{3,}/i.test((fresh.match(/<!--[\s\S]*?-->|\/\/.*$|^\s*;.*$/gm) || []).join(' '))) {
			console.log('  new file with untranslated comments (' + lg + '): ' + path.relative(path.join(SDK, lg), dest));
			retranslate++;
		}
		fs.writeFileSync(dest, fresh);
	}
	return wanted;
};

for (const lg of ['en', 'it']) {
	const N = NAMES[lg], L = path.join(SDK, lg);

	const toolsOut = path.join(L, N.herramientas);
	const toolsWanted = syncTree(lg, path.join(ES, 'herramientas'), toolsOut, rel => mapToolRel(lg, rel), () => false);
	fs.writeFileSync(path.join(toolsOut, 'lib', 'lang.json'), JSON.stringify({ lang: lg }) + '\n');
	for (const f of walk(toolsOut)) if (!toolsWanted.has(f)) fs.unlinkSync(f);

	fs.copyFileSync(path.join(ES, 'plantilla-mod', 'mod.json'), path.join(L, N['plantilla-mod'], 'mod.json'));

	const exOut = path.join(L, N.ejemplos);
	const exWanted = syncTree(lg, path.join(ES, 'ejemplos'), exOut, rel => mapExampleRel(lg, rel), f => path.basename(f) === 'LEEME.md');
	for (const f of walk(exOut)) {
		if (exWanted.has(f) || path.basename(f) === N.leeme) continue;
		fs.unlinkSync(f);
	}
	removeEmptyDirs(exOut);

	const list = fs.readFileSync(path.join(ES, 'funciones-del-juego.txt'), 'utf8').split(/(\r?\n)/);
	let n = 0;
	for (let i = 0; i < list.length; i += 2) {
		const line = list[i];
		if (n < HEADER[lg].length && i === n * 2) list[i] = HEADER[lg][n++].replace('%SCRIPTS%', N.scripts);
		else if (line.startsWith('# ') || line.startsWith('#  ')) for (const [re, to] of TRAILER[lg]) if (re.test(line)) list[i] = line.replace(re, to);
	}
	fs.writeFileSync(path.join(L, N.funciones), list.join(''));
	console.log('synchronized:', lg);
}
if (retranslate) console.log('Warnings: ' + retranslate + ' file(s) with comments to translate.');
