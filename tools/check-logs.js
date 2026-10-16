// Checks the log calls of src/ against the rules of the logging system (docs/conventions.md):
//   - every call names a channel from the list in src/core/logchannels.h;
//   - no call uses the old Log("Channel: text") form (the compatible Log() only exists for old code);
//   - messages are English and ASCII, do not end with a newline, do not repeat the channel name as a prefix and are not built with Tr();
//   - a call does not print raw 64-bit ids with %llu in a channel that is not about the network (names and ids stay in net channels).
//   node tools/check-logs.js
'use strict';
const fs = require('fs');
const path = require('path');

const SRC = path.resolve(__dirname, '..', 'src');
const EXEMPT = new Set(['core/log.cpp', 'core/log.h', 'core/logcore.cpp', 'core/logcore.h', 'core/logfiles.cpp', 'core/logfiles.h', 'core/logchannels.h']);
const NET_CHANNELS = new Set(['steam', 'lobby', 'room', 'match', 'net', 'rejoin', 'spectator', 'content-share', 'discord', 'download']);

const registry = new Set([...fs.readFileSync(path.join(SRC, 'core', 'logchannels.h'), 'utf8').matchAll(/"([a-z-]+)"/g)].map((m) => m[1]));
if (registry.size < 20) { console.error('could not read the channel list from core/logchannels.h'); process.exit(2); }

const walk = (dir, rel, out) => {
  for (const e of fs.readdirSync(path.join(dir, rel), { withFileTypes: true })) {
    const r = rel ? rel + '/' + e.name : e.name;
    if (e.isDirectory()) walk(dir, r, out);
    else if (/\.(cpp|h)$/.test(e.name)) out.push(r);
  }
  return out;
};

const problems = [];
let calls = 0;
const LIT = '"((?:[^"\\\\]|\\\\.)*)"';
const patterns = [
  new RegExp('(?<![A-Za-z0-9_])Log[TDIWEF]\\(\\s*' + LIT + '\\s*,\\s*' + LIT, 'g'),
  new RegExp('(?<![A-Za-z0-9_])LogAt\\([^"]*?' + LIT + '\\s*,\\s*' + LIT, 'g'),
  new RegExp('(?<![A-Za-z0-9_])Log(?:Every|Once)\\([^"]*?' + LIT + '\\s*,\\s*' + LIT, 'g'),
];

for (const rel of walk(SRC, '', [])) {
  if (EXEMPT.has(rel)) continue;
  const text = fs.readFileSync(path.join(SRC, rel), 'utf8');
  const lineOf = (index) => text.slice(0, index).split('\n').length;
  for (const m of text.matchAll(/(?<![A-Za-z0-9_])Log\(\s*"/g)) problems.push(`${rel}:${lineOf(m.index)}: old Log("...") call: use LogI/LogW/LogE/LogD with a channel`);
  for (const re of patterns) {
    for (const m of text.matchAll(re)) {
      calls++;
      const where = `${rel}:${lineOf(m.index)}`;
      const channel = m[1], format = m[2];
      if (!registry.has(channel)) problems.push(`${where}: unknown channel "${channel}" (add it to core/logchannels.h)`);
      if (/[^\x00-\x7f]/.test(format)) problems.push(`${where}: the message has non-ASCII characters`);
      if (/\\n$/.test(format)) problems.push(`${where}: the message ends with a newline`);
      if (format.toLowerCase().startsWith(channel + ': ')) problems.push(`${where}: the message repeats the channel "${channel}:"`);
      if (/%llu|%I64u/.test(format) && !NET_CHANNELS.has(channel)) problems.push(`${where}: a 64-bit id is printed in the channel "${channel}"`);
      const callEnd = text.indexOf(');', m.index);
      if (callEnd > 0 && /\bTr(?:Fmt)?\(/.test(text.slice(m.index, callEnd))) problems.push(`${where}: the message goes through Tr() (logs are English; use TrSource() for text that is already translated)`);
    }
  }
}

console.log(`${calls} log calls checked, ${problems.length} problems`);
for (const p of problems.slice(0, 60)) console.log('  ' + p);
if (problems.length > 60) console.log('  ... and ' + (problems.length - 60) + ' more');
process.exitCode = problems.length ? 1 : 0;
