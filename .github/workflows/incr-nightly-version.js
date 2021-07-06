const { execSync } = require("child_process");
const path = require("path");
const fs = require("fs");

const args = process.argv.slice(2);

let packageJsonPath = path.join(__dirname, "../../package.json");
if (args[0] != null) {
  packageJsonPath = path.resolve(args[0]);
}

const pkgJsonData = JSON.parse(fs.readFileSync(packageJsonPath, "utf8"));

const commit = execSync("git rev-parse --verify HEAD").toString();
const version = `${pkgJsonData.version}-nightly.${commit.slice(0, 6)}`;

pkgJsonData.version = version;
fs.writeFileSync(packageJsonPath, JSON.stringify(pkgJsonData, null, 2));

console.log(pkgJsonData.version);
