module Io = OSnap_Diff_Io;
module Diff = Odiff.Diff.MakeDiff(Io.PNG, Io.PNG);

type failState =
  | Pixel(int)
  | Layout;

let load = path => {
  switch (Images.load(path, [])) {
  | Index8(i8img) => Index8.to_rgba32(i8img)
  | Rgb24(rgba24img) => Rgb24.to_rgba32(rgba24img)
  | Rgba32(img) => img
  | Index16(_) => raise(Odiff.ImageIO.ImageNotLoaded)
  | Cmyk32(_) => raise(Odiff.ImageIO.ImageNotLoaded)
  };
};

let diff = (~output, ~diffPixel=(255, 0, 0), ~threshold=0.1, path1, path2) => {
  Diff.diff(
    Io.PNG.loadImage(path1),
    Io.PNG.loadImage(path2),
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
        let img1 = load(path1);
        let img2 = load(path2);
        let width = img1.width + diffImg.width + img2.width + 2;
        let height = img1.height |> max(diffImg.height) |> max(img2.height);
        let image =
          Rgba32.make(
            width,
            height,
            Color.{
              color: {
                r: 255,
                g: 255,
                b: 255,
              },
              alpha: 0,
            },
          );

        let source = img1;
        let source_x = 0;
        let source_y = 0;
        let destination = image;
        let destination_x = 0;
        let destination_y = 0;
        let width = img1.width;
        let height = img1.height;
        Rgba32.blit(
          source,
          source_x,
          source_y,
          destination,
          destination_x,
          destination_y,
          width,
          height,
        );

        let source = Obj.magic(diffImg.image);
        let destination_x = img1.width + 1;
        let width = diffImg.width;
        let height = diffImg.height;
        Rgba32.blit(
          source,
          source_x,
          source_y,
          destination,
          destination_x,
          destination_y,
          width,
          height,
        );

        let source = img2;
        let destination_x = img1.width + diffImg.width + 2;
        let width = img2.width;
        let height = img2.height;
        Rgba32.blit(
          source,
          source_x,
          source_y,
          destination,
          destination_x,
          destination_y,
          width,
          height,
        );

        Png.save(output, [], Images.Rgba32(image));

        Result.error(Pixel(diffCount));
      }
  );
};
