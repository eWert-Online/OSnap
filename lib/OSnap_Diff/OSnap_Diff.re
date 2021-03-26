module Io = OSnap_Diff_Io;
module Diff = Odiff.Diff.MakeDiff(Io.PNG, Io.PNG);

type failState =
  | Pixel(int)
  | Layout;

let transparent = Color.{
                    color: {
                      r: 255,
                      g: 255,
                      b: 255,
                    },
                    alpha: 0,
                  };

let diff = (~output, ~diffPixel=(255, 0, 0), ~threshold=0, path1, path2) => {
  let original_image = Io.PNG.loadImage(path1);
  let new_image = Io.PNG.loadImage(path2);

  Diff.diff(
    original_image,
    new_image,
    ~outputDiffMask=true,
    ~threshold=0.1,
    ~failOnLayoutChange=true,
    ~diffPixel,
    (),
  )
  |> (
    fun
    | Pixel((_diffImg, diffCount)) when diffCount <= threshold => Result.ok()
    | Layout => Result.error(Layout)
    | Pixel((diff_mask, diffCount)) => {
        let original_image: Rgba32.t = Obj.magic(original_image.image);
        let new_image: Rgba32.t = Obj.magic(new_image.image);

        let diff_mask: Rgba32.t = Obj.magic(diff_mask.image);
        let diff_image = Rgba32.copy(original_image);
        for (x in 0 to diff_image.width - 1) {
          for (y in 0 to diff_image.height - 1) {
            let Color.{color, alpha} = Rgba32.get(diff_image, x, y);
            let mono = min(255, Color.brightness(color) + 50);
            let mono_color =
              Color.{
                color: {
                  r: mono,
                  g: mono,
                  b: mono,
                },
                alpha,
              };

            mono_color
            |> Color.Rgba.merge(Rgba32.get(diff_mask, x, y))
            |> Rgba32.set(diff_image, x, y);
          };
        };

        let complete_image =
          Rgba32.make(
            original_image.width + diff_image.width + new_image.width + 2,
            original_image.height
            |> max(diff_image.height)
            |> max(new_image.height),
            transparent,
          );

        // Place original image on the completed image
        Rgba32.blit(
          original_image,
          0,
          0,
          complete_image,
          0,
          0,
          original_image.width,
          original_image.height,
        );

        // Place diff image on the completed image right next to the original image with a space of 1px
        let destination_x = original_image.width + 1;
        Rgba32.blit(
          diff_image,
          0,
          0,
          complete_image,
          destination_x,
          0,
          diff_image.width,
          diff_image.height,
        );

        // Place new image on the completed image right next to the diff image with a space of 1px
        let destination_x = original_image.width + diff_image.width + 2;
        Rgba32.blit(
          new_image,
          0,
          0,
          complete_image,
          destination_x,
          0,
          new_image.width,
          new_image.height,
        );

        Png.save(output, [], Images.Rgba32(complete_image));

        Result.error(Pixel(diffCount));
      }
  );
};
