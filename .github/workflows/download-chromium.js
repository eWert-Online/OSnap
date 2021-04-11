var path = require("path");
var cp = require("child_process");

var command = path.resolve(__dirname, "bin", "downloadChromium");

cp.spawnSync(command, [], {
  cwd: process.cwd(),
  env: process.env,
  stdio: [process.stdin, process.stdout, process.stderr],
  encoding: "utf-8",
});
