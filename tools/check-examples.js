// Runs the validator of the SDK on every example of es, en and it: an example that stops validating is a broken example.
// The folder that is not a mod (the sprite generator) is skipped. No game files are needed.
//   node tools/check-examples.js
'use strict';
const fs = require('fs');
const path = require('path');
const { spawnSync } = require('child_process');
const { SDK } = require('./sdk-common');

const LANGS = {
  es: { examples: 'ejemplos', tools: 'herramientas', validator: 'validar-mod.js', skip: ['unidad-graficos-generador'] },
  en: { examples: 'examples', tools: 'tools', validator: 'validate-mod.js', skip: ['unit-graphics-generator'] },
  it: { examples: 'esempi', tools: 'strumenti', validator: 'valida-mod.js', skip: ['generatore-unita-grafica'] },
};
let checked = 0;
const problems = [];
for (const [lang, c] of Object.entries(LANGS)) {
  const root = path.join(SDK, lang, c.examples);
  for (const name of fs.readdirSync(root)) {
    if (c.skip.includes(name) || !fs.statSync(path.join(root, name)).isDirectory()) continue;
    const r = spawnSync(process.execPath, [path.join(SDK, lang, c.tools, c.validator), path.join(root, name)], { encoding: 'utf8' });
    checked++;
    if (r.status !== 0) problems.push(`${lang}/${c.examples}/${name}: ${(r.stdout + r.stderr).trim().split('\n').slice(0, 4).join(' | ')}`);
  }
}
console.log(`${checked} examples validated, ${problems.length} with problems`);
for (const p of problems) console.log('  ' + p);
process.exitCode = problems.length ? 1 : 0;
