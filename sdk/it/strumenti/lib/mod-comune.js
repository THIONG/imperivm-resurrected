// Parti comuni di crea-abilita.js, crea-formazione.js e crea-opzione.js (SDK di Imperivm Resurrected).
//
//   - Piano: raccoglie tutti i file da scrivere e li scrive in un colpo solo alla fine. Se qualcosa fallisce a metà, annulla quanto scritto:
//     la cartella della mod non resta mai con file a metà.
//   - Lettura delle classi e degli ordini del gioco (cartella DATA) e della mod stessa, per controllare nomi ripetuti e tasti occupati.
const fs = require('fs'), path = require('path');
const { T } = require('./lang');

const lower = s => String(s).toLowerCase();
const ci = (dir, name) => { try { return fs.readdirSync(dir).find(f => lower(f) === lower(name)); } catch (e) { return undefined; } };
const readLatin1 = f => fs.readFileSync(f, 'latin1');
const readUtf8 = f => fs.readFileSync(f, 'utf8').replace(/^﻿/, '');
const stripComments = t => t.replace(/<!--[\s\S]*?-->/g, '');
const xmlEsc = s => String(s).replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;').replace(/"/g, '&quot;');

// Errore "previsto": si stampa il messaggio, senza traccia, e si esce con 1
class Fallo extends Error {}
const fallo = msg => { throw new Fallo(msg); };

// Gli attributi di un tag, con il nome in minuscolo
function attrs(tag) {
	const out = {};
	for (const m of tag.matchAll(/([A-Za-z_][\w-]*)\s*=\s*("([^"]*)"|'([^']*)')/g)) out[lower(m[1])] = m[3] !== undefined ? m[3] : m[4];
	return out;
}

// Opzioni --nome valore (anche --nome=valore) e posizionali. "boolean" sono le opzioni che non hanno valore
function parseArgs(argv, boolean = []) {
	const pos = [], opt = {};
	for (let i = 0; i < argv.length; i++) {
		const a = argv[i];
		if (a.startsWith('--')) {
			const eq = a.indexOf('=');
			const k = eq > 0 ? a.slice(2, eq) : a.slice(2);
			if (boolean.includes(k)) { opt[k] = true; continue; }
			if (eq > 0) { opt[k] = a.slice(eq + 1); continue; }
			if (i + 1 >= argv.length) fallo(T('a --' + k + ' le falta el valor', '--' + k + ' is missing its value', 'a --' + k + ' manca il valore'));
			opt[k] = argv[++i];
		}
		else pos.push(a);
	}
	return { pos, opt };
}

// Un intero di --opción, entro un intervallo
function entero(opt, k, min, max) {
	if (opt[k] === undefined) return undefined;
	if (!/^-?\d+$/.test(String(opt[k]))) fallo(T('--' + k + ' tiene que ser un número entero (has puesto "' + opt[k] + '")', '--' + k + ' must be an integer (you gave "' + opt[k] + '")', '--' + k + ' deve essere un numero intero (hai messo "' + opt[k] + '")'));
	const n = Number(opt[k]);
	if (n < min || n > max) fallo(T('--' + k + ' tiene que estar entre ' + min + ' y ' + max + ' (has puesto ' + n + ')', '--' + k + ' must be between ' + min + ' and ' + max + ' (you gave ' + n + ')', '--' + k + ' deve essere tra ' + min + ' e ' + max + ' (hai messo ' + n + ')'));
	return n;
}

// Controlla che le opzioni indicate siano quelle dello strumento
function soloConocidas(opt, conocidas) {
	for (const k of Object.keys(opt)) if (!conocidas.includes(k)) {
		const lista = conocidas.map(c => '--' + c).join(', ');
		fallo(T('opción desconocida: --' + k + ' (las que hay: ' + lista + ')', 'unknown option: --' + k + ' (available: ' + lista + ')', 'opzione sconosciuta: --' + k + ' (disponibili: ' + lista + ')'));
	}
}

function carpetaData(dataDir) {
	if (!dataDir || !fs.existsSync(dataDir) || !fs.statSync(dataDir).isDirectory()) fallo(T('la carpeta DATA no existe: ' + dataDir + '\n  La saca estrai-dati.js (ver el README del SDK).',
		'the DATA folder does not exist: ' + dataDir + '\n  estrai-dati.js extracts it (see the SDK README).',
		'la cartella DATA non esiste: ' + dataDir + '\n  La estrae estrai-dati.js (vedi il README dell\'SDK).'));
	for (const n of ['classes', 'commands']) if (!ci(dataDir, n)) fallo(T(dataDir + ' no parece la carpeta DATA del juego (no tiene "' + n + '").\n  La saca estrai-dati.js (ver el README del SDK).',
		dataDir + ' does not look like the game\'s DATA folder (it has no "' + n + '").\n  estrai-dati.js extracts it (see the SDK README).',
		dataDir + ' non sembra la cartella DATA del gioco (manca "' + n + '").\n  La estrae estrai-dati.js (vedi il README dell\'SDK).'));
	return dataDir;
}

// --- Piano di scrittura --------------------------------------------------------------------------------------------------
class Plan {
	constructor() { this.items = new Map(); }  // ruta -> { texto, nuevo }
	// Legge il testo attuale di un file della mod, o quello che questo piano ha già per lui
	texto(ruta) {
		if (this.items.has(ruta)) return this.items.get(ruta).texto;
		return fs.existsSync(ruta) ? readUtf8(ruta) : null;
	}
	existe(ruta) { return this.items.has(ruta) || fs.existsSync(ruta); }
	poner(ruta, texto) {
		const previo = this.items.get(ruta);
		this.items.set(ruta, { texto, nuevo: previo ? previo.nuevo : !fs.existsSync(ruta) });
	}
	// Scrive tutto. Se qualcosa fallisce, lascia la cartella com'era
	escribir() {
		const hecho = [];  // { ruta, antes: Buffer|null, carpetas: [] }
		try {
			for (const [ruta, it] of this.items) {
				const carpetas = [];
				let d = path.dirname(ruta);
				while (!fs.existsSync(d)) { carpetas.unshift(d); d = path.dirname(d); }
				const antes = fs.existsSync(ruta) ? fs.readFileSync(ruta) : null;
				fs.mkdirSync(path.dirname(ruta), { recursive: true });
				hecho.push({ ruta, antes, carpetas });
				fs.writeFileSync(ruta, it.texto, 'utf8');
			}
		}
		catch (e) {
			for (const h of hecho.reverse()) {
				try { if (h.antes) fs.writeFileSync(h.ruta, h.antes); else fs.rmSync(h.ruta, { force: true }); } catch (e2) { /* nient'altro da fare */ }
				for (const c of h.carpetas.slice().reverse()) { try { fs.rmdirSync(c); } catch (e2) { /* non è vuota */ } }
			}
			fallo(T('no he podido escribir los archivos (' + e.message + '). No he dejado nada a medias.', 'could not write the files (' + e.message + '). Nothing was left half-written.', 'impossibile scrivere i file (' + e.message + '). Non è rimasto niente a metà.'));
		}
	}
}

// Aggiunge testo subito prima dell'ultimo tag di chiusura di un file XML già esistente (</patch>, </commands>...)
function antesDelCierre(texto, cierre, nuevo, ruta) {
	const i = texto.lastIndexOf(cierre);
	if (i < 0) fallo(T(ruta + ' ya existe y no tiene ' + cierre + ': no sé dónde añadir. Arréglalo o bórralo y vuelve a ejecutar.',
		ruta + ' already exists and has no ' + cierre + ': cannot tell where to add. Fix it or delete it and run again.',
		ruta + ' esiste già e non ha ' + cierre + ': non so dove aggiungere. Correggilo o cancellalo e rilancia.'));
	return texto.slice(0, i).replace(/[ \t]*$/, '') + nuevo + texto.slice(i);
}

// --- Dati del gioco ------------------------------------------------------------------------------------------------------
// Classi: id (e altid) in minuscolo -> { id, parent, file (nome del file), mod }
function leerClases(dataDir, modDir) {
	const map = new Map();
	const add = (file, text, deMod) => {
		const h = text.match(/<class\b[^>]*>/);
		if (!h) return;
		const a = attrs(h[0]);
		if (!a.id) return;
		const info = { id: a.id, parent: a.parent || '', file, mod: deMod };
		map.set(lower(a.id), info);
		if (a.altid) map.set(lower(a.altid), info);
	};
	const cd = path.join(dataDir, ci(dataDir, 'classes'));
	for (const f of fs.readdirSync(cd)) if (/\.sc\.xml$/i.test(f)) add(f, readLatin1(path.join(cd, f)), false);
	const md = path.join(modDir, 'data', 'classes');
	if (fs.existsSync(md)) for (const f of fs.readdirSync(md)) if (/\.sc\.xml$/i.test(f)) add(f, readUtf8(path.join(md, f)), true);
	return map;
}

// Gli antenati di una classe (inclusa lei), in minuscolo
function linaje(clases, id) {
	const out = [];
	let c = clases.get(lower(id));
	let n = 0;
	if (!c) return [lower(id)];
	while (c && n++ < 40) {
		out.push(lower(c.id));
		c = c.parent ? clases.get(lower(c.parent)) : null;
	}
	return out;
}
const emparentadas = (clases, a, b) => linaje(clases, a).includes(lower(b)) || linaje(clases, b).includes(lower(a));

// Ordini: [{ name, key, srcs: [classi], file, mod, menu }] di DATA\commands e dei data\commands e patches\data\commands della mod
function leerOrdenes(dataDir, modDir) {
	const out = [];
	const leer = (file, text, deMod) => {
		for (const m of stripComments(text).matchAll(/<cmd\b([^>]*)>([\s\S]*?)<\/cmd>/g)) {
			const a = attrs(m[1]);
			if (!a.name) continue;
			out.push({ name: a.name, key: a.key || '', srcs: [...m[2].matchAll(/<src\b([^>]*)>/g)].map(s => attrs(s[1]).obj).filter(Boolean), file, mod: deMod, menu: a.menu === '1', param: a.param, button: a.button });
		}
	};
	const cd = path.join(dataDir, ci(dataDir, 'commands'));
	for (const f of fs.readdirSync(cd)) if (/\.xml$/i.test(f)) leer(f, readLatin1(path.join(cd, f)), false);
	for (const sub of [['data', 'commands'], ['patches', 'data', 'commands']]) {
		const d = path.join(modDir, ...sub);
		if (fs.existsSync(d)) for (const f of fs.readdirSync(d)) if (/\.xml$/i.test(f)) leer(sub.join('/') + '/' + f, readUtf8(path.join(d, f)), true);
	}
	return out;
}

// Ordini che usano già il tasto in una classe collegata a quella che lo riceverà (la stessa, un'antenata o una figlia)
function chocaTecla(ordenes, clases, tecla, clase, ignorar = []) {
	return ordenes.filter(o => lower(o.key) === lower(tecla) && !ignorar.includes(o.name) && o.srcs.some(s => emparentadas(clases, s, clase)));
}

// --- mod.json minimo, se la cartella non ne ha uno -------------------------------------------------------------------------
function modJsonMinimo(plan, modDir, categoria, descripcion) {
	const ruta = path.join(modDir, 'mod.json');
	if (plan.existe(ruta)) return false;
	const id = lower(path.basename(path.resolve(modDir))).replace(/[^a-z0-9_-]+/g, '-').replace(/^-+|-+$/g, '') || 'mi-mod';
	const j = { id, name: path.basename(path.resolve(modDir)), version: '1.0', category: categoria, tags: [], authors: ['Me'], description: descripcion, redistributable: true };
	plan.poner(ruta, JSON.stringify(j, null, 2) + '\n');
	return true;
}

// Esegue uno strumento: gli errori previsti escono come una riga di errore, senza traccia
function ejecutar(fn) {
	try { fn(); }
	catch (e) {
		if (e instanceof Fallo) { console.error(T('error: ', 'error: ', 'errore: ') + e.message); process.exit(1); }
		throw e;
	}
}

module.exports = { fs, path, lower, ci, readLatin1, readUtf8, stripComments, xmlEsc, attrs, Fallo, fallo, parseArgs, entero, soloConocidas, carpetaData, Plan,
	antesDelCierre, leerClases, linaje, emparentadas, leerOrdenes, chocaTecla, modJsonMinimo, ejecutar };
