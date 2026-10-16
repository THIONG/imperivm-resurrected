// Removes C/C++ comments from the given files, in place. Usage: node strip-comments.js <file>...
// Handles string, character and raw string literals, digit separators, line splices and CRLF files. A line left empty by a
// removed comment is dropped; trailing comments take their leading spaces with them; runs of blank lines collapse to one.
'use strict';
const fs = require('fs');

function strip(text) {
  const bom = text.charCodeAt(0) === 0xfeff ? '﻿' : '';
  if (bom) text = text.slice(1);
  const crlf = text.includes('\r\n');
  text = text.replace(/\r\n/g, '\n');
  let out = '';
  let i = 0;
  const n = text.length;
  while (i < n) {
    const c = text[i];
    const d = text[i + 1];
    if (c === '/' && d === '/') {
      // line comment, continued by a backslash at the end of a line
      i += 2;
      while (i < n && text[i] !== '\n') {
        if (text[i] === '\\' && text[i + 1] === '\n') i += 2;
        else i++;
      }
      out = out.replace(/[ \t]+$/, '');
      if ((out === '' || out.endsWith('\n')) && text[i] === '\n') i++;  // the line held only the comment
      continue;
    }
    if (c === '/' && d === '*') {
      const end = text.indexOf('*/', i + 2);
      const stop = end < 0 ? n : end + 2;
      const body = text.slice(i, stop);
      const newlines = (body.match(/\n/g) || []).length;
      // keep the line structure only if code surrounds the comment on both sides
      const before = out.slice(out.lastIndexOf('\n') + 1);
      const after = text.slice(stop, text.indexOf('\n', stop) < 0 ? n : text.indexOf('\n', stop));
      if (before.trim() === '' && after.trim() === '') {
        i = stop;
        if (text[i] === '\n') i++;
        out = out.replace(/[ \t]+$/, '');
        continue;
      }
      if (before.trim() === '' || /^[ \t]/.test(after)) {
        i = stop;  // at the start of a line, or followed by a space: nothing to separate
        if (after.startsWith(' ')) i++;
        continue;
      }
      out = out.replace(/[ \t]+$/, '');
      out += newlines ? '\n' : ' ';
      i = stop;
      continue;
    }
    if (c === 'R' && d === '"' && !/[A-Za-z0-9_]/.test(text[i - 1] || ' ')) {
      // raw string literal R"delim( ... )delim"
      const open = text.indexOf('(', i + 2);
      const delim = text.slice(i + 2, open);
      const close = text.indexOf(')' + delim + '"', open);
      const stop = close < 0 ? n : close + delim.length + 2;
      out += text.slice(i, stop);
      i = stop;
      continue;
    }
    if (c === '"' || (c === "'" && !isDigitSeparator(text, i))) {
      const quote = c;
      let j = i + 1;
      while (j < n && text[j] !== quote && text[j] !== '\n') {
        if (text[j] === '\\') j++;
        j++;
      }
      out += text.slice(i, j + 1);
      i = j + 1;
      continue;
    }
    out += c;
    i++;
  }
  // a comment-only line leaves an empty line behind: drop those that were not empty before
  const before = text.split('\n');
  const after = out.split('\n');
  void before;
  let result = after.join('\n');
  result = result.replace(/^[ \t]*\n/, '');
  result = result.replace(/\n{3,}/g, '\n\n');
  if (crlf) result = result.replace(/\n/g, '\r\n');
  return bom + result;
}

function isDigitSeparator(text, i) {
  const p = text[i - 1] || '';
  const q = text[i + 1] || '';
  return /[0-9a-fA-F]/.test(p) && /[0-9a-fA-F]/.test(q) && numberBefore(text, i);
}

function numberBefore(text, i) {
  // walk back over the pp-number to see that it starts with a digit
  let j = i - 1;
  while (j >= 0 && /[0-9A-Za-z_.']/.test(text[j])) j--;
  return /[0-9]/.test(text[j + 1] || '');
}

module.exports = { strip };

if (require.main === module) {
  for (const file of process.argv.slice(2)) {
    const original = fs.readFileSync(file, 'utf8');
    const stripped = strip(original);
    if (stripped !== original) fs.writeFileSync(file, stripped, 'utf8');
  }
}
