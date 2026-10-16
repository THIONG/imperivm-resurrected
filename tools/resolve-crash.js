// Turns the addresses of a crash in imperivm-mod.log ("winmm.dll+1a2b3") into function names, using the linker map of the same build.
// The log header says which build it is ("session: build 6ad1f7c2, built ..."); publish-version.js keeps the map of every release as
// dll\symbols\winmm-<version>-<build>.map.zip.
//   node tools/resolve-crash.js <imperivm-mod.log> <winmm.map | winmm-<version>-<build>.map.zip> [--demangle]
// --demangle needs undname.exe on the PATH (a Visual Studio Developer Command Prompt has it).
'use strict';
const fs = require('fs');
const path = require('path');
const { execFileSync, spawnSync } = require('child_process');

const args = process.argv.slice(2);
const files = args.filter((a) => !a.startsWith('--'));
if (files.length < 2) { console.error('usage: node tools/resolve-crash.js <log> <map or map zip> [--demangle]'); process.exit(2); }
const [logFile, mapFile] = files;

let mapText;
if (/\.zip$/i.test(mapFile)) {
  const tar = process.platform === 'win32' ? path.join(process.env.SystemRoot || 'C:\\Windows', 'System32', 'tar.exe') : 'tar';
  mapText = execFileSync(tar, ['-xOf', mapFile, 'winmm.map'], { encoding: 'latin1', maxBuffer: 1 << 28 });
}
else {
  mapText = fs.readFileSync(mapFile, 'latin1');
}
const log = fs.readFileSync(logFile, 'latin1');

const base = parseInt((/Preferred load address is ([0-9a-f]+)/i.exec(mapText) || [])[1] || '', 16);
const mapStamp = (/Timestamp is ([0-9a-f]{8})/i.exec(mapText) || [])[1];
const logStamp = (/build ([0-9a-f]{8})/i.exec(log) || [])[1];
if (!base) { console.error('this does not look like a linker map (no preferred load address)'); process.exit(2); }
if (logStamp && mapStamp && logStamp !== mapStamp) console.log(`WARNING: the log is from build ${logStamp} and the map from build ${mapStamp}: the names will be wrong`);

const symbols = [];
const section = mapText.indexOf('Publics by Value');
for (const line of mapText.slice(section).split(/\r?\n/)) {
  const m = /^\s*0001:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})\s+(?:f\s+)?(?:i\s+)?(\S+)\s*$/i.exec(line);
  if (m) symbols.push({ name: m[1], va: parseInt(m[2], 16), object: m[3] });
}
symbols.sort((a, b) => a.va - b.va);

const demangle = (name) => {
  if (!args.includes('--demangle') || !name.startsWith('?')) return name;
  const r = spawnSync('undname.exe', [name], { encoding: 'utf8' });
  const m = /is :- "([^"]*)"/.exec(r.stdout || '');
  return m ? m[1] : name;
};

const find = (va) => {
  let lo = 0, hi = symbols.length - 1, best = -1;
  while (lo <= hi) {
    const mid = (lo + hi) >> 1;
    if (symbols[mid].va <= va) { best = mid; lo = mid + 1; } else hi = mid - 1;
  }
  return best < 0 ? null : symbols[best];
};

const seen = new Set();
let resolved = 0;
for (const m of log.matchAll(/winmm\.dll\+([0-9a-f]{1,8})/gi)) {
  const key = m[1].toLowerCase();
  if (seen.has(key)) continue;
  seen.add(key);
  const va = base + parseInt(key, 16);
  const sym = find(va);
  if (sym && va - sym.va < 0x4000) {
    console.log(`winmm.dll+${key}  ${demangle(sym.name)} +0x${(va - sym.va).toString(16)}  (${sym.object})`);
    resolved++;
  }
  else {
    console.log(`winmm.dll+${key}  not found in the map`);
  }
}
console.log(`${resolved} of ${seen.size} addresses resolved` + (seen.size ? '' : ' (the log has no winmm.dll+offset address)'));
