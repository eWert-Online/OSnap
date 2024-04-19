const fs = require('fs');
const path = require('path');

const releaseDir = path.join(__dirname, '..', '..', '_release');

console.log('Cleanup _release directory');

fs.rmSync(releaseDir, { recursive: true, force: true });
fs.mkdirSync(releaseDir);

console.log('Creating package.json');
// From the project root pwd
const mainPackageJsonPath = 'package.json';

const exists = fs.existsSync(mainPackageJsonPath);
if (!exists) {
  console.error('No package.json at ' + mainPackageJsonPath);
  process.exit(1);
}
// Now require from this script's location.
const mainPackageJson = require(path.join('..', '..', mainPackageJsonPath));
const bins = {
  osnap: 'bin/osnap',
};

const packageJson = JSON.stringify(
  {
    name: mainPackageJson.name,
    version: mainPackageJson.version,
    license: mainPackageJson.license,
    description: mainPackageJson.description,
    repository: mainPackageJson.repository,
    keywords: mainPackageJson.keywords,
    scripts: {
      postinstall: 'node ./postinstall.js',
    },
    bin: bins,
    files: [
      'bin/',
      'postinstall.js',
      'platform-linux/',
      'platform-darwin/',
      // 'platform-windows-x64/',
    ],
  },
  null,
  2
);

fs.writeFileSync(path.join(__dirname, '..', '..', '_release', 'package.json'), packageJson, {
  encoding: 'utf8',
});

try {
  console.log('Copying LICENSE');
  fs.copyFileSync(path.join(__dirname, '..', '..', 'LICENSE'), path.join(__dirname, '..', '..', '_release', 'LICENSE'));
} catch (e) {
  console.warn('No LICENSE found');
}

console.log('Copying README.md');
fs.copyFileSync(
  path.join(__dirname, '..', '..', 'README.md'),
  path.join(__dirname, '..', '..', '_release', 'README.md')
);

console.log('Copying postinstall.js');
fs.copyFileSync(
  path.join(__dirname, 'release-postinstall.js'),
  path.join(__dirname, '..', '..', '_release', 'postinstall.js')
);

console.log('Creating placeholder files');
const placeholderFile = `:; echo "You need to have postinstall enabled"; exit $?
@ECHO OFF
ECHO You need to have postinstall enabled`;
fs.mkdirSync(path.join(__dirname, '..', '..', '_release', 'bin'));

Object.keys(bins).forEach((name) => {
  if (bins[name]) {
    const binPath = path.join(__dirname, '..', '..', '_release', bins[name]);
    fs.writeFileSync(binPath, placeholderFile);
    fs.chmodSync(binPath, 0777);
  } else {
    console.log('bins[name] name=' + name + ' was empty. Weird.');
    console.log(bins);
  }
});
