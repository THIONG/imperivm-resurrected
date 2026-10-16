// Extracts the game data (classes, orders, scripts, constants...) to a folder, to look at it and for the SDK tools that ask for the DATA folder.
//
//   node extract-data.js "<game folder>" <output folder> [package] [filter]
//
// By default it extracts Packs\data.pak: <output>\DATA ends up with what clone-civilization.js, create-outpost.js and validate-mod.js --data ask for.
//   package   another .pak from Packs\ (units.pak, buildings.pak, ui.pak...): extracted whole, or only what matches the filter
//   filter    part of a name, case-insensitive ("CLASSES\\R" extracts the classes that start with R)
//
// It is read-only on the game. What it extracts are copies of your game files: they are for studying or as a starting point,
// and are not to be redistributed (publish only what you change or add).
const fs = require('fs');
const path = require('path');
const { T } = require('./lib/lang');

const [gameDir, outDir, pakArg = 'data.pak', filter] = process.argv.slice(2);
if (!gameDir || !outDir) {
	console.error(T(
		'uso: node extract-data.js "<carpeta del juego>" <carpeta de salida> [paquete] [filtro]',
		'usage: node extract-data.js "<game folder>" <output folder> [package] [filter]',
		'uso: node extract-data.js "<cartella del gioco>" <cartella di output> [pacchetto] [filtro]'));
	process.exit(1);
}
const pakName = /\.pak$/i.test(pakArg) ? pakArg : pakArg + '.pak';
const pakFile = fs.readdirSync(path.join(gameDir, 'Packs')).find(f => f.toLowerCase() === pakName.toLowerCase());
if (!pakFile) {
	console.error(T(`no encuentro Packs\\${pakName} en ${gameDir}`, `cannot find Packs\\${pakName} in ${gameDir}`, `non trovo Packs\\${pakName} in ${gameDir}`));
	process.exit(1);
}
const data = fs.readFileSync(path.join(gameDir, 'Packs', pakFile));
if (data.toString('latin1', 0, 15) !== 'HMMSYS PackFile') {
	console.error(T('no parece un paquete del juego (HMMSYS PackFile)', 'does not look like a game package (HMMSYS PackFile)', 'non sembra un pacchetto del gioco (HMMSYS PackFile)'));
	process.exit(1);
}
const count = data.readUInt32LE(0x20);
const re = filter ? new RegExp(filter.replace(/[.*+?^${}()|[\]\\]/g, c => (c === '\\' ? '\\\\' : '\\' + c)), 'i') : null;

// Index: per entry, name length and bytes shared with the previous name, the new part, and offset and size
let p = 0x28, prev = '', written = 0, bytes = 0;
for (let i = 0; i < count; i++) {
	const len = data[p], shared = data[p + 1];
	p += 2;
	const name = prev.slice(0, shared) + data.toString('latin1', p, p + len - shared);
	p += len - shared;
	const offset = data.readUInt32LE(p), size = data.readUInt32LE(p + 4);
	p += 8;
	prev = name;
	if (re && !re.test(name)) continue;
	const dest = path.join(outDir, ...name.split('\\'));
	fs.mkdirSync(path.dirname(dest), { recursive: true });
	fs.writeFileSync(dest, data.subarray(offset, offset + size));
	written++;
	bytes += size;
}
console.log(T(
	`${written} archivos (${(bytes / 1048576).toFixed(1)} MB) de ${pakFile} en ${outDir}`,
	`${written} files (${(bytes / 1048576).toFixed(1)} MB) from ${pakFile} in ${outDir}`,
	`${written} file (${(bytes / 1048576).toFixed(1)} MB) da ${pakFile} in ${outDir}`));
