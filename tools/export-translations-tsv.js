// Writes the translation table as a TSV file for tools/test-i18n.cpp: en, es, it per line, with \\ \n \r \t escaped.
//   node tools/export-translations-tsv.js <output.tsv>
const fs = require('fs');
const path = require('path');
const rows = JSON.parse(fs.readFileSync(path.join(__dirname, '..', 'src', 'i18n', 'translations.json'), 'utf8'));
const esc = (s) => s.split('\\').join('\\\\').split('\n').join('\\n').split('\r').join('\\r').split('\t').join('\\t');
fs.writeFileSync(process.argv[2], rows.map((r) => [esc(r.en), esc(r.es ?? '='), esc(r.it ?? '=')].join('\t')).join('\n') + '\n');
console.log(rows.length + ' rows written to ' + process.argv[2]);
