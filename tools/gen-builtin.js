// Generates dll/src/hooks/builtin_data.cpp from the patches in dll/src/hooks/builtin/: the DLL's own changes to game files,
// applied like a mod's patches (patches\<path>) but before them, over whichever version of the file is loaded.
// The path under builtin\ is the path the game opens (menuini\mainmenu.ini is menuini/mainmenu.ini).
// A fix of one exact game file starts with the line "@game-file-sha256 <sha256 of that file>": it is applied only when the
// loaded file is that one, and a content pack's own version of the file is left as its author wrote it.
//
//   node dll/tools/gen-builtin.js
const fs = require('fs'), path = require('path');
const root = path.join(__dirname, '..', 'src', 'hooks', 'builtin');
const dst = path.join(__dirname, '..', 'src', 'hooks', 'builtin_data.cpp');

const esc = s => '"' + s.replace(/\\/g, '\\\\').replace(/"/g, '\\"').replace(/\t/g, '\\t').replace(/\?\?/g, '?\\?') + '\\n"';
const files = [];
(function walk(dir, rel) {
	for (const e of fs.readdirSync(dir, { withFileTypes: true }).sort((a, b) => a.name < b.name ? -1 : 1)) {
		const r = rel ? rel + '/' + e.name : e.name;
		if (e.isDirectory()) walk(path.join(dir, e.name), r);
		else files.push(r);
	}
})(root, '');

let problems = 0;
const rows = [];
for (const rel of files) {
	const text = fs.readFileSync(path.join(root, rel), 'utf8').replace(/^﻿/, '').replace(/\r\n/g, '\n');
	if (/[^\x09\x0a\x20-\x7e]/.test(text)) { console.error(`${rel}: only ASCII text`); problems++; continue; }
	const lines = text.replace(/\n+$/, '').split('\n');
	let sha = 'nullptr';
	const only = /^@game-file-sha256(?:\s+(.*))?$/.exec(lines[0]);
	if (only) {
		if (!/^[0-9a-f]{64}$/.test(only[1] || '')) { console.error(`${rel}: @game-file-sha256 needs the 64 lowercase hex digits of the file's sha256`); problems++; continue; }
		sha = `"${only[1]}"`;
		lines.shift();
	}
	if (lines.some(l => /^@game-file-sha256/.test(l))) { console.error(`${rel}: @game-file-sha256 goes only on the first line`); problems++; continue; }
	rows.push(`\t{"${rel.toLowerCase()}",\n${lines.map(l => '\t\t' + esc(l)).join('\n')},\n\t\t${sha}},`);
}
const previous = fs.existsSync(dst) ? fs.readFileSync(dst, 'utf8') : '';
const header = (/^(\/\/ SPDX-License-Identifier: [^\n]*\n\/\/ Copyright \(C\) [^\n]*\n\/\/ Part of Imperivm Resurrected\.[^\n]*\n)/.exec(previous) || ['', ''])[1];
const cpp = header + `#include "hooks/builtin_data.h"

namespace {
const BuiltinText kPatches[] = {
${rows.join('\n')}
};
}

const BuiltinText *BuiltinPatchTexts(int &count)
{
	count = static_cast<int>(sizeof(kPatches) / sizeof(kPatches[0]));
	return kPatches;
}
`;
if (cpp !== previous) fs.writeFileSync(dst, cpp, 'utf8');
console.log(`${rows.length} built-in patches written to ${path.relative(process.cwd(), dst)}` + (problems ? ` (${problems} problems)` : ''));
process.exit(problems ? 1 : 0);
