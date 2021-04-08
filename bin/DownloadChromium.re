// /** ====================== DOWNLOAD CHROMIUM ===================== */
// function getDownloadURL() {
//   var basePath = "https://storage.googleapis.com/chromium-browser-snapshots";
//   var chromeRevision = "856583";
//   switch (platform) {
//     case "win32":
//       if (arch() !== "x64") {
//         return basePath + "/Win_x64/" + chromeRevision + "/chrome-win.zip";
//       }
//       return basePath + "/Win/" + chromeRevision + "/chrome-win.zip";
//       break;
//     case "linux":
//       return basePath + "/Linux_x64/" + chromeRevision + "/chrome-linux.zip";
//     case "darwin":
//       return basePath + "/Mac/" + chromeRevision + "/chrome-mac.zip";
//     default:
//       console.warn("error: no release built for the " + platform + " platform");
//       process.exit(1);
//   }
// }
// var unzipper = require("unzipper");
// var chromiumFolder = path.join(__dirname, "./.local-chromium");
// fs.mkdirSync(chromiumFolder);
// var url = getDownloadURL();
// var zipPath = fs.createWriteStream(path.join(__dirname, "chromium.zip"));
// return new Promise((resolve, reject) => {
//   got.stream(url, utils.getRequestOptions(url))
//       .on('downloadProgress', onProgress)
//       .on('error', error => {
//           console.error('An error occurred while trying to download file', error.message);
//           reject(error);
//       })
//       .pipe(fs.createWriteStream(destPath))
//       .on('error', error => {
//           console.error('An error occurred while trying to save file to disk', error);
//           reject(error);
//       })
//       .on('finish', () => {
//           resolve(destPath);
//       });
// });
// const request = require("https").get(url, function (response) {
//   response.pipe(zipPath);
//   unzip.Open.file(zipPath).then(function (d) {
//     d.extract({ path: "/extraction/path", concurrency: 5 });
//     fs.unlinkSync(zipPath);
//   });
// });
