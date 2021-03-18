module Io = OSnap_Diff_Io;
module Diff = Odiff.Diff.MakeDiff(Io.PNG, Io.PNG);

type failState =
  | Pixel(int)
  | Layout;

let diff = (~output, ~diffPixel=(255, 0, 0), ~threshold=0.1, path1, path2) => {
  let img1 = Io.PNG.loadImage(path1);
  let img2 = Io.PNG.loadImage(path2);

  Diff.diff(
    img1,
    img2,
    ~outputDiffMask=false,
    ~threshold,
    ~failOnLayoutChange=true,
    ~diffPixel,
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
