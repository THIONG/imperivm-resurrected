// Lingua dei messaggi degli strumenti. Si sceglie, in quest'ordine, con:
//   1. l'opzione --lang es|en|it nella riga di comando (qui viene tolta da process.argv, quindi gli strumenti non la vedono)
//   2. la variabile d'ambiente IMPERIVM_SDK_LANG
//   3. il file lib\lang.json ({"lang": "en"}), che ogni cartella dell'SDK include con la propria lingua
//   4. spagnolo
// Uso in uno strumento:  const { T } = require('./lib/lang');  console.log(T('testo in spagnolo', 'text in English', 'testo in italiano'));
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
	catch (e) { /* nessun file: spagnolo */ }
}
if (!lang) lang = 'es';

// Le opzioni si accettano nelle tre lingue (--nombre, --name, --nome); qui ogni nome viene convertito in quello spagnolo, che è quello che leggono gli strumenti.
const OPCIONES = {
	categoria: ['category', 'categoria'], color: ['color', 'colore'], edificio: ['building', 'edificio'], tipo: ['type', 'tipo'], version: ['version', 'versione'],
	alcance: ['range', 'raggio'], alto: ['height', 'altezza'], ancho: ['width', 'larghezza'], armadura: ['armor', 'armatura'], boton: ['button', 'pulsante'],
	capas: ['layers', 'livelli'], clase: ['class', 'classe'], clases: ['classes', 'classi'], como: ['like', 'come'], convenio: ['convention', 'convenzione'],
	'coste-comida': ['food-cost', 'costo-cibo'], 'coste-energia': ['energy-cost', 'costo-energia'], 'coste-oro': ['gold-cost', 'costo-oro'],
	dano: ['damage', 'danno'], 'dar-heroes': ['give-heroes', 'dai-eroi'], datos: ['data', 'dati'], defecto: ['default', 'predefinito'],
	'defensa-pierce': ['pierce-defense', 'difesa-pierce'], 'defensa-slash': ['slash-defense', 'difesa-slash'], descripcion: ['description', 'descrizione'],
	'descripcion-en': ['description-en', 'descrizione-en'], 'descripcion-it': ['description-it', 'descrizione-it'], distorsion: ['distortion', 'distorsione'],
	etiquetas: ['tags', 'tag'], experiencia: ['experience', 'esperienza'], 'flanco-x': ['wing-x', 'ala-x'], 'flanco-y': ['wing-y', 'ala-y'], fondo: ['background', 'sfondo'],
	fotogramas: ['frames', 'fotogrammi'], frente: ['front', 'fronte'], huella: ['footprint', 'impronta'], 'huella-png': ['footprint-png', 'impronta-png'],
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
// Con --tipo e --sombra si accettano anche i loro valori in inglese e in italiano.
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
// I messaggi mostrano le opzioni nella lingua attiva.
function local(s) {
	if (lang === 'es' || typeof s !== 'string') return s;
	const col = lang === 'en' ? 0 : 1;
	s = s.replace(opcionesRe, (m, k) => '--' + OPCIONES[k][col]);
	for (const t of TEXTOS) s = s.split(t[0]).join(t[col + 1]);
	return s;
}

// Restituisce il testo della lingua attiva; se manca la traduzione, quello in spagnolo
function T(es, en, it) {
	if (lang === 'en' && en !== undefined) return local(en);
	if (lang === 'it' && it !== undefined) return local(it);
	return es;
}

module.exports = { lang, T };
