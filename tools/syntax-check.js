// Compiles every .cpp of src/ with /Zs (syntax and types only, no object files), so a change is checked without the Steamworks SDK.
// Files that need the Steamworks headers are skipped when the SDK is not available; with it (STEAMWORKS_SDK, the folder that
// contains public\steam\steam_api.h) every file is checked. It has to run where cl.exe is on the PATH (a Developer Command Prompt).
//   node tools/syntax-check.js [--strict] [--no-steam]
//   --strict    skipped files count as a failure
//   --no-steam  ignore the Steamworks SDK even if it is found (to see what the continuous integration checks)
'use strict';
const fs = require('fs');
const os = require('os');
const path = require('path');
const { spawn, spawnSync } = require('child_process');

const ROOT = path.resolve(__dirname, '..');
const SRC = path.join(ROOT, 'src');
const STRICT = process.argv.includes('--strict');
const NO_STEAM = process.argv.includes('--no-steam');

const firstExisting = (list) => list.find((p) => p && fs.existsSync(p));
const minhook = firstExisting([path.join(ROOT, 'third_party', 'minhook', 'include'), path.join(ROOT, '..', 'tools', 'minhook', 'include')]);
const miniaudio = firstExisting([path.join(ROOT, 'third_party', 'miniaudio'), path.join(ROOT, '..', 'tools', 'miniaudio')]);
const miniz = firstExisting([path.join(ROOT, 'third_party', 'miniz'), path.join(ROOT, '..', 'tools', 'miniz')]);
const libarchive = firstExisting([path.join(ROOT, "third_party", "libarchive"), path.join(ROOT, "..", "tools", "libarchive")]);
const steamRoot = NO_STEAM ? undefined : firstExisting([process.env.STEAMWORKS_SDK && path.join(process.env.STEAMWORKS_SDK, 'public'), path.join(ROOT, '..', 'tools', 'steamworks_sdk', 'public')]);

if (spawnSync('cl.exe', ['/?'], { encoding: 'utf8' }).error) {
  console.error('cl.exe was not found: run this from a Visual Studio Developer Command Prompt (x86).');
  process.exit(2);
}
if (!minhook || !miniaudio || !miniz || !libarchive) {
  console.error('MinHook, miniaudio, miniz or libarchive were not found (third_party\\ in the public repository, ..\\tools\\ in the private workspace).');
  process.exit(2);
}

const walk = (dir, rel, out) => {
  for (const e of fs.readdirSync(path.join(dir, rel), { withFileTypes: true })) {
    const r = rel ? rel + '/' + e.name : e.name;
    if (e.isDirectory()) walk(dir, r, out);
    else if (e.name.endsWith('.cpp')) out.push(r);
  }
  return out;
};
const files = walk(SRC, '', []).sort();

const include = ['/I' + SRC, '/I' + minhook, '/I' + miniaudio, '/I' + miniz, '/I' + libarchive];
if (steamRoot) include.push('/I' + steamRoot);
const flags = ['/nologo', '/Zs', '/std:c++17', '/utf-8', '/EHsc', '/W3', '/DNDEBUG', ...include];

const chunks = [];
const per = Math.max(1, Math.ceil(files.length / Math.max(1, os.cpus().length)));
for (let i = 0; i < files.length; i += per) chunks.push(files.slice(i, i + per));

const errors = [];
const skipped = new Set();

function run(chunk) {
  return new Promise((resolve) => {
    const child = spawn('cl.exe', [...flags, ...chunk.map((f) => path.join(SRC, f))], { cwd: SRC });
    let text = '';
    child.stdout.on('data', (d) => (text += d));
    child.stderr.on('data', (d) => (text += d));
    child.on('close', () => {
      let current = '';
      for (const line of text.split(/\r?\n/)) {
        const name = /^([\w.]+\.cpp)$/.exec(line.trim());
        if (name) { current = name[1]; continue; }
        if (!/\b(fatal )?error C\d+/.test(line)) continue;
        if (/C1083.*'[^']*(steam|isteam)[^']*'/i.test(line)) skipped.add(current);
        else errors.push(line.replace(SRC + path.sep, '').trim());
      }
      resolve();
    });
  });
}

Promise.all(chunks.map(run)).then(() => {
  const checked = files.length - skipped.size;
  console.log(`${checked} of ${files.length} files checked, ${skipped.size} skipped (they need the Steamworks SDK), ${errors.length} errors`);
  for (const e of errors.slice(0, 40)) console.log('  ' + e);
  if (errors.length > 40) console.log('  ... and ' + (errors.length - 40) + ' more');
  process.exitCode = errors.length || (STRICT && skipped.size) ? 1 : 0;
});
