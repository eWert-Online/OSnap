module Io = OSnap_Diff_Io;
module Diff = Odiff.Diff.MakeDiff(Io.PNG, Io.PNG);

type failState =
  | Pixel(int)
  | Layout;

let diff = (~output, path1, path2) => {
  let img1 = Io.PNG.loadImage(path1);
  let img2 = Io.PNG.loadImage(path2);

  if (img1.width != img2.width || img1.height != img2.height) {
    Result.error(Layout);
  } else {
    Diff.diff(
      img1,
      img2,
      ~outputDiffMask=false,
      ~threshold=0.1,
      ~failOnLayoutChange=true,
      ~diffPixel=(255, 0, 0),
      (),
    )
    |> (
      fun
      | Pixel((_diffImg, diffCount)) when diffCount == 0 => Result.ok()
      | Layout => Result.error(Layout)
      | Pixel((diffImg, diffCount)) => {
          Io.PNG.saveImage(diffImg, output);
          Result.error(Pixel(diffCount));
        }
    );
  };
};
