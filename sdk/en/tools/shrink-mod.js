// Converts an "old-style" mod (whole .pak packages and copies of game files) into an Imperivm Resurrected mod
// that only carries what it changes or adds. What is identical to the game stays out: every player already has it.
//
//   node shrink-mod.js <old mod> <game folder> <output folder> [options]
//     --id <id>  --name <name>  --version <version>  --category <category>  --tags a,b,c
//     --paths    only shows which folders are inside the packages (writes nothing)
//
// What it does with each thing in the old mod:
//   Packs\*.pak        extracts each file that changes or is new, with its internal path (data\..., ui\..., sounds\...)
//   local\<language>.pak  the same, comparing with the game's package for that language. What is in currentlang\ goes to Local\<language>\
//                      (that language only) or, if it is the same in all the mod's languages, to currentlang\ (all)
//   other files     (Scenarios, Adventures, Conquests...) are kept if they change or are new
//   *.bak              dropped
// mod.json is created (or completed) with the options. At the end it reports how large the mod was and how large it is now.
const fs = require('fs');
const path = require('path');
const crypto = require('crypto');
const { T } = require('./lib/lang');

const args = process.argv.slice(2);
const opt = {};
const pos = [];
for (let i = 0; i < args.length; i++) {
	if (args[i].startsWith('--')) {
		const k = args[i].slice(2);
		if (k === 'rutas') opt.rutas = true;
		else opt[k] = args[++i];
	}
	else pos.push(args[i]);
}
const [oldDir, gameDir, outDir] = pos;
if (!oldDir || !gameDir || (!outDir && !opt.rutas)) {
	console.error(T(
		'uso: node shrink-mod.js <mod antiguo> <carpeta del juego> <carpeta de salida> [--id --nombre --version --categoria --etiquetas] [--rutas]',
		'usage: node shrink-mod.js <old mod> <game folder> <output folder> [--id --nombre --version --categoria --etiquetas] [--rutas]',
		'uso: node shrink-mod.js <mod vecchia> <cartella del gioco> <cartella di output> [--id --nombre --version --categoria --etiquetas] [--rutas]'));
	process.exit(1);
}

// --- .pak packages ("HMMSYS PackFile": index with prefix-compressed names; uncompressed data) ---
function readPak(file) {
	const b = fs.readFileSync(file);
	if (b.toString('latin1', 0, 15) !== 'HMMSYS PackFile') return null;
	const n = b.readUInt32LE(0x20);
	let p = 0x28, prev = '';
	const out = new Map();
	for (let i = 0; i < n; i++) {
		const len = b[p], sh = b[p + 1];
		p += 2;
		const name = prev.slice(0, sh) + b.toString('latin1', p, p + len - sh);
		p += len - sh;
		const off = b.readUInt32LE(p), size = b.readUInt32LE(p + 4);
		p += 8;
		prev = name;
		const data = b.subarray(off, off + size);
		out.set(name.toLowerCase(), {name, data, md5: crypto.createHash('md5').update(data).digest('hex')});
	}
	return out;
}
const md5 = buf => crypto.createHash('md5').update(buf).digest('hex');
function walk(dir, rel = '') {
	const out = [];
	if (!fs.existsSync(path.join(dir, rel))) return out;
	for (const e of fs.readdirSync(path.join(dir, rel), {withFileTypes: true})) {
		const r = rel ? rel + '\\' + e.name : e.name;
		if (e.isDirectory()) out.push(...walk(dir, r));
		else out.push(r);
	}
	return out;
}
const mb = n => (n / 1048576).toFixed(1) + ' MB';

// The game: Packs\*.pak together (by internal path) and local\<language>.pak separately
const gamePacks = new Map();
for (const f of fs.readdirSync(path.join(gameDir, 'Packs'))) {
	if (!/\.pak$/i.test(f)) continue;
	const pk = readPak(path.join(gameDir, 'Packs', f));
	if (pk) for (const [k, v] of pk) if (!gamePacks.has(k)) gamePacks.set(k, v);
}
const gameLocal = {};
for (const f of fs.readdirSync(path.join(gameDir, 'local'))) {
	const m = f.match(/^(.+)\.pak$/i);
	if (m) gameLocal[m[1].toLowerCase()] = readPak(path.join(gameDir, 'local', f)) || new Map();
}

if (opt.rutas) {
	for (const rel of walk(oldDir).filter(r => /\.pak$/i.test(r))) {
		const pk = readPak(path.join(oldDir, rel));
		if (!pk) continue;
		const tops = {};
		for (const v of pk.values()) {
			const t = v.name.split('\\')[0];
			tops[t] = (tops[t] || 0) + 1;
		}
		console.log(rel, JSON.stringify(tops));
	}
	process.exit(0);
}

if (fs.existsSync(outDir) && fs.readdirSync(outDir).length) {
	console.error(T('la carpeta de salida tiene que estar vacía: ', 'the output folder has to be empty: ', 'la cartella di output deve essere vuota: ') + outDir);
	process.exit(1);
}
fs.mkdirSync(outDir, {recursive: true});
function write(rel, data) {
	const full = path.join(outDir, rel);
	fs.mkdirSync(path.dirname(full), {recursive: true});
	fs.writeFileSync(full, data);
}

const stats = {before: 0, same: 0, sameBytes: 0, changed: 0, added: 0, kept: 0, keptBytes: 0, junk: 0};
function keep(rel, data, isNew) {
	write(rel, data);
	stats[isNew ? 'added' : 'changed']++;
	stats.kept++;
	stats.keptBytes += data.length;
}

// 1. The mod's Packs\*.pak
const langFiles = {};  // path inside currentlang (lowercase) -> {language: {name, data, md5, isNew}}
for (const rel of walk(oldDir)) {
	const full = path.join(oldDir, rel);
	const size = fs.statSync(full).size;
	stats.before += size;
	if (/\.bak$/i.test(rel)) {
		stats.junk++;
		continue;
	}
	const isPak = /\.pak$/i.test(rel);
	const top = rel.split('\\')[0].toLowerCase();
	if (isPak && top === 'packs') {
		for (const [k, v] of readPak(full) || []) {
			if (/\.bak$/i.test(k)) {
				stats.junk++;
				continue;
			}
			const g = gamePacks.get(k);
			if (g && g.md5 === v.md5) {
				stats.same++;
				stats.sameBytes += v.data.length;
				continue;
			}
			keep(v.name, v.data, !g);
		}
		continue;
	}
	if (isPak && top === 'local') {
		const lang = path.basename(rel, path.extname(rel)).toLowerCase();
		const game = gameLocal[lang] || new Map();
		for (const [k, v] of readPak(full) || []) {
			if (/\.bak$/i.test(k)) {
				stats.junk++;
				continue;
			}
			const g = game.get(k);
			if (g && g.md5 === v.md5) {
				stats.same++;
				stats.sameBytes += v.data.length;
				continue;
			}
			const m = v.name.match(/^currentlang\\(.+)$/i);
			if (!m) {
				keep(v.name, v.data, !g);  // outside currentlang: with its path
				continue;
			}
			const inner = m[1].toLowerCase();
			(langFiles[inner] = langFiles[inner] || {})[lang] = {name: m[1], data: v.data, md5: v.md5, isNew: !g};
		}
		continue;
	}
	// Loose file: same as the game's, dropped
	const data = fs.readFileSync(full);
	const gamePath = path.join(gameDir, rel);
	if (fs.existsSync(gamePath) && md5(fs.readFileSync(gamePath)) === md5(data)) {
		stats.same++;
		stats.sameBytes += data.length;
		continue;
	}
	keep(rel, data, !fs.existsSync(gamePath));
}

// 2. What is in currentlang: if it is the same in all the mod's languages, once in currentlang\; if not, per language
const modLangs = walk(oldDir).filter(r => /^local\\[^\\]+\.pak$/i.test(r)).map(r => path.basename(r, '.pak').toLowerCase());
let shared = 0, perLang = 0;
for (const [inner, byLang] of Object.entries(langFiles)) {
	const langs = Object.keys(byLang);
	const first = byLang[langs[0]];
	if (langs.length === modLangs.length && langs.every(l => byLang[l].md5 === first.md5)) {
		keep('currentlang\\' + first.name, first.data, langs.every(l => byLang[l].isNew));
		shared++;
		continue;
	}
	for (const l of langs) {
		keep('Local\\' + l + '\\' + byLang[l].name, byLang[l].data, byLang[l].isNew);
		perLang++;
	}
}

// 3. mod.json
let manifest = {};
const oldManifest = path.join(oldDir, 'mod.json');
if (fs.existsSync(oldManifest)) {
	try {
		manifest = JSON.parse(fs.readFileSync(oldManifest, 'utf8'));
	}
	catch (e) {}
}
delete manifest.autoGenerated;
delete manifest.generadoAutomaticamente;
delete manifest.sources;
delete manifest.fuentes;  // where the original mod is downloaded from: the reduced one is a different mod and is uploaded on its own
if (opt.id) manifest.id = opt.id;
if (opt.nombre) { manifest.name = opt.nombre; delete manifest.nombre; }
if (opt.version) manifest.version = opt.version;
if (opt.categoria) { manifest.category = opt.categoria; delete manifest.categoria; }
if (opt.etiquetas) { manifest.tags = opt.etiquetas.split(',').map(s => s.trim()).filter(Boolean); delete manifest.etiquetas; }
if (!manifest.id) manifest.id = path.basename(path.resolve(oldDir)).toLowerCase().replace(/[^a-z0-9]+/g, '-').replace(/^-|-$/g, '');
fs.writeFileSync(path.join(outDir, 'mod.json'), JSON.stringify(manifest, null, 2) + '\n');

console.log(T(`Mod antiguo: ${mb(stats.before)}`, `Old mod: ${mb(stats.before)}`, `Mod vecchia: ${mb(stats.before)}`));
console.log(T(
	`  idénticos al juego (fuera): ${stats.same} archivos, ${mb(stats.sameBytes)}`,
	`  identical to the game (left out): ${stats.same} files, ${mb(stats.sameBytes)}`,
	`  identici al gioco (esclusi): ${stats.same} file, ${mb(stats.sameBytes)}`));
console.log(T(
	`  copias de seguridad .bak (fuera): ${stats.junk}`,
	`  .bak backup copies (left out): ${stats.junk}`,
	`  copie di backup .bak (escluse): ${stats.junk}`));
console.log(T(
	`Mod nuevo: ${stats.kept} archivos, ${mb(stats.keptBytes)}  (${stats.changed} cambiados, ${stats.added} nuevos)`,
	`New mod: ${stats.kept} files, ${mb(stats.keptBytes)}  (${stats.changed} changed, ${stats.added} new)`,
	`Mod nuova: ${stats.kept} file, ${mb(stats.keptBytes)}  (${stats.changed} modificati, ${stats.added} nuovi)`));
console.log(T(
	`  textos y sonidos de idioma: ${shared} para todos los idiomas, ${perLang} de un idioma (Local\\<idioma>)`,
	`  language texts and sounds: ${shared} for all languages, ${perLang} for a single language (Local\\<language>)`,
	`  testi e suoni delle lingue: ${shared} per tutte le lingue, ${perLang} di una sola lingua (Local\\<lingua>)`));
console.log(T(
	`En ${path.resolve(outDir)}. Revísalo con: node validate-mod.js "${path.resolve(outDir)}"`,
	`In ${path.resolve(outDir)}. Check it with: node validate-mod.js "${path.resolve(outDir)}"`,
	`In ${path.resolve(outDir)}. Controllala con: node validate-mod.js "${path.resolve(outDir)}"`));
