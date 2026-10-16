// Checks the rules for the addresses of the game executable (gbr.exe):
//   1. Every address lives in src/addr/*.h as `inline constexpr uintptr_t kName = 0x00xxxxxx;`: names and values are unique across
//      the headers, written with 8 lowercase hex digits and sorted by value inside each header.
//   2. No other file writes an 8-digit hex literal in the address range of the executable (0x00400000-0x00afffff). Assembly lines of
//      a naked function and `static_assert` lines (which tie them to the constants) are the only exceptions.
//   node tools/check-addresses.js [--ghidra]
// --ghidra also lists the code constants that are not the start of a function in export/functions.tsv (information only: data
// addresses, vtables and addresses inside a function are expected to be there).
'use strict';
const fs = require('fs');
const path = require('path');

const SRC = path.resolve(__dirname, '..', 'src');
const LO = 0x400000, HI = 0xb00000;
const problems = [];

function walk(dir, rel, out) {
  for (const e of fs.readdirSync(path.join(dir, rel), { withFileTypes: true })) {
    const r = rel ? rel + '/' + e.name : e.name;
    if (e.isDirectory()) walk(dir, r, out);
    else if (/\.(cpp|h|c|inc)$/.test(e.name)) out.push(r);
  }
  return out;
}

const files = walk(SRC, '', []);
const names = new Map(), values = new Map();
const entries = [];
for (const rel of files.filter((f) => f.startsWith('addr/'))) {
  let prev = -1;
  fs.readFileSync(path.join(SRC, rel), 'latin1').split(/\r?\n/).forEach((ln, i) => {
    const where = rel + ':' + (i + 1);
    const scalar = /^inline constexpr uintptr_t (k\w+) = (0x[0-9a-fA-F]+);$/.exec(ln);
    const array = /^constexpr uintptr_t (k\w+)\[\] = \{(0x[0-9a-f]{8}(?:, 0x[0-9a-f]{8})*)\};$/.exec(ln);
    if (!scalar && !array) {
      if (/\bk\w+\s*=|0x/.test(ln)) problems.push(where + ': unexpected declaration form: ' + ln);
      return;
    }
    const name = (scalar || array)[1];
    const lits = scalar ? [scalar[2]] : array[2].split(', ');
    for (const l of lits) if (!/^0x[0-9a-f]{8}$/.test(l)) problems.push(where + ': ' + name + ' must be written with 8 lowercase hex digits (' + l + ')');
    if (names.has(name)) problems.push(where + ': duplicate name ' + name + ' (also at ' + names.get(name) + ')');
    names.set(name, where);
    const first = parseInt(lits[0], 16);
    if (scalar) {
      if (values.has(first)) problems.push(where + ': ' + name + ' has the same value as ' + values.get(first));
      values.set(first, name);
    }
    if (first < prev) problems.push(where + ': ' + name + ' is not sorted by value');
    prev = first;
    entries.push({ name, value: first, array: !!array, where });
  });
}

const asmRe = /^\s*(mov|push|imul|call|cmp|lea|jmp|ret|add|sub|xor|test|jae|jz|jns)\b/;
for (const rel of files) {
  if (rel.startsWith('addr/') || rel === 'i18n/tr_data.cpp' || rel === 'hooks/miniaudio_impl.c') continue;
  fs.readFileSync(path.join(SRC, rel), 'latin1').split(/\r?\n/).forEach((ln, i) => {
    if (asmRe.test(ln) || /^\s*static_assert\b/.test(ln)) return;
    for (const m of ln.matchAll(/\b0[xX]([0-9a-fA-F]{8})\b/g)) {
      const v = parseInt(m[1], 16);
      if (v >= LO && v < HI) problems.push(rel + ':' + (i + 1) + ': address literal ' + m[0] + ' outside src/addr (add it to a header in src/addr)');
    }
  });
}

if (process.argv.includes('--ghidra')) {
  const tsv = path.resolve(__dirname, '..', '..', 'export', 'functions.tsv');
  if (!fs.existsSync(tsv)) console.log('export/functions.tsv not found: skipping the Ghidra check');
  else {
    const starts = new Set(fs.readFileSync(tsv, 'utf8').split('\n').slice(1).map((l) => parseInt(l.split('\t')[0], 16)).filter((v) => !Number.isNaN(v)));
    const notFunctions = entries.filter((e) => e.value < 0x780000 && !starts.has(e.value));
    console.log(notFunctions.length + ' constants below 0x00780000 are not the start of a known function:');
    for (const e of notFunctions) console.log('  ' + e.name + ' = 0x' + e.value.toString(16).padStart(8, '0'));
  }
}

console.log(`${entries.length} address constants in ${files.filter((f) => f.startsWith('addr/')).length} headers, ${problems.length} problems`);
for (const p of problems) console.log('  ' + p);
process.exitCode = problems.length ? 1 : 0;
