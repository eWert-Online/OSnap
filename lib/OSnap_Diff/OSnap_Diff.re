module Io = OSnap_Diff_Io;
module Diff = Odiff.Diff.MakeDiff(Io.PNG, Io.PNG);

type failState =
  | Pixel(int, float)
  | Layout;

let merge_rgba = (src, dst) => {
  let (src_r, src_g, src_b, src_a) = src;
  let (dst_r, dst_g, dst_b, dst_a) = dst;
  if (src_a == 0) {
    dst;
  } else if (src_a == 255) {
    src;
  } else {
    let alpha = 255 - src_a;
    let r =
      alpha * dst_r * dst_a / 255 + src_r * src_a |> min(255) |> max(0);
    let g =
      alpha * dst_g * dst_a / 255 + src_g * src_a |> min(255) |> max(0);
    let b =
      alpha * dst_b * dst_a / 255 + src_b * src_a |> min(255) |> max(0);
    let a = 255 - alpha * (255 - dst_a) / 255 |> min(255) |> max(0);

    (r, g, b, a);
  };
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
    ~antialiasing=true,
    ~diffPixel,
    (),
  )
  |> (
    fun
    | Pixel((_diffImg, diffCount, _diffPercentage))
        when diffCount <= threshold =>
      Result.ok()
    | Layout => Result.error(Layout)
    | Pixel((diff_mask, diffCount, diffPercentage)) => {
        let complete_width =
          original_image.width + diff_mask.width + new_image.width;
        let complete_height =
          original_image.height
          |> max(diff_mask.height)
          |> max(new_image.height);
        let complete_image =
          Image.create_rgb(~alpha=true, complete_width, complete_height);

        for (y in 0 to complete_height - 1) {
          let diff_row = Io.PNG.readRow(diff_mask, y);
          for (x in 0 to complete_width - 1) {
            let write = Image.write_rgba(complete_image, x, y);
            if (x < original_image.width) {
              if (y < original_image.height) {
                let (r, g, b, a) =
                  Io.PNG.readDirectPixel(~x, ~y, original_image);
                write(r, g, b, a);
              };
            } else if (x >= original_image.width
                       - 1
                       && x < diff_mask.width
                       + original_image.width) {
              if (y < diff_mask.height) {
                let read_x = x - original_image.width;

                let (r, g, b, a) =
                  Io.PNG.readDirectPixel(~x=read_x, ~y, original_image);
                let brightness = (r * 54 + g * 182 + b * 19) / 255;
                let mono = min(255, brightness + 80);
                let mono = (mono, mono, mono, a);

                let diff_mask_color =
                  Io.PNG.readImgColor(read_x, diff_row, diff_mask);
                let (r, g, b, a) = mono |> merge_rgba(diff_mask_color);
                write(r, g, b, a);
              };
            } else if (x >= original_image.width + diff_mask.width) {
              if (y < new_image.height) {
                let read_x = x - (original_image.width + diff_mask.width);

                let (r, g, b, a) =
                  Io.PNG.readDirectPixel(~x=read_x, ~y, new_image);
                write(r, g, b, a);
              };
            } else {
              ();
            };
          };
        };

        ImageLib.PNG.write(
          ImageUtil_unix.chunk_writer_of_path(output),
          complete_image,
        );

        Result.error(Pixel(diffCount, diffPercentage));
      }
  );
};
