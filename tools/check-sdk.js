// Checks sdk/: the layout is the flat English one, every link between guides points to something that exists, every
// guide listed in sdk/README.md is there and every example has its README. Usage:  node tools/check-sdk.js
const fs = require('fs');
const path = require('path');

const SDK = path.resolve(__dirname, '..', 'sdk');
const walk = d => { let o = []; for (const e of fs.readdirSync(d, { withFileTypes: true })) { const p = path.join(d, e.name); if (e.isDirectory()) o = o.concat(walk(p)); else o.push(p); } return o; };
const problems = [];
const add = msg => problems.push(msg);

for (const old of ['es', 'en', 'it']) if (fs.existsSync(path.join(SDK, old))) add(`sdk/${old} should not exist: the SDK is in English only, directly in sdk/`);

const changelog = path.join(SDK, 'CHANGELOG.md');
if (!fs.existsSync(changelog)) add('sdk/CHANGELOG.md is missing');
else if (!/^## \d+\.\d+/m.test(fs.readFileSync(changelog, 'utf8'))) add('sdk/CHANGELOG.md has no "## x.y" version heading');

for (const f of walk(SDK).filter(x => x.endsWith('.md'))) {
	const rel = path.relative(SDK, f).split(path.sep).join('/');
	for (const m of fs.readFileSync(f, 'utf8').matchAll(/\]\((?!https?:|#|mailto:)([^)\s#]+)(#[^)]*)?\)/g)) {
		const target = path.resolve(path.dirname(f), decodeURI(m[1]));
		if (!fs.existsSync(target)) add(`${rel}: broken link ${m[1]}`);
	}
}

const examples = path.join(SDK, 'examples');
if (fs.existsSync(examples)) {
	for (const name of fs.readdirSync(examples)) {
		if (fs.statSync(path.join(examples, name)).isDirectory() && !fs.existsSync(path.join(examples, name, 'README.md'))) add(`sdk/examples/${name} has no README.md`);
	}
}

if (problems.length) {
	console.log(problems.join('\n'));
	console.log('\n' + problems.length + ' problem(s).');
	process.exit(1);
}
console.log('sdk: no problems.');
