// Convierte un mod "a la antigua" (paquetes .pak enteros y copias de archivos del juego, que se instalaba copiándolo encima del
// juego) en un mod de Imperivm Resurrected que solo lleva lo que cambia o añade. Lo que es idéntico al juego se queda fuera:
// cada jugador ya lo tiene.
//
//   node reducir-mod.js <mod antiguo> <carpeta del juego> <carpeta de salida> [opciones]
//     --id <id>  --nombre <nombre>  --version <versión>  --categoria <categoría>  --etiquetas a,b,c
//     --fuera <patrones>   deja fuera esos archivos del mod antiguo (rutas con * y ?, separadas por comas)
//     --ocultar <patrones> archivos sueltos del juego que el léeme del mod antiguo mandaba borrar (van en hidden.txt)
//     --rutas              solo enseña qué carpetas hay dentro de los paquetes (no escribe nada)
//
// Primero calcula qué ve el juego con el mod copiado encima (sus paquetes sustituyen a los del juego con el mismo nombre;
// en una carpeta, el paquete con el nombre mayor gana, como zzMod.pak sobre Data.pak; un archivo suelto gana a los paquetes)
// y qué ve sin el mod. Luego guarda solo lo que cambia (lo que el mod antiguo quitaba del juego va en hidden.txt):
//   archivos de los paquetes      con su ruta interna (data\..., ui\..., sounds\...)
//   local\<idioma>.pak            lo de currentlang\ va a Local\<idioma>\ (ese idioma) o a currentlang\ (si es igual en todos);
//                                 lo demás, a Local\<idioma>.pak
//   data\const.ini                como parche (patches\data\const.ini), sin la sección [Resolutions]: la resolución la elige el jugador
//   rle.mmp                       lo que cambia o añade al del juego, detrás de él, en rle.mmp.tail (sus imágenes se mueven)
//   otros archivos sueltos        (Scenarios, Adventures, Movies...) si cambian o son nuevos
// Fuera: *.bak, Config.ini y Settings.ini (configuración de la instalación), y los ejecutables y scripts del sistema (con ellos
// el mod no se cargaría). Al final dice qué ha quedado fuera.
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
		'uso: node reducir-mod.js <mod antiguo> <carpeta del juego> <carpeta de salida> [--id --nombre --version --categoria --etiquetas --fuera] [--rutas]',
		'usage: node reducir-mod.js <old mod> <game folder> <output folder> [--id --nombre --version --categoria --etiquetas --fuera] [--rutas]',
		'uso: node reducir-mod.js <mod vecchia> <cartella del gioco> <cartella di output> [--id --nombre --version --categoria --etiquetas --fuera] [--rutas]'));
	process.exit(1);
}

const FORBIDDEN = ['.exe', '.dll', '.asi', '.bat', '.cmd', '.com', '.scr', '.ps1', '.vbs', '.js', '.msi', '.sys', '.cpl', '.lnk', '.reg', '.jar', '.hta', '.wsf', '.pif'];
const INSTALL_FILES = ['config.ini', 'settings.ini'];
const md5 = buf => crypto.createHash('md5').update(buf).digest('hex');
const mb = n => (n / 1048576).toFixed(1) + ' MB';
const norm = p => p.toLowerCase().split('\\').join('/');

// --- Paquetes .pak ("HMMSYS PackFile": índice con nombres comprimidos por prefijo; datos sin comprimir) ---
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
		out.set(norm(name), {name, data, md5: md5(data)});
	}
	return out;
}
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
function pakFiles(dir) {
	if (!fs.existsSync(dir)) return [];
	return fs.readdirSync(dir).filter(f => /\.pak$/i.test(f)).map(f => ({name: f, low: f.toLowerCase(), full: path.join(dir, f)}));
}
// Como el juego: los paquetes de una carpeta, del nombre mayor al menor; el primero que tiene un archivo es el que vale
const pakCache = new Map();
function resolve(paks) {
	const out = new Map();
	for (const p of [...paks].sort((a, b) => (a.low < b.low ? 1 : a.low > b.low ? -1 : 0))) {
		if (!pakCache.has(p.full)) pakCache.set(p.full, readPak(p.full) || new Map());
		for (const [k, v] of pakCache.get(p.full)) if (!out.has(k)) out.set(k, Object.assign({fromMod: p.fromMod === true}, v));
	}
	return out;
}
function withReplacements(gamePaks, modPaks) {
	const byName = new Map(gamePaks.map(p => [p.low, p]));
	for (const p of modPaks) byName.set(p.low, p);
	return [...byName.values()];
}

// --- Imágenes ("IMGRLE"): las celdas "pamm" guardan en qué byte de rle.mmp están sus píxeles ---
const isSprite = name => /\.rle(\.mmp)?$/i.test(name);
function pammCells(buf) {
	if (buf.length < 26 || buf.toString('latin1', 0, 6) !== 'IMGRLE') return null;
	try {
		const cols = buf.readUInt32LE(18), rows = buf.readUInt32LE(22);
		const out = [];
		let p = 26;
		for (let i = 0; i < cols * rows; i++) {
			const x0 = buf.readInt32LE(p), y0 = buf.readInt32LE(p + 4), x1 = buf.readInt32LE(p + 8), y1 = buf.readInt32LE(p + 12);
			p += 16;
			if (x0 > x1 || y0 > y1) continue;
			if (buf.toString('latin1', p, p + 4) !== 'RLE2') return false;
			const size = buf.readUInt32LE(p + 14);
			if (buf.toString('latin1', p + 30, p + 34) === 'pamm') {
				out.push({at: p + 34, off: buf.readUInt32LE(p + 34), size});
				p += 38;
			}
			else p += 30 + size;
		}
		return out;
	}
	catch (e) {
		return false;
	}
}
// Si el mod cambia el rle.mmp del juego desde el byte "from", lo suyo va detrás del del juego: sus imágenes se mueven "delta" bytes
let rleMove = null;
function moved(name, data) {
	if (!rleMove || !isSprite(name)) return data;
	const cells = pammCells(data);
	const hits = cells ? cells.filter(c => c.off >= rleMove.from) : [];
	if (!hits.length) return data;
	const out = Buffer.from(data);
	for (const c of hits) out.writeUInt32LE(c.off + rleMove.delta, c.at);
	rleMove.files++;
	rleMove.cells += hits.length;
	return out;
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

const excludePatterns = (opt.fuera || '').split(',').map(s => s.trim()).filter(Boolean)
	.map(s => new RegExp('^' + norm(s).replace(/[.+^${}()|[\]]/g, '\\$&').replace(/\*/g, '.*').replace(/\?/g, '.') + '$'));
const oldFiles = walk(oldDir);
const top = rel => rel.split('\\')[0].toLowerCase();
const folderOf = name => (oldFiles.find(r => top(r) === name) || name).split('\\')[0];
const fromMod = list => list.map(p => Object.assign(p, {fromMod: true}));
const oldPacks = fromMod(pakFiles(path.join(oldDir, folderOf('packs'))));
const oldLocals = fromMod(pakFiles(path.join(oldDir, folderOf('local'))));
const gamePacks = pakFiles(path.join(gameDir, 'Packs'));
const gameLocals = pakFiles(path.join(gameDir, 'local'));

// 1. Lo que ve el juego sin el mod y con el mod copiado encima
const before = resolve(gamePacks);
const after = resolve(withReplacements(gamePacks, oldPacks));
const langOf = p => p.low.replace(/\.pak$/, '');
const modLangs = [...new Set(oldLocals.map(p => langOf(p).split('_')[0]))];
const beforeLang = {}, afterLang = {};
for (const lang of modLangs) {
	const starts = p => p.low.startsWith(lang);
	beforeLang[lang] = resolve(gameLocals.filter(starts));
	afterLang[lang] = resolve(withReplacements(gameLocals.filter(starts), oldLocals.filter(starts)));
}

fs.mkdirSync(outDir, {recursive: true});
function write(rel, data) {
	const full = path.join(outDir, rel);
	fs.mkdirSync(path.dirname(full), {recursive: true});
	fs.writeFileSync(full, moved(rel, data));
}
const stats = {before: 0, same: 0, sameBytes: 0, changed: 0, added: 0, kept: 0, keptBytes: 0, junk: 0, removed: {}, removedLang: {}};
const leftOut = [];
function keep(rel, data, isNew) {
	write(rel, data);
	stats[isNew ? 'added' : 'changed']++;
	stats.kept++;
	stats.keptBytes += data.length;
}
for (const rel of oldFiles) stats.before += fs.statSync(path.join(oldDir, rel)).size;

// 2. Los sueltos del mod antiguo: ganan a los paquetes
const loose = new Map();
for (const rel of oldFiles) {
	const t = top(rel);
	if ((t === 'packs' || t === 'local') && /\.pak$/i.test(rel)) continue;
	const n = norm(rel);
	const ext = path.extname(rel).toLowerCase();
	if (ext === '.bak') {
		stats.junk++;
		continue;
	}
	if (FORBIDDEN.includes(ext)) {
		leftOut.push([rel, T('ejecutable o script del sistema: con él el mod no se cargaría', 'executable or system script: with it the mod would not load', 'eseguibile o script di sistema: con esso la mod non si caricherebbe')]);
		continue;
	}
	if (INSTALL_FILES.includes(n)) {
		leftOut.push([rel, T('configuración de la instalación: la llevan el juego y sus opciones', 'install settings: the game and its options handle them', 'configurazione dell\'installazione: la gestiscono il gioco e le sue opzioni')]);
		continue;
	}
	if (excludePatterns.some(re => re.test(n))) {
		leftOut.push([rel, T('fuera por --fuera', 'left out with --fuera', 'escluso con --fuera')]);
		continue;
	}
	loose.set(n, rel);
}

// 3. rle.mmp: el del juego se queda entero y lo que el mod cambia o añade va detrás, en rle.mmp.tail. Las imágenes del mod que
// apuntan a lo que cambia se mueven a su sitio nuevo al escribirlas; las del juego siguen apuntando a lo suyo
let rleKept = null;
function rleMoveProblem(from, modSize, gameSize) {
	if (gameSize + modSize - from > 0xffffffff) {
		return T('con lo del mod detrás, el rle.mmp pasaría de 4 GB', 'with the mod\'s bytes after it, rle.mmp would go past 4 GB', 'con i byte della mod in fondo, rle.mmp supererebbe i 4 GB');
	}
	const sprites = [];
	for (const [n, rel] of loose) if (isSprite(n)) sprites.push([rel, fs.readFileSync(path.join(oldDir, rel))]);
	for (const view of [after, ...Object.values(afterLang)]) {
		for (const [k, v] of view) if (v.fromMod && isSprite(k) && !loose.has(k)) sprites.push([v.name, v.data]);
	}
	for (const [name, data] of sprites) {
		const cells = pammCells(data);
		if (cells === false) {
			return T(`${name} está dañado: no se sabe a qué parte del rle.mmp apunta`, `${name} is damaged: it is not known which part of rle.mmp it points to`,
				`${name} è danneggiato: non si sa a quale parte di rle.mmp punta`);
		}
		if (cells && cells.some(c => c.off < from && c.off + c.size > from)) {
			return T(`${name} tiene una imagen que empieza en la parte del juego y acaba en la del mod`, `${name} has an image that starts in the game's part and ends in the mod's`,
				`${name} ha un'immagine che inizia nella parte del gioco e finisce in quella della mod`);
		}
	}
	return '';
}
if (loose.has('rle.mmp')) {
	const rel = loose.get('rle.mmp');
	loose.delete('rle.mmp');
	const mine = path.join(oldDir, rel), theirs = path.join(gameDir, 'rle.mmp');
	const a = fs.openSync(mine, 'r'), b = fs.openSync(theirs, 'r');
	const sa = fs.fstatSync(a).size, sb = fs.fstatSync(b).size;
	const bufA = Buffer.alloc(1 << 22), bufB = Buffer.alloc(1 << 22);
	const sha = crypto.createHash('sha256');
	let from = -1;
	for (let off = 0; off < sb; off += bufB.length) {
		const n = Math.min(bufB.length, sb - off);
		fs.readSync(b, bufB, 0, n, off);
		sha.update(bufB.subarray(0, n));
		if (from >= 0) continue;
		const m = Math.max(0, Math.min(n, sa - off));
		fs.readSync(a, bufA, 0, m, off);
		if (m === n && bufA.compare(bufB, 0, n, 0, n) === 0) continue;
		let i = 0;
		while (i < m && bufA[i] === bufB[i]) i++;
		from = off + i;
	}
	if (from < 0) from = sb;
	const problem = from < sb ? rleMoveProblem(from, sa, sb) : '';
	if (from === sb && sa === sb) {
		stats.same++;
		stats.sameBytes += sa;
	}
	else if (problem) {
		leftOut.push([rel, problem]);
	}
	else {
		if (from < sb) rleMove = {from, delta: sb - from, files: 0, cells: 0};
		const head = Buffer.alloc(56);
		head.write('IMPRTAIL', 0, 'latin1');
		head.writeUInt32LE(1, 8);
		head.writeUInt32LE(56, 12);
		head.writeBigUInt64LE(BigInt(sb), 16);
		Buffer.from(sha.digest('hex'), 'hex').copy(head, 24);
		const out = fs.openSync(path.join(outDir, 'rle.mmp.tail'), 'w');
		fs.writeSync(out, head);
		for (let off = from; off < sa; off += bufA.length) {
			const n = Math.min(bufA.length, sa - off);
			fs.readSync(a, bufA, 0, n, off);
			fs.writeSync(out, bufA, 0, n);
		}
		fs.closeSync(out);
		stats.added++;
		stats.kept++;
		stats.keptBytes += 56 + sa - from;
		rleKept = {own: sa - from};
	}
	fs.closeSync(a);
	fs.closeSync(b);
}

// 4. data\const.ini: parche sin [Resolutions]
function iniSections(text) {
	const list = [];
	const byName = new Map();
	let cur = null;
	for (const raw of text.replace(/\r/g, '').split('\n')) {
		const line = raw.trim();
		if (!line || line.startsWith(';') || line.startsWith('#')) continue;
		const s = /^\[([^\]]+)\]/.exec(line);
		if (s) {
			const name = s[1].trim();
			cur = byName.get(name.toLowerCase());
			if (!cur) {
				cur = {name, entries: []};
				list.push(cur);
				byName.set(name.toLowerCase(), cur);
			}
			continue;
		}
		if (!cur) continue;
		const eq = line.indexOf('=');
		cur.entries.push(eq < 0 ? {bare: line} : {key: line.slice(0, eq).trim(), value: line.slice(eq + 1).trim()});
	}
	return {list, byName};
}
function iniPatch(baseText, newText, skipSections) {
	const base = iniSections(baseText), next = iniSections(newText);
	const out = [];
	for (const s of next.list) {
		if (skipSections.includes(s.name.toLowerCase())) continue;
		const b = base.byName.get(s.name.toLowerCase());
		const lines = [];
		const keysB = new Map(), keysN = new Set();
		if (b) for (const e of b.entries) if (e.key && !keysB.has(e.key.toLowerCase())) keysB.set(e.key.toLowerCase(), e);
		const bareB = new Set(b ? b.entries.filter(e => e.bare).map(e => e.bare.toLowerCase()) : []);
		const bareN = new Set(s.entries.filter(e => e.bare).map(e => e.bare.toLowerCase()));
		for (const e of s.entries) {
			if (e.key) {
				if (keysN.has(e.key.toLowerCase())) continue;
				keysN.add(e.key.toLowerCase());
				const old = keysB.get(e.key.toLowerCase());
				if (!old || old.value !== e.value) lines.push(e.key + ' = ' + e.value);
			}
			else if (!bareB.has(e.bare.toLowerCase())) lines.push('+' + e.bare);
		}
		if (b) {
			for (const [k, e] of keysB) if (!keysN.has(k)) lines.push('-' + e.key);
			for (const e of b.entries) if (e.bare && !bareN.has(e.bare.toLowerCase())) lines.push('-' + e.bare);
		}
		if (lines.length) out.push('[' + s.name + ']', ...lines, '');
	}
	return out.join('\r\n');
}
const finalConst = loose.has('data/const.ini') ? fs.readFileSync(path.join(oldDir, loose.get('data/const.ini'))) : (after.get('data/const.ini') || {}).data;
const gameConst = (before.get('data/const.ini') || {}).data;
let constPatched = false;
if (finalConst && gameConst && md5(finalConst) !== md5(gameConst)) {
	const patch = iniPatch(gameConst.toString('latin1'), finalConst.toString('latin1'), ['resolutions']);
	if (patch.trim()) {
		write(path.join('patches', 'data', 'const.ini'), Buffer.from(patch, 'latin1'));
		stats.changed++;
		stats.kept++;
		stats.keptBytes += patch.length;
	}
	constPatched = true;
}
loose.delete('data/const.ini');

// 5. Los archivos de los paquetes que cambian. Si el paquete de un idioma tiene la misma ruta, en ese idioma gana el suyo: el
// archivo va solo a Local\<idioma>.pak de los idiomas que no la tienen
const allLangs = [...new Set([...modLangs, ...gameLocals.map(p => langOf(p).split('_')[0])])];
const langViews = {};
const langView = lang => langViews[lang] || (langViews[lang] = afterLang[lang] || resolve(gameLocals.filter(p => p.low.startsWith(lang))));
const langPaks = {};
for (const [k, v] of after) {
	if (!v.fromMod || k === 'data/const.ini' || loose.has(k)) continue;
	if (/\.bak$/.test(k)) {
		stats.junk++;
		continue;
	}
	const g = before.get(k);
	if (g && g.md5 === v.md5) {
		stats.same++;
		stats.sameBytes += v.data.length;
		continue;
	}
	const covered = allLangs.filter(l => langView(l).has(k));
	if (!covered.length) {
		keep(v.name, v.data, !g);
		continue;
	}
	for (const l of allLangs) {
		if (!covered.includes(l)) (langPaks[l] = langPaks[l] || []).push(v);
	}
}
// 6. Idiomas: lo de currentlang\ igual en todos los idiomas del mod va a currentlang\, el resto a Local\<idioma>\; lo que va
// fuera de currentlang\ en un paquete de idioma, a Local\<idioma>.pak (el juego solo lo monta con ese idioma)
const langFiles = {};
for (const lang of modLangs) {
	for (const [k, v] of afterLang[lang]) {
		if (!v.fromMod || /\.bak$/.test(k) || loose.has(k)) continue;
		const g = beforeLang[lang].get(k);
		if (g && g.md5 === v.md5) {
			stats.same++;
			stats.sameBytes += v.data.length;
			continue;
		}
		const m = v.name.match(/^currentlang\\(.+)$/i);
		if (!m) {
			(langPaks[lang] = langPaks[lang] || []).push(v);
			continue;
		}
		const inner = m[1].toLowerCase();
		(langFiles[inner] = langFiles[inner] || {})[lang] = {name: m[1], data: v.data, md5: v.md5, isNew: !g};
	}
}
function writePak(rel, entries) {
	const list = entries.map(e => ({name: e.name.toUpperCase().split('/').join('\\'), data: moved(e.name, e.data)}))
		.sort((a, b) => (a.name < b.name ? -1 : a.name > b.name ? 1 : 0));
	let prev = '', indexSize = 0;
	const recs = list.map(e => {
		let sh = 0;
		while (sh < prev.length && sh < e.name.length && sh < 255 && prev[sh] === e.name[sh]) sh++;
		prev = e.name;
		const suffix = Buffer.from(e.name.slice(sh), 'latin1');
		indexSize += 2 + suffix.length + 8;
		return {len: e.name.length, sh, suffix, data: e.data};
	});
	const header = Buffer.alloc(0x28);
	header.write('HMMSYS PackFile\n', 0, 'latin1');
	header[16] = 0x1a;
	header.writeUInt32LE(list.length, 0x20);
	header.writeUInt32LE(indexSize, 0x24);
	const index = Buffer.alloc(indexSize);
	let p = 0, off = 0x28 + indexSize + 4 * list.length;
	for (const r of recs) {
		index[p++] = r.len;
		index[p++] = r.sh;
		r.suffix.copy(index, p);
		p += r.suffix.length;
		index.writeUInt32LE(off, p);
		index.writeUInt32LE(r.data.length, p + 4);
		p += 8;
		off += r.data.length;
	}
	write(rel, Buffer.concat([header, index, Buffer.alloc(4 * list.length), ...recs.map(r => r.data)]));
}
for (const [lang, entries] of Object.entries(langPaks)) {
	writePak(path.join('Local', lang + '.pak'), entries);
	stats.changed++;
	stats.kept++;
	stats.keptBytes += entries.reduce((n, e) => n + e.data.length, 0);
}

// 7. Lo que el mod antiguo quitaba del juego (al sustituir sus paquetes, o porque su léeme mandaba borrarlo: --ocultar) va en
// hidden.txt: lo que falta en todos los idiomas, sin sección; lo que falta solo en uno, en la sección [idioma]
const hidePatterns = (opt.ocultar || '').split(',').map(s => s.trim()).filter(Boolean)
	.map(s => new RegExp('^' + norm(s).replace(/[.+^${}()|[\]]/g, '\\$&').replace(/\*/g, '.*').replace(/\?/g, '.') + '$'));
const notGame = /^(mods|mod|mod-dump|logs|profiles|hof|gamedev|xtras)\//;
const gameLoose = new Set(walk(gameDir).map(norm).filter(n => !notGame.test(n) && !/\.(pak|exe|dll)$/.test(n)));
const keptLoose = new Set([...gameLoose].filter(n => !hidePatterns.some(re => re.test(n))));
for (const rel of oldFiles) {
	const n = norm(rel);
	if (!/^(packs|local)\/[^/]+\.pak$/.test(n)) keptLoose.add(n);
}
function removedIn(beforeView, afterView) {
	const out = new Set();
	for (const k of new Set([...beforeView, ...gameLoose])) if (!afterView.has(k) && !keptLoose.has(k)) out.add(k);
	return out;
}
const removedBase = removedIn(before.keys(), after);
const removedByLang = {};
for (const lang of modLangs) {
	const afterAll = new Set([...after.keys(), ...afterLang[lang].keys()]);
	removedByLang[lang] = removedIn([...before.keys(), ...beforeLang[lang].keys()], afterAll);
}
const hiddenAll = [...removedBase].filter(k => modLangs.every(l => removedByLang[l].has(k))).sort();
const hiddenSet = new Set(hiddenAll);
for (const k of hiddenAll) stats.removed[k.split('/')[0]] = (stats.removed[k.split('/')[0]] || 0) + 1;
const sections = [];
for (const lang of modLangs) {
	const own = [...removedByLang[lang]].filter(k => !hiddenSet.has(k)).sort();
	if (!own.length) continue;
	stats.removedLang[lang] = own.length;
	sections.push('', '[' + lang + ']', ...own);
}
if (hiddenAll.length || sections.length) {
	const head = T('; Archivos del juego que este mod oculta: el mod antiguo los quitaba al sustituir los paquetes del juego',
		'; Game files this mod hides: the old mod removed them by replacing the game\'s packs',
		'; File del gioco che questa mod nasconde: la mod vecchia li toglieva sostituendo i pacchetti del gioco');
	write('hidden.txt', Buffer.from([head, ...hiddenAll, ...sections].join('\r\n') + '\r\n', 'latin1'));
}
let shared = 0, perLang = 0;
for (const byLang of Object.values(langFiles)) {
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

// 8. Los demás sueltos: fuera si son iguales a lo que ya ve el juego en todos los idiomas
const shownBefore = lang => beforeLang[lang] || langView(lang);
for (const [n, rel] of loose) {
	const data = fs.readFileSync(path.join(oldDir, rel));
	const gamePath = path.join(gameDir, rel);
	const h = md5(data);
	const same = fs.existsSync(gamePath) ? md5(fs.readFileSync(gamePath)) === h :
		before.has(n) && before.get(n).md5 === h && allLangs.every(l => !shownBefore(l).has(n) || shownBefore(l).get(n).md5 === h);
	if (same) {
		stats.same++;
		stats.sameBytes += data.length;
		continue;
	}
	keep(rel, data, !fs.existsSync(gamePath) && !before.has(n));
}

// 9. mod.json
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
delete manifest.fuentes;  // de dónde se baja el mod original: el reducido es otro mod y se sube por su cuenta
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
if (constPatched) console.log(T(
	'  data\\const.ini: como parche, sin [Resolutions] (la resolución la elige el jugador en las opciones)',
	'  data\\const.ini: as a patch, without [Resolutions] (the player picks the resolution in the options)',
	'  data\\const.ini: come patch, senza [Resolutions] (la risoluzione la sceglie il giocatore nelle opzioni)'));
if (rleKept && !rleMove) console.log(T(
	`  rle.mmp: lo añadido al del juego (${mb(rleKept.own)}), en rle.mmp.tail`,
	`  rle.mmp: what is appended to the game's (${mb(rleKept.own)}), in rle.mmp.tail`,
	`  rle.mmp: ciò che è aggiunto a quello del gioco (${mb(rleKept.own)}), in rle.mmp.tail`));
else if (rleKept) console.log(T(
	`  rle.mmp: lo que el mod cambia o añade (${mb(rleKept.own)}), detrás del del juego, en rle.mmp.tail; ${rleMove.files} imágenes del mod movidas a su sitio nuevo`,
	`  rle.mmp: what the mod changes or adds (${mb(rleKept.own)}), after the game's, in rle.mmp.tail; ${rleMove.files} images of the mod moved to their new place`,
	`  rle.mmp: ciò che la mod cambia o aggiunge (${mb(rleKept.own)}), dopo quello del gioco, in rle.mmp.tail; ${rleMove.files} immagini della mod spostate al loro nuovo posto`));
if (leftOut.length) {
	console.log(T('Fuera:', 'Left out:', 'Esclusi:'));
	for (const [rel, why] of leftOut) console.log(`  ${rel}: ${why}`);
}
const removed = Object.entries(stats.removed);
const removedLang = Object.entries(stats.removedLang);
if (removed.length || removedLang.length) {
	console.log(T(
		`El mod antiguo quitaba archivos del juego: van en hidden.txt para que el juego no los vea (${hiddenAll.length} en todos los idiomas)`,
		`The old mod removed game files: they go in hidden.txt so that the game does not see them (${hiddenAll.length} in every language)`,
		`La mod vecchia toglieva file del gioco: vanno in hidden.txt perché il gioco non li veda (${hiddenAll.length} in tutte le lingue)`));
	for (const [where, n] of removed) console.log(`  ${where}\\: ${n}`);
	for (const [lang, n] of removedLang) console.log(T(`  solo en ${lang}: ${n}`, `  only in ${lang}: ${n}`, `  solo in ${lang}: ${n}`));
}
if (Object.keys(langPaks).length) {
	console.log(T('  archivos de un idioma fuera de currentlang\\: en Local\\<idioma>.pak', '  language files outside currentlang\\: in Local\\<language>.pak',
		'  file di una lingua fuori da currentlang\\: in Local\\<lingua>.pak'));
}
console.log(T(
	`En ${path.resolve(outDir)}. Revísalo con: node validar-mod.js "${path.resolve(outDir)}"`,
	`In ${path.resolve(outDir)}. Check it with: node validar-mod.js "${path.resolve(outDir)}"`,
	`In ${path.resolve(outDir)}. Controllala con: node validar-mod.js "${path.resolve(outDir)}"`));
