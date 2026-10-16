// Saca los datos del juego (clases, órdenes, scripts, constantes...) a una carpeta, para mirarlos y para las herramientas del SDK que piden la carpeta DATA.
//
//   node extraer-datos.js "<carpeta del juego>" <carpeta de salida> [paquete] [filtro]
//
// Por defecto saca Packs\data.pak: en <salida>\DATA queda lo que piden clonar-civilizacion.js, crear-fortin.js y validar-mod.js --datos.
//   paquete   otro .pak de Packs\ (units.pak, buildings.pak, ui.pak...): se saca entero, o solo lo que case con el filtro
//   filtro    una parte del nombre, sin distinguir mayúsculas ("CLASSES\\R" saca las clases que empiezan por R)
//
// Es de solo lectura sobre el juego. Lo que saca son copias de tus archivos del juego: sirven para estudiarlos o para partir de ellos,
// y no se redistribuyen (publica solo lo que cambias o añades).
const fs = require('fs');
const path = require('path');
const { T } = require('./lib/lang');

const [gameDir, outDir, pakArg = 'data.pak', filter] = process.argv.slice(2);
if (!gameDir || !outDir) {
	console.error(T(
		'uso: node extraer-datos.js "<carpeta del juego>" <carpeta de salida> [paquete] [filtro]',
		'usage: node extraer-datos.js "<game folder>" <output folder> [package] [filter]',
		'uso: node extraer-datos.js "<cartella del gioco>" <cartella di output> [pacchetto] [filtro]'));
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

// Índice: por entrada, longitud del nombre y bytes que comparte con el nombre anterior, el trozo nuevo, y desplazamiento y tamaño
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
