var path = require('path');
var cp = require('child_process');
var fs = require('fs');
var os = require('os');
var platform = process.platform;

var packageJson = require('./package.json');
var binariesToCopy = Object.keys(packageJson.bin).map(function (name) {
  return packageJson.bin[name];
});

function copyRecursive(srcDir, dstDir) {
  fs.readdirSync(srcDir).forEach(function (file) {
    var src = path.join(srcDir, file);
    var dst = path.join(dstDir, file);

    var stat = fs.statSync(src);
    if (stat && stat.isDirectory()) {
      try {
        fs.mkdirSync(dst);
      } catch (e) {
        console.log('directory already exists: ' + dst);
        console.error(e);
      }
      copyRecursive(src, dst);
    } else {
      try {
        fs.copyFileSync(src, dst);
      } catch (e) {
        console.log("could't copy file " + src + ' to ' + dst);
        console.error(e);
      }
    }
  });
}

/**
 * Since os.arch returns node binary's target arch, not
 * the system arch.
 * Credits: https://github.com/feross/arch/blob/af080ff61346315559451715c5393d8e86a6d33c/index.js#L10-L58
 */

function arch() {
  /**
   * The running binary is 64-bit, so the OS is clearly 64-bit.
   */
  if (process.arch === 'x64') {
    return 'x64';
  }

  /**
   * All recent versions of Mac OS are 64-bit.
   */
  if (process.platform === 'darwin') {
    return 'x64';
  }

  /**
   * On Windows, the most reliable way to detect a 64-bit OS from within a 32-bit
   * app is based on the presence of a WOW64 file: %SystemRoot%\SysNative.
   * See: https://twitter.com/feross/status/776949077208510464
   */
  if (process.platform === 'win32') {
    var useEnv = false;
    try {
      useEnv = !!(process.env.SYSTEMROOT && fs.statSync(process.env.SYSTEMROOT));
    } catch (err) {}

    var sysRoot = useEnv ? process.env.SYSTEMROOT : 'C:\\Windows';

    // If %SystemRoot%\SysNative exists, we are in a WOW64 FS Redirected application.
    var isWOW64 = false;
    try {
      isWOW64 = !!fs.statSync(path.join(sysRoot, 'sysnative'));
    } catch (err) {}

    return isWOW64 ? 'x64' : 'x86';
  }

  /**
   * On Linux, use the `getconf` command to get the architecture.
   */
  if (process.platform === 'linux') {
    var output = cp.execSync('getconf LONG_BIT', { encoding: 'utf8' });
    return output === '64\n' ? 'x64' : 'x86';
  }

  /**
   * If none of the above, assume the architecture is 32-bit.
   */
  return 'x86';
}

var copyPlatformBinaries = (platformPath) => {
  var platformBuildPath = path.join(__dirname, 'platform-' + platformPath);

  var sourcePath = path.join(platformBuildPath, 'bin');
  var destPath = path.join(__dirname, 'bin');
  copyRecursive(sourcePath, destPath);

  binariesToCopy.forEach((binaryPath) => {
    var sourcePath = path.join(platformBuildPath, binaryPath);
    var destPath = path.join(__dirname, binaryPath);
    if (fs.existsSync(destPath)) {
      fs.unlinkSync(destPath);
    }
    fs.copyFileSync(sourcePath, destPath);
    fs.chmodSync(destPath, 0777);
  });
};

switch (platform) {
  case 'win32':
    if (arch() !== 'x64') {
      console.warn('error: x86 is currently not supported on Windows');
      process.exit(1);
    }

    copyPlatformBinaries('windows-x64');
    break;
  case 'linux':
  case 'darwin':
    copyPlatformBinaries(platform);
    break;
  default:
    console.warn('error: no release built for the ' + platform + ' platform');
    process.exit(1);
}
