// Generates dll/src/i18n/tr_data.cpp (the UI translation table) from dll/src/i18n/translations.json.
//
//   node dll/tools/gen-tr.js
//
// translations.json is a list of {"en": "...", "es": "...", "it": "..."}. "en" is the text used in the code (the key of Tr()).
// "=" in es/it means "the same as en". The format markers (%d, %s...) of every translation must be the same, and in the same
// order, as in the English text.
const fs = require('fs'), path = require('path');
const src = path.join(__dirname, '..', 'src', 'i18n', 'translations.json');
const dst = path.join(__dirname, '..', 'src', 'i18n', 'tr_data.cpp');
const rows = JSON.parse(fs.readFileSync(src, 'utf8'));

const placeholders = s => (s.replace(/%%/g, '').match(/%[-0-9.l]*[dusxXfi]/g) || []).join(' ');
const esc = s => '"' + s.replace(/\\/g, '\\\\').replace(/"/g, '\\"').replace(/\n/g, '\\n').replace(/\r/g, '\\r').replace(/\t/g, '\\t')
	.replace(/\?\?/g, '?\\?') + '"';

let problems = 0;
const seen = new Set();
const out = [];
for (const r of rows) {
	if (typeof r.en !== 'string' || !r.en) continue;
	if (seen.has(r.en)) { console.error('duplicate: ' + JSON.stringify(r.en)); problems++; continue; }
	seen.add(r.en);
	const es = r.es ?? '=', it = r.it ?? '=';
	for (const [lang, t] of [['es', es], ['it', it]]) {
		if (t === '=') continue;
		if (placeholders(t) !== placeholders(r.en)) { console.error(`different markers (${lang}): ${JSON.stringify(r.en)} -> ${JSON.stringify(t)}`); problems++; }
		if (/^\s/.test(r.en) !== /^\s/.test(t) || /\s$/.test(r.en) !== /\s$/.test(t)) console.error(`warning: edge spaces (${lang}): ${JSON.stringify(r.en)} -> ${JSON.stringify(t)}`);
	}
	out.push(`\t{${esc(r.en)}, ${esc(es)}, ${esc(it)}},`);
}
const previous = fs.existsSync(dst) ? fs.readFileSync(dst, 'utf8') : '';
const header = (/^(\/\/ SPDX-License-Identifier: [^\n]*\n\/\/ Copyright \(C\) [^\n]*\n\/\/ Part of Imperivm Resurrected\.[^\n]*\n)/.exec(previous) || ['', ''])[1];
const cpp = header + `struct TrRow {
	const char *en, *es, *it;
};

extern const TrRow kTrRows[] = {
${out.join('\n')}
};
extern const int kTrCount = sizeof(kTrRows) / sizeof(kTrRows[0]);
`;
fs.writeFileSync(dst, cpp, 'utf8');
console.log(`${out.length} texts written to ${path.relative(process.cwd(), dst)}` + (problems ? ` (${problems} problems)` : ''));
process.exit(problems ? 1 : 0);
