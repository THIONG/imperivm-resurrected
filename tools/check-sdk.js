// Checks sdk\en and sdk\it: same files as sdk\es (with the names of each language), guide links that exist and text
// that has been left in Spanish. Usage:  node dll\tools\check-sdk.js
const fs = require('fs');
const path = require('path');

const { SDK, ES, NAMES, EXAMPLES, mapToolRel, mapExampleRel, walk, codeOf, makeFix } = require('./sdk-common');

const OPCIONES = eval('(' + fs.readFileSync(path.join(ES, 'herramientas', 'lib', 'lang.js'), 'utf8').match(/const OPCIONES = (\{[\s\S]*?\n\});/)[1] + ')');
const opcionRe = new RegExp('(?<![\\w-])--(' + Object.keys(OPCIONES).sort((a, b) => b.length - a.length).join('|') + ')(?![\\w-])', 'g');
const SPANISH_WORDS = new Set(['el', 'los', 'las', 'para', 'que', 'sus', 'hay', 'está', 'están', 'cuando', 'donde', 'también', 'pero', 'como', 'desde', 'hasta', 'sin', 'porque', 'puede', 'pueden', 'tiene', 'tienen', 'según', 'cada', 'todos', 'todas', 'una', 'uno', 'del', 'esta', 'este', 'estos', 'esto', 'ese', 'eso', 'debe', 'después', 'antes', 'aquí', 'nunca', 'siempre', 'también', 'solo', 'más', 'menos', 'muy', 'otro', 'otra', 'otros', 'otras', 'mismo', 'misma', 'dentro', 'fuera', 'ejemplo', 'archivo', 'carpeta', 'juego', 'partida', 'jugador']);
const ITALIAN_WORDS = new Set(['una', 'uno', 'del', 'come', 'sono', 'solo', 'cada', 'sin']);
const SPANISH_ONLY_CHARS = /[ñ¿¡áíóú]/i;

const isSpanish = (line, lg) => {
	const clean = line.replace(/`[^`]*`/g, ' ').replace(/https?:\/\/\S+/g, ' ').replace(/\([^)]*\.md[^)]*\)/g, ' ');
	if (SPANISH_ONLY_CHARS.test(clean) && lg === 'en') return true;
	if (/[ñ¿¡]/.test(clean)) return true;
	const words = clean.toLowerCase().match(/[a-záéíóúñ]+/g) || [];
	let hits = 0;
	for (const w of words) if (SPANISH_WORDS.has(w) && !(lg === 'it' && ITALIAN_WORDS.has(w))) hits++;
	return hits >= 3;
};

const problems = [];
const add = (lg, msg) => problems.push(lg + ': ' + msg);

const esExamples = fs.readdirSync(path.join(ES, 'ejemplos'));
const checkCode = (lg, fromDir, toDir, mapRel) => {
	const fix = makeFix(lg);
	for (const f of walk(fromDir)) {
		const ext = path.extname(f).toLowerCase();
		if (!['.xml', '.vs', '.js', '.ini'].includes(ext) || path.basename(f) === 'LEEME.md') continue;
		const rel = mapRel(path.relative(fromDir, f).split(path.sep));
		const dest = path.join(toDir, ...rel);
		if (!fs.existsSync(dest)) { add(lg, 'missing ' + path.relative(path.join(SDK, lg), dest)); continue; }
		if (codeOf(fix(fs.readFileSync(f, 'utf8'), ext), ext) !== codeOf(fs.readFileSync(dest, 'utf8'), ext)) add(lg, 'the code of ' + path.relative(path.join(SDK, lg), dest) + ' does not match the one in es (run sync-sdk.js)');
	}
};
for (const lg of ['en', 'it']) {
	const N = NAMES[lg], L = path.join(SDK, lg);
	checkCode(lg, path.join(ES, 'herramientas'), path.join(L, N.herramientas), rel => mapToolRel(lg, rel));
	checkCode(lg, path.join(ES, 'ejemplos'), path.join(L, N.ejemplos), rel => mapExampleRel(lg, rel));
	for (const name of esExamples) {
		const dir = path.join(L, N.ejemplos, EXAMPLES[name][lg]);
		if (!fs.existsSync(dir)) add(lg, 'missing example ' + EXAMPLES[name][lg]);
		else if (fs.existsSync(path.join(ES, 'ejemplos', name, 'LEEME.md')) && !fs.existsSync(path.join(dir, N.leeme))) add(lg, 'the example ' + EXAMPLES[name][lg] + ' does not have ' + N.leeme);
	}
	for (const f of walk(L).filter(x => /\.(md|txt)$/.test(x))) {
		const rel = path.relative(L, f);
		const lines = fs.readFileSync(f, 'utf8').split(/\r?\n/);
		let inCode = false;
		lines.forEach((line, i) => {
			if (/^\s*```/.test(line)) inCode = !inCode;
			if (rel === N.funciones && !line.startsWith('#')) return;
			if (/"spanish"|--(?:nombre|name|nome) |Mod_Log\(|\[Español\]|result="|Script: Golpe|"Golpe crítico"|"Muy rápida"|"rapida"/.test(line)) return;
			for (const m of line.matchAll(opcionRe)) if (OPCIONES[m[1]][lg === 'en' ? 0 : 1] !== m[1] && !/`--nombre`.*`--nome`|`--nome`.*`--nombre`/.test(line)) add(lg, rel + ':' + (i + 1) + ' untranslated option: ' + m[0]);
			if (isSpanish(line, lg)) add(lg, rel + ':' + (i + 1) + ' text in Spanish: ' + line.trim().slice(0, 110));
		});
		for (const m of fs.readFileSync(f, 'utf8').matchAll(/\]\((?!https?:|#|mailto:)([^)\s#]+)(#[^)]*)?\)/g)) {
			const target = path.resolve(path.dirname(f), decodeURI(m[1]));
			if (!fs.existsSync(target)) add(lg, rel + ': broken link ' + m[1]);
		}
		for (const name of Object.keys(EXAMPLES)) if (fs.readFileSync(f, 'utf8').includes(name) && lg !== 'es') add(lg, rel + ': untranslated example name ' + name);
	}
	for (const f of walk(L).filter(x => /\.(xml|vs|ini|js)$/.test(x))) {
		const rel = path.relative(L, f);
		if (/spanish\.loc\.xml$|french\.loc\.xml$/.test(f)) continue;
		const text = fs.readFileSync(f, 'utf8');
		const comments = (text.match(f.endsWith('.xml') ? /<!--[\s\S]*?-->/g : f.endsWith('.ini') ? /^\s*;.*$/gm : /^\s*\/\/.*$|\/\*[\s\S]*?\*\//gm) || []);
		for (const c of comments) for (const line of c.split(/\r?\n/)) if (isSpanish(line.replace(/^[\s<!\-/;]+/, ''), lg)) add(lg, rel + ' comment in Spanish: ' + line.trim().slice(0, 100));
	}
}
if (problems.length) {
	console.log(problems.join('\n'));
	console.log('\n' + problems.length + ' problem(s).');
	process.exit(1);
}
console.log('sdk\\en and sdk\\it: no problems.');
