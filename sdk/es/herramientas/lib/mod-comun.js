// Piezas comunes de crear-habilidad.js, crear-formacion.js y crear-opcion.js (SDK de Imperivm Resurrected).
//
//   - Plan: junta todos los archivos que hay que escribir, y los escribe de golpe al final. Si algo falla a mitad, deshace lo escrito:
//     la carpeta del mod nunca queda con archivos a medias.
//   - Lectura de las clases y órdenes del juego (carpeta DATA) y del propio mod, para comprobar nombres repetidos y teclas ocupadas.
const fs = require('fs'), path = require('path');
const { T } = require('./lang');

const lower = s => String(s).toLowerCase();
const ci = (dir, name) => { try { return fs.readdirSync(dir).find(f => lower(f) === lower(name)); } catch (e) { return undefined; } };
const readLatin1 = f => fs.readFileSync(f, 'latin1');
const readUtf8 = f => fs.readFileSync(f, 'utf8').replace(/^﻿/, '');
const stripComments = t => t.replace(/<!--[\s\S]*?-->/g, '');
const xmlEsc = s => String(s).replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;').replace(/"/g, '&quot;');

// Error "esperado": se imprime el mensaje, sin traza, y se sale con 1
class Fallo extends Error {}
const fallo = msg => { throw new Fallo(msg); };

// Los atributos de una etiqueta, con el nombre en minúsculas
function attrs(tag) {
	const out = {};
	for (const m of tag.matchAll(/([A-Za-z_][\w-]*)\s*=\s*("([^"]*)"|'([^']*)')/g)) out[lower(m[1])] = m[3] !== undefined ? m[3] : m[4];
	return out;
}

// Opciones --nombre valor (también --nombre=valor) y posicionales. "boolean" son las opciones que no llevan valor
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

// Un entero de --opción, dentro de un rango
function entero(opt, k, min, max) {
	if (opt[k] === undefined) return undefined;
	if (!/^-?\d+$/.test(String(opt[k]))) fallo(T('--' + k + ' tiene que ser un número entero (has puesto "' + opt[k] + '")', '--' + k + ' must be an integer (you gave "' + opt[k] + '")', '--' + k + ' deve essere un numero intero (hai messo "' + opt[k] + '")'));
	const n = Number(opt[k]);
	if (n < min || n > max) fallo(T('--' + k + ' tiene que estar entre ' + min + ' y ' + max + ' (has puesto ' + n + ')', '--' + k + ' must be between ' + min + ' and ' + max + ' (you gave ' + n + ')', '--' + k + ' deve essere tra ' + min + ' e ' + max + ' (hai messo ' + n + ')'));
	return n;
}

// Comprueba que las opciones que se han puesto son de la herramienta
function soloConocidas(opt, conocidas) {
	for (const k of Object.keys(opt)) if (!conocidas.includes(k)) {
		const lista = conocidas.map(c => '--' + c).join(', ');
		fallo(T('opción desconocida: --' + k + ' (las que hay: ' + lista + ')', 'unknown option: --' + k + ' (available: ' + lista + ')', 'opzione sconosciuta: --' + k + ' (disponibili: ' + lista + ')'));
	}
}

function carpetaData(dataDir) {
	if (!dataDir || !fs.existsSync(dataDir) || !fs.statSync(dataDir).isDirectory()) fallo(T('la carpeta DATA no existe: ' + dataDir + '\n  La saca extraer-datos.js (ver el README del SDK).',
		'the DATA folder does not exist: ' + dataDir + '\n  extraer-datos.js extracts it (see the SDK README).',
		'la cartella DATA non esiste: ' + dataDir + '\n  La estrae extraer-datos.js (vedi il README dell\'SDK).'));
	for (const n of ['classes', 'commands']) if (!ci(dataDir, n)) fallo(T(dataDir + ' no parece la carpeta DATA del juego (no tiene "' + n + '").\n  La saca extraer-datos.js (ver el README del SDK).',
		dataDir + ' does not look like the game\'s DATA folder (it has no "' + n + '").\n  extraer-datos.js extracts it (see the SDK README).',
		dataDir + ' non sembra la cartella DATA del gioco (manca "' + n + '").\n  La estrae extraer-datos.js (vedi il README dell\'SDK).'));
	return dataDir;
}

// --- Plan de escritura ---------------------------------------------------------------------------------------------------
class Plan {
	constructor() { this.items = new Map(); }  // ruta -> { texto, nuevo }
	// Lee el texto actual de un archivo del mod, o el que ya tiene este plan para él
	texto(ruta) {
		if (this.items.has(ruta)) return this.items.get(ruta).texto;
		return fs.existsSync(ruta) ? readUtf8(ruta) : null;
	}
	existe(ruta) { return this.items.has(ruta) || fs.existsSync(ruta); }
	poner(ruta, texto) {
		const previo = this.items.get(ruta);
		this.items.set(ruta, { texto, nuevo: previo ? previo.nuevo : !fs.existsSync(ruta) });
	}
	// Escribe todo. Si algo falla, deja la carpeta como estaba
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
				try { if (h.antes) fs.writeFileSync(h.ruta, h.antes); else fs.rmSync(h.ruta, { force: true }); } catch (e2) { /* nada más que hacer */ }
				for (const c of h.carpetas.slice().reverse()) { try { fs.rmdirSync(c); } catch (e2) { /* no está vacía */ } }
			}
			fallo(T('no he podido escribir los archivos (' + e.message + '). No he dejado nada a medias.', 'could not write the files (' + e.message + '). Nothing was left half-written.', 'impossibile scrivere i file (' + e.message + '). Non è rimasto niente a metà.'));
		}
	}
}

// Añade texto justo antes de la última etiqueta de cierre de un archivo XML que ya existe (</patch>, </commands>...)
function antesDelCierre(texto, cierre, nuevo, ruta) {
	const i = texto.lastIndexOf(cierre);
	if (i < 0) fallo(T(ruta + ' ya existe y no tiene ' + cierre + ': no sé dónde añadir. Arréglalo o bórralo y vuelve a ejecutar.',
		ruta + ' already exists and has no ' + cierre + ': cannot tell where to add. Fix it or delete it and run again.',
		ruta + ' esiste già e non ha ' + cierre + ': non so dove aggiungere. Correggilo o cancellalo e rilancia.'));
	return texto.slice(0, i).replace(/[ \t]*$/, '') + nuevo + texto.slice(i);
}

// --- Datos del juego -----------------------------------------------------------------------------------------------------
// Clases: id (y altid) en minúsculas -> { id, parent, file (nombre del archivo), mod }
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

// Los antepasados de una clase (ella incluida), en minúsculas
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

// Órdenes: [{ name, key, srcs: [clases], file, mod, menu }] de DATA\commands y de los data\commands y patches\data\commands del mod
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

// Órdenes que ya usan la tecla en una clase relacionada con la que la va a recibir (la misma, una antepasada o una hija)
function chocaTecla(ordenes, clases, tecla, clase, ignorar = []) {
	return ordenes.filter(o => lower(o.key) === lower(tecla) && !ignorar.includes(o.name) && o.srcs.some(s => emparentadas(clases, s, clase)));
}

// --- mod.json mínimo, si la carpeta no tiene -------------------------------------------------------------------------------
function modJsonMinimo(plan, modDir, categoria, descripcion) {
	const ruta = path.join(modDir, 'mod.json');
	if (plan.existe(ruta)) return false;
	const id = lower(path.basename(path.resolve(modDir))).replace(/[^a-z0-9_-]+/g, '-').replace(/^-+|-+$/g, '') || 'mi-mod';
	const j = { id, name: path.basename(path.resolve(modDir)), version: '1.0', category: categoria, tags: ['Gameplay'], authors: ['Me'], description: descripcion, redistributable: true };
	plan.poner(ruta, JSON.stringify(j, null, 2) + '\n');
	return true;
}

// Ejecuta una herramienta: los fallos esperados salen como una línea de error, sin traza
function ejecutar(fn) {
	try { fn(); }
	catch (e) {
		if (e instanceof Fallo) { console.error(T('error: ', 'error: ', 'errore: ') + e.message); process.exit(1); }
		throw e;
	}
}

module.exports = { fs, path, lower, ci, readLatin1, readUtf8, stripComments, xmlEsc, attrs, Fallo, fallo, parseArgs, entero, soloConocidas, carpetaData, Plan,
	antesDelCierre, leerClases, linaje, emparentadas, leerOrdenes, chocaTecla, modJsonMinimo, ejecutar };
