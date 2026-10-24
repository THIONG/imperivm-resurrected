// Builds the installer of a version: build\installer-stub.exe + the release zip + a trailer that the stub reads back.
//   node installer/pack.js <zip> <version> <output.exe>
// Trailer (80 bytes): "IRSETUP1", zip size (u64 little endian), SHA-256 of the zip (32 bytes), version (32 bytes, zero padded).
'use strict';
const fs = require('fs');
const path = require('path');
const crypto = require('crypto');

function packInstaller(zipPath, version, outPath) {
	const stub = path.join(__dirname, '..', 'build', 'installer-stub.exe');
	if (!fs.existsSync(stub)) throw new Error('build\\installer-stub.exe is missing: build it with installer\\build.bat');
	const zip = fs.readFileSync(zipPath);
	const trailer = Buffer.alloc(80);
	trailer.write('IRSETUP1', 0, 'ascii');
	trailer.writeBigUInt64LE(BigInt(zip.length), 8);
	crypto.createHash('sha256').update(zip).digest().copy(trailer, 16);
	if (Buffer.byteLength(version) > 31) throw new Error('version too long: ' + version);
	trailer.write(version, 48, 'utf8');
	fs.writeFileSync(outPath, Buffer.concat([fs.readFileSync(stub), zip, trailer]));
	return outPath;
}

module.exports = {packInstaller};

if (require.main === module) {
	const [zip, version, out] = process.argv.slice(2);
	if (!zip || !version || !out) {
		console.error('usage: node installer/pack.js <zip> <version> <output.exe>');
		process.exit(2);
	}
	console.log('installer: ' + packInstaller(zip, version, out));
}
