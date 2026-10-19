// Common pieces of create-ability.js, create-formation.js and create-option.js (Imperivm Resurrected SDK).
//
//   - Plan: gathers all the files to be written and writes them all at once at the end. If something fails midway, it undoes what was written:
//     the mod folder never ends up with half-written files.
//   - Reading the game's classes and orders (DATA folder) and the mod's own, to check for repeated names and taken hotkeys.
const fs = require('fs'), path = require('path');

const lower = s => String(s).toLowerCase();
const ci = (dir, name) => { try { return fs.readdirSync(dir).find(f => lower(f) === lower(name)); } catch (e) { return undefined; } };
const readLatin1 = f => fs.readFileSync(f, 'latin1');
const readUtf8 = f => fs.readFileSync(f, 'utf8').replace(/^﻿/, '');
const stripComments = t => t.replace(/<!--[\s\S]*?-->/g, '');
const xmlEsc = s => String(s).replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;').replace(/"/g, '&quot;');

// "Expected" error: the message is printed, with no trace, and it exits with 1
class Failure extends Error {}
const fail = msg => { throw new Failure(msg); };

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
			if (i + 1 >= argv.length) fail('--' + k + ' is missing its value');
			opt[k] = argv[++i];
		}
		else pos.push(a);
	}
	return { pos, opt };
}

// An integer from --option, within a range
function integer(opt, k, min, max) {
	if (opt[k] === undefined) return undefined;
	if (!/^-?\d+$/.test(String(opt[k]))) fail('--' + k + ' must be an integer (you gave "' + opt[k] + '")');
	const n = Number(opt[k]);
	if (n < min || n > max) fail('--' + k + ' must be between ' + min + ' and ' + max + ' (you gave ' + n + ')');
	return n;
}

// Checks that the options given belong to the tool
function onlyKnown(opt, known) {
	for (const k of Object.keys(opt)) if (!known.includes(k)) {
		const list = known.map(c => '--' + c).join(', ');
		fail('unknown option: --' + k + ' (available: ' + list + ')');
	}
}

function dataFolder(dataDir) {
	if (!dataDir || !fs.existsSync(dataDir) || !fs.statSync(dataDir).isDirectory()) fail('the DATA folder does not exist: ' + dataDir + '\n  extract-data.js extracts it (see the SDK README).');
	for (const n of ['classes', 'commands']) if (!ci(dataDir, n)) fail(dataDir + ' does not look like the game\'s DATA folder (it has no "' + n + '").\n  extract-data.js extracts it (see the SDK README).');
	return dataDir;
}

// --- Write plan ----------------------------------------------------------------------------------------------------------
class Plan {
	constructor() { this.items = new Map(); }  // filePath -> { text, fresh }
	// Reads the current text of a mod file, or the one this plan already holds for it
	text(filePath) {
		if (this.items.has(filePath)) return this.items.get(filePath).text;
		return fs.existsSync(filePath) ? readUtf8(filePath) : null;
	}
	exists(filePath) { return this.items.has(filePath) || fs.existsSync(filePath); }
	put(filePath, text) {
		const previous = this.items.get(filePath);
		this.items.set(filePath, { text, fresh: previous ? previous.fresh : !fs.existsSync(filePath) });
	}
	// Writes everything. If something fails, it leaves the folder as it was
	write() {
		const done = [];  // { filePath, before: Buffer|null, folders: [] }
		try {
			for (const [filePath, it] of this.items) {
				const folders = [];
				let d = path.dirname(filePath);
				while (!fs.existsSync(d)) { folders.unshift(d); d = path.dirname(d); }
				const before = fs.existsSync(filePath) ? fs.readFileSync(filePath) : null;
				fs.mkdirSync(path.dirname(filePath), { recursive: true });
				done.push({ filePath, before, folders });
				fs.writeFileSync(filePath, it.text, 'utf8');
			}
		}
		catch (e) {
			for (const h of done.reverse()) {
				try { if (h.before) fs.writeFileSync(h.path, h.before); else fs.rmSync(h.path, { force: true }); } catch (e2) { /* nothing more to do */ }
				for (const c of h.folders.slice().reverse()) { try { fs.rmdirSync(c); } catch (e2) { /* not empty */ } }
			}
			fail('could not write the files (' + e.message + '). Nothing was left half-written.');
		}
	}
}

// Appends text right before the last closing tag of an XML file that already exists (</patch>, </commands>...)
function beforeClosing(text, closing, fresh, filePath) {
	const i = text.lastIndexOf(closing);
	if (i < 0) fail(filePath + ' already exists and has no ' + closing + ': cannot tell where to add. Fix it or delete it and run again.');
	return text.slice(0, i).replace(/[ \t]*$/, '') + fresh + text.slice(i);
}

// --- Game data -----------------------------------------------------------------------------------------------------------
// Classes: id (and altid) in lowercase -> { id, parent, file (file name), mod }
function readClasses(dataDir, modDir) {
	const map = new Map();
	const add = (file, text, fromMod) => {
		const h = text.match(/<class\b[^>]*>/);
		if (!h) return;
		const a = attrs(h[0]);
		if (!a.id) return;
		const info = { id: a.id, parent: a.parent || '', file, mod: fromMod };
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
function lineage(classes, id) {
	const out = [];
	let c = classes.get(lower(id));
	let n = 0;
	if (!c) return [lower(id)];
	while (c && n++ < 40) {
		out.push(lower(c.id));
		c = c.parent ? classes.get(lower(c.parent)) : null;
	}
	return out;
}
const related = (classes, a, b) => lineage(classes, a).includes(lower(b)) || lineage(classes, b).includes(lower(a));

// Orders: [{ name, key, srcs: [classes], file, mod, menu }] from DATA\commands and from the mod's data\commands and patches\data\commands
function readCommands(dataDir, modDir) {
	const out = [];
	const read = (file, text, fromMod) => {
		for (const m of stripComments(text).matchAll(/<cmd\b([^>]*)>([\s\S]*?)<\/cmd>/g)) {
			const a = attrs(m[1]);
			if (!a.name) continue;
			out.push({ name: a.name, key: a.key || '', srcs: [...m[2].matchAll(/<src\b([^>]*)>/g)].map(s => attrs(s[1]).obj).filter(Boolean), file, mod: fromMod, menu: a.menu === '1', param: a.param, button: a.button });
		}
	};
	const cd = path.join(dataDir, ci(dataDir, 'commands'));
	for (const f of fs.readdirSync(cd)) if (/\.xml$/i.test(f)) read(f, readLatin1(path.join(cd, f)), false);
	for (const sub of [['data', 'commands'], ['patches', 'data', 'commands']]) {
		const d = path.join(modDir, ...sub);
		if (fs.existsSync(d)) for (const f of fs.readdirSync(d)) if (/\.xml$/i.test(f)) read(sub.join('/') + '/' + f, readUtf8(path.join(d, f)), true);
	}
	return out;
}

// Orders that already use the hotkey in a class related to the one that will receive it (the same, an ancestor or a child)
function keyClashes(commands, classes, key, className, ignore = []) {
	return commands.filter(o => lower(o.key) === lower(key) && !ignore.includes(o.name) && o.srcs.some(s => related(classes, s, className)));
}

// --- minimal mod.json, if the folder does not have one ---------------------------------------------------------------------
function minimalModJson(plan, modDir, category, description) {
	const filePath = path.join(modDir, 'mod.json');
	if (plan.exists(filePath)) return false;
	const id = lower(path.basename(path.resolve(modDir))).replace(/[^a-z0-9_-]+/g, '-').replace(/^-+|-+$/g, '') || 'my-mod';
	const j = { id, name: path.basename(path.resolve(modDir)), version: '1.0', category: category, tags: [], authors: ['Me'], description: description, redistributable: true };
	plan.put(filePath, JSON.stringify(j, null, 2) + '\n');
	return true;
}

// Runs a tool: expected failures come out as a single error line, with no trace
function run(fn) {
	try { fn(); }
	catch (e) {
		if (e instanceof Failure) { console.error('error: ' + e.message); process.exit(1); }
		throw e;
	}
}

module.exports = { fs, path, lower, ci, readLatin1, readUtf8, stripComments, xmlEsc, attrs, Failure, fail, parseArgs, integer, onlyKnown, dataFolder, Plan,
	beforeClosing, readClasses, lineage, related, readCommands, keyClashes, minimalModJson, run };
