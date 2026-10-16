// Estrae i dati del gioco (classi, ordini, script, costanti...) in una cartella, per consultarli e per gli strumenti dell'SDK che richiedono la cartella DATA.
//
//   node estrai-dati.js "<cartella del gioco>" <cartella di output> [pacchetto] [filtro]
//
// Di default estrae Packs\data.pak: in <output>\DATA resta ciò che richiedono clona-civilta.js, crea-avamposto.js e valida-mod.js --dati.
//   pacchetto un altro .pak di Packs\ (units.pak, buildings.pak, ui.pak...): lo estrae intero, o solo ciò che corrisponde al filtro
//   filtro    una parte del nome, senza distinguere maiuscole/minuscole ("CLASSES\\R" estrae le classi che iniziano per R)
//
// È di sola lettura sul gioco. Ciò che estrae sono copie dei tuoi file del gioco: servono per studiarli o per partire da essi,
// e non si ridistribuiscono (pubblica solo ciò che modifichi o aggiungi).
const fs = require('fs');
const path = require('path');
const { T } = require('./lib/lang');

const [gameDir, outDir, pakArg = 'data.pak', filter] = process.argv.slice(2);
if (!gameDir || !outDir) {
	console.error(T(
		'uso: node estrai-dati.js "<carpeta del juego>" <carpeta de salida> [paquete] [filtro]',
		'usage: node estrai-dati.js "<game folder>" <output folder> [package] [filter]',
		'uso: node estrai-dati.js "<cartella del gioco>" <cartella di output> [pacchetto] [filtro]'));
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

// Indice: per voce, lunghezza del nome e byte che condivide con il nome precedente, la parte nuova, e offset e dimensione
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
