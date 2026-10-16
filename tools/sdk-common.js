// Tables and functions shared by sync-sdk.js and check-sdk.js.
const fs = require('fs');
const path = require('path');

const SDK = fs.existsSync(path.resolve(__dirname, '..', 'sdk')) ? path.resolve(__dirname, '..', 'sdk') : path.resolve(__dirname, '..', '..', 'sdk');
const ES = path.join(SDK, 'es');
const walk = d => { let o = []; for (const e of fs.readdirSync(d, { withFileTypes: true })) { const p = path.join(d, e.name); if (e.isDirectory()) o = o.concat(walk(p)); else o.push(p); } return o; };
const NAMES = {
	en: { herramientas: 'tools', ejemplos: 'examples', 'plantilla-mod': 'mod-template', leeme: 'README.md', funciones: 'game-functions.txt', scripts: 'scripts.md', modGuides: { graficos: 'graphics', formaciones: 'formations', habilidades: 'abilities', opciones: 'options', imagenes: 'images', publicar: 'publishing', unidades: 'units', validar: 'validating' } },
	it: { herramientas: 'strumenti', ejemplos: 'esempi', 'plantilla-mod': 'modello-mod', leeme: 'LEGGIMI.md', funciones: 'funzioni-del-gioco.txt', scripts: 'script.md', modGuides: { graficos: 'grafica', formaciones: 'formazioni', habilidades: 'abilita', opciones: 'opzioni', imagenes: 'immagini', publicar: 'pubblicare', unidades: 'unita', validar: 'validare' } },
};
const EXAMPLES = {
	'civilizacion-ejemplo': { en: 'civilization-example', it: 'esempio-civilta' },
	'especialidad-ejemplo': { en: 'specialty-example', it: 'esempio-specialita' },
	'formacion-ejemplo': { en: 'formation-example', it: 'esempio-formazione' },
	'fortin-ejemplo': { en: 'outpost-example', it: 'esempio-avamposto' },
	'habilidades-ejemplo': { en: 'ability-example', it: 'esempio-abilita' },
	'ia-tactica-ejemplo': { en: 'ai-tactic-example', it: 'esempio-tattica-ia' },
	'idioma-frances': { en: 'french-language', it: 'lingua-francese' },
	'mapa-aleatorio-ejemplo': { en: 'random-map-example', it: 'esempio-mappa-casuale' },
	'objeto-ejemplo': { en: 'item-example', it: 'esempio-oggetto' },
	'opciones-ejemplo': { en: 'options-example', it: 'esempio-opzioni' },
	'ordenes-propias': { en: 'custom-orders', it: 'ordini-propri' },
	'parches-ejemplo': { en: 'patches-example', it: 'esempio-patch' },
	'scripts-ejemplo': { en: 'scripts-example', it: 'esempio-script' },
	'textos-propios': { en: 'custom-texts', it: 'testi-propri' },
	'unidad-ejemplo': { en: 'unit-example', it: 'esempio-unita' },
	'unidad-graficos-ejemplo': { en: 'unit-graphics-example', it: 'esempio-unita-grafica' },
	'unidad-graficos-generador': { en: 'unit-graphics-generator', it: 'generatore-unita-grafica' },
	'victoria-ejemplo': { en: 'victory-example', it: 'esempio-vittoria' },
};
const TOOLS = {
	'clonar-civilizacion': { en: 'clone-civilization', it: 'clona-civilta' },
	'convertir-imagen': { en: 'convert-image', it: 'converti-immagine' },
	'convertir-sprite': { en: 'convert-sprite', it: 'converti-sprite' },
	'crear-entidad': { en: 'create-entity', it: 'crea-entita' },
	'crear-formacion': { en: 'create-formation', it: 'crea-formazione' },
	'crear-fortin': { en: 'create-outpost', it: 'crea-avamposto' },
	'crear-habilidad': { en: 'create-ability', it: 'crea-abilita' },
	'crear-mapa-aleatorio': { en: 'create-random-map', it: 'crea-mappa-casuale' },
	'crear-objeto': { en: 'create-item', it: 'crea-oggetto' },
	'crear-opcion': { en: 'create-option', it: 'crea-opzione' },
	'crear-plantilla': { en: 'create-template', it: 'crea-modello' },
	'crear-unidad': { en: 'create-unit', it: 'crea-unita' },
	'extraer-datos': { en: 'extract-data', it: 'estrai-dati' },
	'leer-rle': { en: 'read-rle', it: 'leggi-rle' },
	'reducir-mod': { en: 'shrink-mod', it: 'riduci-mod' },
	'validar-mod': { en: 'validate-mod', it: 'valida-mod' },
	'mod-comun': { en: 'mod-common', it: 'mod-comune' },
};
const EX_FILES = {
	'dibujar-soldado': { en: 'draw-soldier', it: 'disegna-soldato' },
	'MiCuartel': { en: 'MyBarracks', it: 'MiaCaserma' },
	'dar_amuleto': { en: 'give_amulet', it: 'dai_amuleto' },
	'heroes_veloces': { en: 'fast_heroes', it: 'eroi_veloci' },
	'Veterano.xml': { en: 'Veteran.xml', it: 'Veterano.xml' },
	'veterano.png': { en: 'veteran.png', it: 'veterano.png' },
	'boton.png': { en: 'button.png', it: 'pulsante.png' },
	'anillo.png': { en: 'ring.png', it: 'anello.png' },
};
const EX_DIRS = {
	'arte': { en: 'art', it: 'arte' },
	'dibujos': { en: 'drawings', it: 'disegni' },
};

const escapeRe = t => t.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');
const byLength = o => Object.keys(o).sort((a, b) => b.length - a.length);
const esNames = byLength(EXAMPLES);
const exampleRegex = new RegExp('(?<![\\w-])(' + esNames.join('|') + ')(?![\\w-])', 'g');
const toolRegex = new RegExp('(?<![\\w-])(' + byLength(TOOLS).join('|') + ')(?![\\w-])', 'g');
const fileRegex = new RegExp('(?<![\\w-])(' + byLength(EX_FILES).map(escapeRe).join('|') + ')(?![\\w-])', 'g');
const dirRegex = new RegExp('(?<![\\w-])(' + Object.keys(EX_DIRS).join('|') + ')(?=[\\\\/])', 'g');

const mapToolRel = (lg, rel) => rel.map(seg => {
	const base = seg.replace(/\.js$/, '');
	return TOOLS[base] ? TOOLS[base][lg] + (seg.endsWith('.js') ? '.js' : '') : seg;
});
const mapExampleRel = (lg, rel) => {
	const out = rel.slice();
	if (EXAMPLES[out[0]]) out[0] = EXAMPLES[out[0]][lg];
	for (let i = 1; i < out.length; i++) {
		const last = i === out.length - 1;
		const base = out[i].replace(/\.[^.]+$/, '');
		if (last && EX_FILES[out[i]]) out[i] = EX_FILES[out[i]][lg];
		else if (last && EX_FILES[base]) out[i] = EX_FILES[base][lg] + out[i].slice(base.length);
		else if (!last && EX_DIRS[out[i]]) out[i] = EX_DIRS[out[i]][lg];
	}
	return out;
};

const codeOf = (text, ext) => {
	let t = text.replace(/\r\n/g, '\n');
	if (ext === '.xml') t = t.replace(/<!--[\s\S]*?-->/g, '');
	else if (ext === '.vs' || ext === '.js') {
		t = t.replace(/\/\*[\s\S]*?\*\//g, '').split('\n').map(l => l.replace(/\/\/.*$/, '')).join('\n');
		t = t.replace(/(Mod_Log(?:Int)?\()"[^"]*"/g, '$1""');
	}
	else if (ext === '.ini') t = t.split('\n').filter(l => !/^\s*;/.test(l)).join('\n');
	return t.replace(/\s+/g, ' ').trim();
};

const renameTokens = (t, lg) => t.replace(toolRegex, (m, n) => TOOLS[n][lg]).replace(fileRegex, (m, n) => EX_FILES[n][lg]).replace(dirRegex, (m, n) => EX_DIRS[n][lg]);

const makeFix = lg => {
	const N = NAMES[lg];
	return (t, ext) => {
		t = t.replace(/herramientas([\\/])/g, N.herramientas + '$1').replace(/ejemplos([\\/])/g, N.ejemplos + '$1').replace(/plantilla-mod/g, N['plantilla-mod']);
		t = t.replace(/(['"])herramientas\1/g, '$1' + N.herramientas + '$1').replace(/(['"])ejemplos\1/g, '$1' + N.ejemplos + '$1');
		t = t.replace(/\b(graficos|formaciones|habilidades|opciones|imagenes|publicar|unidades|validar)\.md\b/g, (m, g) => N.modGuides[g] + '.md');
		t = renameTokens(t, lg);
		return ext === '.js' ? t.replace(exampleRegex, (m, n) => EXAMPLES[n][lg]) : t;
	};
};

module.exports = { SDK, ES, NAMES, EXAMPLES, TOOLS, EX_FILES, EX_DIRS, mapToolRel, mapExampleRel, renameTokens, exampleRegex, walk, codeOf, makeFix };
