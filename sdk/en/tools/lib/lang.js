// Language of the tools' messages. It is chosen, in this order, with:
//   1. the --lang es|en|it option on the command line (it is removed from process.argv here, so the tools do not see it)
//   2. the IMPERIVM_SDK_LANG environment variable
//   3. the lib\lang.json file ({"lang": "en"}), which each SDK folder ships with its own language
//   4. Spanish
// Usage in a tool:  const { T } = require('./lib/lang');  console.log(T('<text in Spanish>', '<text in English>', '<text in Italian>'));
const fs = require('fs');
const path = require('path');

const VALID = ['es', 'en', 'it'];
let lang = null;

const i = process.argv.indexOf('--lang');
if (i >= 0 && process.argv[i + 1]) {
	lang = String(process.argv[i + 1]).toLowerCase();
	process.argv.splice(i, 2);
}
if (!VALID.includes(lang)) lang = null;
if (!lang && VALID.includes(String(process.env.IMPERIVM_SDK_LANG || '').toLowerCase())) lang = String(process.env.IMPERIVM_SDK_LANG).toLowerCase();
if (!lang) {
	try {
		const j = JSON.parse(fs.readFileSync(path.join(__dirname, 'lang.json'), 'utf8'));
		if (VALID.includes(j.lang)) lang = j.lang;
	}
	catch (e) { /* no file: Spanish */ }
}
if (!lang) lang = 'es';

// Options are accepted in all three languages (--nombre, --name, --nome); here each name is turned into the Spanish one, which is what the tools read.
const OPCIONES = {
	categoria: ['category', 'categoria'], color: ['color', 'colore'], edificio: ['building', 'edificio'], tipo: ['type', 'tipo'], version: ['version', 'versione'],
	alcance: ['range', 'raggio'], alto: ['height', 'altezza'], ancho: ['width', 'larghezza'], armadura: ['armor', 'armatura'], boton: ['button', 'pulsante'],
	capas: ['layers', 'livelli'], clase: ['class', 'classe'], clases: ['classes', 'classi'], como: ['like', 'come'], convenio: ['convention', 'convenzione'],
	'coste-comida': ['food-cost', 'costo-cibo'], 'coste-energia': ['energy-cost', 'costo-energia'], 'coste-oro': ['gold-cost', 'costo-oro'],
	dano: ['damage', 'danno'], 'dar-heroes': ['give-heroes', 'dai-eroi'], datos: ['data', 'dati'], defecto: ['default', 'predefinito'],
	'defensa-pierce': ['pierce-defense', 'difesa-pierce'], 'defensa-slash': ['slash-defense', 'difesa-slash'], descripcion: ['description', 'descrizione'],
	'descripcion-en': ['description-en', 'descrizione-en'], 'descripcion-it': ['description-it', 'descrizione-it'], distorsion: ['distortion', 'distorsione'],
	etiquetas: ['tags', 'tag'], experiencia: ['experience', 'esperienza'], 'flanco-x': ['wing-x', 'ala-x'], 'flanco-y': ['wing-y', 'ala-y'], fondo: ['background', 'sfondo'],
	fotogramas: ['frames', 'fotogrammi'], fuera: ['exclude', 'escludi'], ocultar: ['hide', 'nascondi'], frente: ['front', 'fronte'], huella: ['footprint', 'impronta'], 'huella-png': ['footprint-png', 'impronta-png'],
	icono: ['icon', 'icona'], juego: ['game', 'gioco'], jugador: ['player', 'giocatore'], jugadores: ['players', 'giocatori'], lado: ['side', 'lato'],
	lienzo: ['canvas', 'tela'], nivel: ['level', 'livello'], nombre: ['name', 'nome'], 'nombre-en': ['name-en', 'nome-en'], 'nombre-it': ['name-it', 'nome-it'],
	'paleta-por-animacion': ['palette-per-animation', 'tavolozza-per-animazione'], pequeno: ['small', 'piccolo'],
	'permitir-incompleta': ['allow-incomplete', 'consenti-incompleta'], pies: ['feet', 'piedi'], plural: ['plural', 'plurale'], 'plural-en': ['plural-en', 'plurale-en'],
	'puntos-de': ['points-from', 'punti-da'], recta: ['straight', 'retta'], rutas: ['paths', 'percorsi'], salida: ['output', 'output'],
	'sin-aldeas': ['no-villages', 'senza-villaggi'], 'sin-girar': ['no-rotate', 'senza-rotazione'], sombra: ['shadow', 'ombra'], tecla: ['key', 'tasto'],
	tiempo: ['time', 'tempo'], 'umbral-alfa': ['alpha-threshold', 'soglia-alfa'], valores: ['values', 'valori'], 'valores-en': ['values-en', 'valori-en'],
	'valores-it': ['values-it', 'valori-it'], velocidad: ['speed', 'velocita'], verificar: ['verify', 'verifica'], vida: ['health', 'vita'],
};
const SINONIMOS = {
	tipo: { button: 'boton', icon: 'icono', pulsante: 'boton', icona: 'icono' },
	sombra: { files: 'archivos', file: 'archivos', none: 'ninguna', nessuna: 'ninguna' },
};
const TEXTOS = [
	['boton|icono', 'button|icon', 'pulsante|icona'],
	['auto|archivos|ninguna', 'auto|files|none', 'auto|file|nessuna'],
	['unidad|edificio', 'unit|building', 'unita|edificio'],
	['Unit=flancos,Horse=frente', 'Unit=wings,Horse=front', 'Unit=ali,Horse=fronte'],
];

const nombreCanonico = {};
for (const [k, nombres] of Object.entries(OPCIONES)) for (const n of nombres) nombreCanonico[n] = k;
// With --tipo and --sombra their values are also accepted in English and Italian.
for (let n = 2; n < process.argv.length; n++) {
	const a = process.argv[n];
	if (!a.startsWith('--') || a.length < 3) continue;
	const eq = a.indexOf('=');
	const k = eq > 0 ? a.slice(2, eq) : a.slice(2);
	const canonico = nombreCanonico[k] || k;
	if (canonico !== k) process.argv[n] = '--' + canonico + (eq > 0 ? a.slice(eq) : '');
	const valores = SINONIMOS[canonico];
	if (!valores) continue;
	if (eq > 0) {
		const v = valores[a.slice(eq + 1).toLowerCase()];
		if (v) process.argv[n] = '--' + canonico + '=' + v;
	}
	else if (n + 1 < process.argv.length && valores[String(process.argv[n + 1]).toLowerCase()]) process.argv[n + 1] = valores[String(process.argv[n + 1]).toLowerCase()];
}

const escapar = t => t.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');
const opcionesRe = new RegExp('(?<![\\w-])--(' + Object.keys(OPCIONES).sort((a, b) => b.length - a.length).map(escapar).join('|') + ')(?![\\w-])', 'g');
// Messages show the options in the active language.
function local(s) {
	if (lang === 'es' || typeof s !== 'string') return s;
	const col = lang === 'en' ? 0 : 1;
	s = s.replace(opcionesRe, (m, k) => '--' + OPCIONES[k][col]);
	for (const t of TEXTOS) s = s.split(t[0]).join(t[col + 1]);
	return s;
}

// Returns the text for the active language; if the translation is missing, the Spanish one
function T(es, en, it) {
	if (lang === 'en' && en !== undefined) return local(en);
	if (lang === 'it' && it !== undefined) return local(it);
	return es;
}

module.exports = { lang, T };
