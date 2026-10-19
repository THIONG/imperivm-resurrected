// Builds the SDK zip (everything in sdk/) as imperivm-resurrected-sdk-<version>.zip in the given folder.
// The version is the first "## x.y" of sdk/CHANGELOG.md. The release workflow uses it; locally:
//   node tools/build-sdk-zip.js [output folder]     (default: release-sdk)
'use strict';
const fs = require('fs');
const path = require('path');
const { execFileSync } = require('child_process');

const ROOT = path.resolve(__dirname, '..');
const SDK = fs.existsSync(path.join(ROOT, 'sdk')) ? path.join(ROOT, 'sdk') : path.resolve(ROOT, '..', 'sdk');
const out = path.resolve(process.argv[2] || 'release-sdk');
const version = (/^## (\d+\.\d+(?:\.\d+)?)/m.exec(fs.readFileSync(path.join(SDK, 'CHANGELOG.md'), 'utf8')) || [])[1];
if (!version) { console.error('no version in sdk/CHANGELOG.md'); process.exit(1); }
fs.mkdirSync(out, { recursive: true });
const zip = path.join(out, `imperivm-resurrected-sdk-${version}.zip`);
fs.rmSync(zip, { force: true });
const tarExe = process.platform === 'win32' ? path.join(process.env.SystemRoot || 'C:\\Windows', 'System32', 'tar.exe') : 'tar';
execFileSync(tarExe, ['-a', '-c', '-f', zip, '-C', SDK, ...fs.readdirSync(SDK)]);
console.log(`${path.basename(zip)} (${(fs.statSync(zip).size / 1048576).toFixed(1)} MB)`);
