// Checks the conventions of the files in src/ (the rules are in docs/conventions.md):
//   - no comments (the code explains itself through names; the reasons live in docs/ and in the commit messages);
//   - UTF-8 without BOM, LF line ends, no trailing whitespace, tabs for indentation;
//   - file names in lowercase letters, digits and underscores;
//   - project includes are written "folder/file.h" from the include root src/ (never ../ and never a bare name);
//   - every header starts with #pragma once and has no `using namespace`;
//   - headers do not declare shared variables: `extern` and `inline g_...` only in the listed exceptions, and in the
//     `<module>_internal.h` headers, which hold the state that the files of one split module share and can only be included by
//     the files of that module (`<module>.cpp`, `<module>_*.cpp`).
//   node tools/check-conventions.js
'use strict';
const fs = require('fs');
const path = require('path');
const { strip } = require('./strip-comments.js');

const SRC = path.resolve(__dirname, '..', 'src');
const FOLDERS = new Set(['addr', 'core', 'i18n', 'hooks', 'mods', 'game', 'net', 'ui', 'app']);
const EXTERN_ALLOWED = new Set(['core/crashlog.h:g_crashLogQuiet', 'net/discord.h:kDiscordInviteUrl']);
const INLINE_GLOBAL_ALLOWED = new Set(['ui/textedit.h', 'ui/uikit.h']);
const THIRD_PARTY = new Set(['MinHook.h', 'miniaudio.h', 'extras/stb_vorbis.c', 'miniz.h']);
const NO_COMMENT_CHECK = new Set(['i18n/tr_data.cpp']);
const problems = [];

function walk(dir, rel, out) {
  for (const e of fs.readdirSync(path.join(dir, rel), { withFileTypes: true })) {
    const r = rel ? rel + '/' + e.name : e.name;
    if (e.isDirectory()) walk(dir, r, out);
    else out.push(r);
  }
  return out;
}

const normalize = (s) => s.replace(/\r\n/g, '\n').replace(/\n{3,}/g, '\n\n').replace(/^[ \t]*\n/, '');
const LICENSE_HEADER = /^\/\/ SPDX-License-Identifier: [^\n]*\n\/\/ Copyright \(C\) [^\n]*\n\/\/ Part of Imperivm Resurrected\.[^\n]*\n/;

const allFiles = walk(SRC, '', []);
const seenBase = new Map();
for (const rel of allFiles.filter((f) => /\.(cpp|c)$/.test(f))) {
  const base = path.basename(rel);
  if (seenBase.has(base)) problems.push(rel + ': same file name as ' + seenBase.get(base) + ' (the object files go to one folder)');
  seenBase.set(base, rel);
}

for (const rel of allFiles) {
  const base = path.basename(rel);
  if (!/^[a-z0-9_]+\.[a-z]+$/.test(base)) problems.push(rel + ': file name must be lowercase letters, digits and underscores');
  if (!/\.(cpp|h|c|inc)$/.test(base)) continue;
  const buf = fs.readFileSync(path.join(SRC, rel));
  const full = buf.toString('utf8');
  if (full.charCodeAt(0) === 0xfeff) problems.push(rel + ': has a BOM');
  const text = full.replace(LICENSE_HEADER, '');
  if (text.includes('\r')) problems.push(rel + ': has CR characters (use LF)');
  const lines = text.split('\n');
  lines.forEach((ln, i) => {
    if (/[ \t]+$/.test(ln)) problems.push(rel + ':' + (i + 1) + ': trailing whitespace');
    if (/^ +\S/.test(ln) && !/^ \*/.test(ln)) problems.push(rel + ':' + (i + 1) + ': indented with spaces (use tabs)');
  });
  if (!NO_COMMENT_CHECK.has(rel) && normalize(strip(text)) !== normalize(text)) {
    const a = strip(text).split('\n'), b = text.split('\n');
    let line = 1;
    const stripped = new Set(a);
    for (let i = 0; i < b.length; i++) if (!stripped.has(b[i])) { line = i + 1; break; }
    problems.push(rel + ':' + line + ': contains a comment');
  }
  if (rel !== 'hooks/hookcount.cpp') {
    lines.forEach((ln, i) => {
      if (/\bMH_(CreateHook|EnableHook)\(/.test(ln)) problems.push(rel + ':' + (i + 1) + ': use HookCreate/HookEnable (hooks/hookcount.h), which count the hooks for the start-up summary');
    });
  }
  lines.forEach((ln, i) => {
    const m = /^[ \t]*#[ \t]*include[ \t]+"([^"]+)"/.exec(ln);
    if (!m) return;
    const inc = m[1];
    if (THIRD_PARTY.has(inc)) return;
    if (/\.\./.test(inc)) problems.push(rel + ':' + (i + 1) + ': include with ../ (' + inc + ')');
    else if (!FOLDERS.has(inc.split('/')[0]) || !inc.includes('/')) problems.push(rel + ':' + (i + 1) + ': include must be "folder/file" (' + inc + ')');
    const internal = /^(\w+)\/(\w+)_internal\.h$/.exec(inc);
    if (internal && !new RegExp('^' + internal[1] + '/' + internal[2] + '(_\\w+)?\\.(cpp|h)$').test(rel)) {
      problems.push(rel + ':' + (i + 1) + ': ' + inc + ' is internal to the ' + internal[2] + ' module');
    }
  });
  if (/\.h$/.test(base)) {
    const first = lines.find((l) => l.trim() !== '');
    if (first !== '#pragma once') problems.push(rel + ': the first line must be #pragma once');
    if (/_internal\.h$/.test(rel)) continue;
    lines.forEach((ln, i) => {
      if (/^\s*using\s+namespace\b/.test(ln)) problems.push(rel + ':' + (i + 1) + ': using namespace in a header');
      const ext = /^\s*extern\s+(?!"C").*?\b([A-Za-z_]\w*)\s*(\[[^\]]*\])?\s*;/.exec(ln);
      if (ext && !/\(/.test(ln) && !EXTERN_ALLOWED.has(rel + ':' + ext[1])) problems.push(rel + ':' + (i + 1) + ': extern variable ' + ext[1] + ' in a header');
      if (/^\s*inline\s+(?!constexpr)[\w:<>\*& ]+\bg_\w+/.test(ln) && !INLINE_GLOBAL_ALLOWED.has(rel)) problems.push(rel + ':' + (i + 1) + ': shared g_ variable in a header');
    });
  }
}

console.log(`${problems.length} convention problems`);
for (const p of problems.slice(0, 60)) console.log('  ' + p);
if (problems.length > 60) console.log('  ... and ' + (problems.length - 60) + ' more');
process.exitCode = problems.length ? 1 : 0;
