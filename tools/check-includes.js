// Checks that every file in src/ includes the standard headers of the library names it uses (include what you use), so that
// removing an include somewhere else never breaks it. Only well-known names are checked, and only the C++ library and the
// fixed-width integer types.
//   node tools/check-includes.js          check
//   node tools/check-includes.js --fix    add the missing includes (after the last system include of the file)
'use strict';
const fs = require('fs');
const path = require('path');

const SRC = path.resolve(__dirname, '..', 'src');
const FIX = process.argv.includes('--fix');

const RULES = [
  ['<string>', /\bstd::(string|wstring|to_string|stoi|stol|stoul|stoull|stod|to_wstring|getline|char_traits)\b/],
  ['<vector>', /\bstd::vector\b/],
  ['<map>', /\bstd::(map|multimap)\b/],
  ['<set>', /\bstd::(set|multiset)\b/],
  ['<unordered_map>', /\bstd::unordered_map\b/],
  ['<unordered_set>', /\bstd::unordered_set\b/],
  ['<deque>', /\bstd::deque\b/],
  ['<array>', /\bstd::array\b/],
  ['<list>', /\bstd::list\b/],
  ['<queue>', /\bstd::(queue|priority_queue)\b/],
  ['<mutex>', /\bstd::(mutex|lock_guard|unique_lock|recursive_mutex|scoped_lock|call_once|once_flag)\b/],
  ['<atomic>', /\bstd::(atomic|atomic_bool|atomic_int|memory_order_\w+)\b/],
  ['<thread>', /\bstd::(thread|this_thread)\b/],
  ['<condition_variable>', /\bstd::condition_variable\b/],
  ['<functional>', /\bstd::(function|bind|ref|cref|hash|less|greater)\b/],
  ['<algorithm>', /\bstd::(sort|stable_sort|find|find_if|find_if_not|remove_if|remove|count|count_if|reverse|unique|copy|copy_if|transform|any_of|all_of|none_of|lower_bound|upper_bound|binary_search|min_element|max_element|nth_element|for_each|fill|equal|search|rotate|shuffle|clamp|partition)\b/],
  ['<utility>', /\bstd::(move|pair|make_pair|forward|exchange|declval)\b/],
  ['<memory>', /\bstd::(unique_ptr|shared_ptr|weak_ptr|make_unique|make_shared|unique_ptr|allocator)\b/],
  ['<chrono>', /\bstd::chrono\b/],
  ['<sstream>', /\bstd::(ostringstream|istringstream|stringstream)\b/],
  ['<fstream>', /\bstd::(ifstream|ofstream|fstream)\b/],
  ['<limits>', /\bstd::numeric_limits\b/],
  ['<tuple>', /\bstd::(tuple|tie|make_tuple|get)\b/],
  ['<optional>', /\bstd::(optional|nullopt)\b/],
  ['<stdexcept>', /\bstd::(runtime_error|logic_error|out_of_range|invalid_argument)\b/],
  ['<filesystem>', /\bstd::filesystem\b/],
  ['<initializer_list>', /\bstd::initializer_list\b/],
  ['<cstdint>', /\b(uint8_t|uint16_t|uint32_t|uint64_t|int8_t|int16_t|int32_t|int64_t|uintptr_t|intptr_t)\b/],
];
const EQUIVALENT = {
  '<string>': ['<string>', '<sstream>', '<fstream>', '<iostream>'],
  '<utility>': ['<utility>', '<map>', '<set>', '<unordered_map>', '<unordered_set>', '<vector>', '<algorithm>', '<tuple>', '<functional>'],
  '<cstdint>': ['<cstdint>', '<stdint.h>'],
  '<vector>': ['<vector>'],
};
const systemIncludes = (t) => [...t.matchAll(/^[ \t]*#[ \t]*include[ \t]*(<[^>]+>)/gm)].map((m) => m[1]);
const read = (rel) => (fs.existsSync(path.join(SRC, rel)) ? fs.readFileSync(path.join(SRC, rel), 'latin1') : '');

function mask(text) {
  const out = [];
  const n = text.length;
  let i = 0;
  while (i < n) {
    const c = text[i], d = text[i + 1];
    if (c === '/' && d === '/') { while (i < n && text[i] !== '\n') { out.push(' '); i++; } continue; }
    if (c === '/' && d === '*') { const e = text.indexOf('*/', i + 2); const stop = e < 0 ? n : e + 2; for (; i < stop; i++) out.push(text[i] === '\n' ? '\n' : ' '); continue; }
    if (c === 'R' && d === '"' && !/[A-Za-z0-9_]/.test(text[i - 1] || ' ')) {
      const open = text.indexOf('(', i + 2);
      const delim = text.slice(i + 2, open);
      const close = text.indexOf(')' + delim + '"', open);
      const stop = close < 0 ? n : close + delim.length + 2;
      for (; i < stop; i++) out.push(text[i] === '\n' ? '\n' : ' ');
      continue;
    }
    if (c === '"' || (c === "'" && !/[0-9a-fA-F]/.test(text[i - 1] || ' '))) {
      let j = i + 1;
      while (j < n && text[j] !== c && text[j] !== '\n') { if (text[j] === '\\') j++; j++; }
      for (; i <= j && i < n; i++) out.push(text[i] === '\n' ? '\n' : ' ');
      continue;
    }
    out.push(c);
    i++;
  }
  return out.join('');
}

function walk(dir, rel, out) {
  for (const e of fs.readdirSync(path.join(dir, rel), { withFileTypes: true })) {
    const r = rel ? rel + '/' + e.name : e.name;
    if (e.isDirectory()) walk(dir, r, out);
    else if (/\.(cpp|h)$/.test(e.name) && r !== 'i18n/tr_data.cpp') out.push(r);
  }
  return out;
}

let missingTotal = 0, filesFixed = 0;
const report = [];
for (const rel of walk(SRC, '', [])) {
  const text = fs.readFileSync(path.join(SRC, rel), 'latin1');
  const code = mask(text.replace(/^[ \t]*#[ \t]*include.*$/gm, ''));
  const have = new Set(systemIncludes(text));
  if (rel.endsWith('.cpp')) {
    const dir = rel.slice(0, rel.lastIndexOf('/'));
    const own = rel.slice(0, -4) + '.h';
    const companions = [own, ...[...text.matchAll(/^[ \t]*#[ \t]*include[ \t]*"([^"]*_internal\.h)"/gm)].map((m) => m[1])];
    for (const c of companions) {
      const ct = read(c);
      for (const h of systemIncludes(ct)) have.add(h);
      if (c.endsWith('_internal.h')) for (const m of ct.matchAll(/^[ \t]*#[ \t]*include[ \t]*"([^"]+)"/gm)) for (const h of systemIncludes(read(m[1]))) have.add(h);
    }
  }
  const missing = [];
  for (const [header, re] of RULES) {
    if (!re.test(code)) continue;
    const ok = (EQUIVALENT[header] || [header]).some((h) => have.has(h));
    if (!ok) missing.push(header);
  }
  if (!missing.length) continue;
  missingTotal += missing.length;
  report.push(rel + ': ' + missing.join(' '));
  if (FIX) {
    const lines = text.split('\n');
    let at = -1;
    lines.forEach((l, i) => { if (/^[ \t]*#[ \t]*include[ \t]*</.test(l)) at = i; });
    if (at < 0) {
      const firstInclude = lines.findIndex((l) => /^[ \t]*#[ \t]*include\b/.test(l));
      const pragma = lines.findIndex((l) => /^#pragma once/.test(l));
      at = firstInclude >= 0 ? firstInclude - 1 : pragma;
      lines.splice(at + 1, 0, ...missing.map((h) => '#include ' + h), '');
      if (lines[at + 1 + missing.length + 1] === '') lines.splice(at + 1 + missing.length, 1);
    } else {
      lines.splice(at + 1, 0, ...missing.map((h) => '#include ' + h));
    }
    fs.writeFileSync(path.join(SRC, rel), lines.join('\n'), 'latin1');
    filesFixed++;
  }
}
console.log(FIX ? `added ${missingTotal} includes in ${filesFixed} files` : `${missingTotal} missing standard includes in ${report.length} files`);
if (!FIX) for (const r of report.slice(0, 40)) console.log('  ' + r);
if (!FIX && report.length > 40) console.log('  ... and ' + (report.length - 40) + ' more');
process.exitCode = !FIX && missingTotal ? 1 : 0;
