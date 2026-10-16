// Common pieces of create-ability.js, create-formation.js and create-option.js (Imperivm Resurrected SDK).
//
//   - Plan: gathers all the files to be written and writes them all at once at the end. If something fails midway, it undoes what was written:
//     the mod folder never ends up with half-written files.
//   - Reading the game's classes and orders (DATA folder) and the mod's own, to check for repeated names and taken hotkeys.
const fs = require('fs'), path = require('path');
const { T } = require('./lang');

const lower = s => String(s).toLowerCase();
const ci = (dir, name) => { try { return fs.readdirSync(dir).find(f => lower(f) === lower(name)); } catch (e) { return undefined; } };
const readLatin1 = f => fs.readFileSync(f, 'latin1');
const readUtf8 = f => fs.readFileSync(f, 'utf8').replace(/^﻿/, '');
const stripComments = t => t.replace(/<!--[\s\S]*?-->/g, '');
const xmlEsc = s => String(s).replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;').replace(/"/g, '&quot;');

// "Expected" error: the message is printed, with no trace, and it exits with 1
class Fallo extends Error {}
const fallo = msg => { throw new Fallo(msg); };

// The attributes of a tag, with the name in lowercase
function attrs(tag) {
	const out = {};
	for (const m of tag.matchAll(/([A-Za-z_][\w-]*)\s*=\s*("([^"]*)"|'([^']*)')/g)) out[lower(m[1])] = m[3] !== undefined ? m[3] : m[4];
	return out;
}

// Options --name value (also --name=value) and positionals. "boolean" are the options that take no value
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

// An integer from --option, within a range
function entero(opt, k, min, max) {
	if (opt[k] === undefined) return undefined;
	if (!/^-?\d+$/.test(String(opt[k]))) fallo(T('--' + k + ' tiene que ser un número entero (has puesto "' + opt[k] + '")', '--' + k + ' must be an integer (you gave "' + opt[k] + '")', '--' + k + ' deve essere un numero intero (hai messo "' + opt[k] + '")'));
	const n = Number(opt[k]);
	if (n < min || n > max) fallo(T('--' + k + ' tiene que estar entre ' + min + ' y ' + max + ' (has puesto ' + n + ')', '--' + k + ' must be between ' + min + ' and ' + max + ' (you gave ' + n + ')', '--' + k + ' deve essere tra ' + min + ' e ' + max + ' (hai messo ' + n + ')'));
	return n;
}

// Checks that the options given belong to the tool
function soloConocidas(opt, conocidas) {
	for (const k of Object.keys(opt)) if (!conocidas.includes(k)) {
		const lista = conocidas.map(c => '--' + c).join(', ');
		fallo(T('opción desconocida: --' + k + ' (las que hay: ' + lista + ')', 'unknown option: --' + k + ' (available: ' + lista + ')', 'opzione sconosciuta: --' + k + ' (disponibili: ' + lista + ')'));
	}
}

function carpetaData(dataDir) {
	if (!dataDir || !fs.existsSync(dataDir) || !fs.statSync(dataDir).isDirectory()) fallo(T('la carpeta DATA no existe: ' + dataDir + '\n  La saca extract-data.js (ver el README del SDK).',
		'the DATA folder does not exist: ' + dataDir + '\n  extract-data.js extracts it (see the SDK README).',
		'la cartella DATA non esiste: ' + dataDir + '\n  La estrae extract-data.js (vedi il README dell\'SDK).'));
	for (const n of ['classes', 'commands']) if (!ci(dataDir, n)) fallo(T(dataDir + ' no parece la carpeta DATA del juego (no tiene "' + n + '").\n  La saca extract-data.js (ver el README del SDK).',
		dataDir + ' does not look like the game\'s DATA folder (it has no "' + n + '").\n  extract-data.js extracts it (see the SDK README).',
		dataDir + ' non sembra la cartella DATA del gioco (manca "' + n + '").\n  La estrae extract-data.js (vedi il README dell\'SDK).'));
	return dataDir;
}

// --- Write plan ----------------------------------------------------------------------------------------------------------
class Plan {
	constructor() { this.items = new Map(); }  // ruta -> { texto, nuevo }
	// Reads the current text of a mod file, or the one this plan already holds for it
	texto(ruta) {
		if (this.items.has(ruta)) return this.items.get(ruta).texto;
		return fs.existsSync(ruta) ? readUtf8(ruta) : null;
	}
	existe(ruta) { return this.items.has(ruta) || fs.existsSync(ruta); }
	poner(ruta, texto) {
		const previo = this.items.get(ruta);
		this.items.set(ruta, { texto, nuevo: previo ? previo.nuevo : !fs.existsSync(ruta) });
	}
	// Writes everything. If something fails, it leaves the folder as it was
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
				try { if (h.antes) fs.writeFileSync(h.ruta, h.antes); else fs.rmSync(h.ruta, { force: true }); } catch (e2) { /* nothing more to do */ }
				for (const c of h.carpetas.slice().reverse()) { try { fs.rmdirSync(c); } catch (e2) { /* not empty */ } }
			}
			fallo(T('no he podido escribir los archivos (' + e.message + '). No he dejado nada a medias.', 'could not write the files (' + e.message + '). Nothing was left half-written.', 'impossibile scrivere i file (' + e.message + '). Non è rimasto niente a metà.'));
		}
	}
}

// Appends text right before the last closing tag of an XML file that already exists (</patch>, </commands>...)
function antesDelCierre(texto, cierre, nuevo, ruta) {
	const i = texto.lastIndexOf(cierre);
	if (i < 0) fallo(T(ruta + ' ya existe y no tiene ' + cierre + ': no sé dónde añadir. Arréglalo o bórralo y vuelve a ejecutar.',
		ruta + ' already exists and has no ' + cierre + ': cannot tell where to add. Fix it or delete it and run again.',
		ruta + ' esiste già e non ha ' + cierre + ': non so dove aggiungere. Correggilo o cancellalo e rilancia.'));
	return texto.slice(0, i).replace(/[ \t]*$/, '') + nuevo + texto.slice(i);
}

// --- Game data -----------------------------------------------------------------------------------------------------------
// Classes: id (and altid) in lowercase -> { id, parent, file (file name), mod }
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

// The ancestors of a class (itself included), in lowercase
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

// Orders: [{ name, key, srcs: [classes], file, mod, menu }] from DATA\commands and from the mod's data\commands and patches\data\commands
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

// Orders that already use the hotkey in a class related to the one that will receive it (the same, an ancestor or a child)
function chocaTecla(ordenes, clases, tecla, clase, ignorar = []) {
	return ordenes.filter(o => lower(o.key) === lower(tecla) && !ignorar.includes(o.name) && o.srcs.some(s => emparentadas(clases, s, clase)));
}

// --- minimal mod.json, if the folder does not have one ---------------------------------------------------------------------
function modJsonMinimo(plan, modDir, categoria, descripcion) {
	const ruta = path.join(modDir, 'mod.json');
	if (plan.existe(ruta)) return false;
	const id = lower(path.basename(path.resolve(modDir))).replace(/[^a-z0-9_-]+/g, '-').replace(/^-+|-+$/g, '') || 'mi-mod';
	const j = { id, name: path.basename(path.resolve(modDir)), version: '1.0', category: categoria, tags: ['Gameplay'], authors: ['Me'], description: descripcion, redistributable: true };
	plan.poner(ruta, JSON.stringify(j, null, 2) + '\n');
	return true;
}

// Runs a tool: expected failures come out as a single error line, with no trace
function ejecutar(fn) {
	try { fn(); }
	catch (e) {
		if (e instanceof Fallo) { console.error(T('error: ', 'error: ', 'errore: ') + e.message); process.exit(1); }
		throw e;
	}
}

module.exports = { fs, path, lower, ci, readLatin1, readUtf8, stripComments, xmlEsc, attrs, Fallo, fallo, parseArgs, entero, soloConocidas, carpetaData, Plan,
	antesDelCierre, leerClases, linaje, emparentadas, leerOrdenes, chocaTecla, modJsonMinimo, ejecutar };
