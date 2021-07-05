module Io = OSnap_Diff_Io;
module Diff = Odiff.Diff.MakeDiff(Io.PNG, Io.PNG);

type failState =
  | Pixel(int, float)
  | Layout;

let blit = (~dst, ~src, ~offset) => {
  open Bigarray;

  let exec = (src, dst) => {
    let len_src = Array2.dim1(src);

    for (i in 0 to len_src - 1) {
      let src = Array2.slice_left(src, i);
      let dst =
        Array1.sub(Array2.slice_left(dst, i + offset), 0, Array1.dim(src));

      Array1.blit(src, dst);
    };
  };

  let blit_channel = (src, dst) => {
    switch (src, dst) {
    | (Image.Pixmap.Pix8(src), Image.Pixmap.Pix8(dst)) => exec(src, dst)
    | (Image.Pixmap.Pix16(src), Image.Pixmap.Pix16(dst)) => exec(src, dst)
    | (Image.Pixmap.Pix8(_a), Image.Pixmap.Pix16(_b)) => ()
    | (Image.Pixmap.Pix16(_a), Image.Pixmap.Pix8(_b)) => ()
    };
  };

  switch (src, dst) {
  | (Image.Grey(src), Image.Grey(dst)) => blit_channel(src, dst)
  | (Image.Grey(_), Image.GreyA(_, _)) => ()
  | (Image.Grey(_), Image.RGB(_, _, _)) => ()
  | (Image.Grey(_), Image.RGBA(_, _, _, _)) => ()
  | (Image.GreyA(_, _), Image.Grey(_)) => ()
  | (Image.GreyA(src1, src2), Image.GreyA(dst1, dst2)) =>
    blit_channel(src1, dst1);
    blit_channel(src2, dst2);
  | (Image.GreyA(_, _), Image.RGB(_, _, _)) => ()
  | (Image.GreyA(_, _), Image.RGBA(_, _, _, _)) => ()
  | (Image.RGB(_, _, _), Image.Grey(_)) => ()
  | (Image.RGB(_, _, _), Image.GreyA(_, _)) => ()
  | (Image.RGB(src1, src2, src3), Image.RGB(dst1, dst2, dst3)) =>
    blit_channel(src1, dst1);
    blit_channel(src2, dst2);
    blit_channel(src3, dst3);
  | (Image.RGB(_, _, _), Image.RGBA(_, _, _, _)) => ()
  | (Image.RGBA(_, _, _, _), Image.Grey(_)) => ()
  | (Image.RGBA(_, _, _, _), Image.GreyA(_, _)) => ()
  | (Image.RGBA(_, _, _, _), Image.RGB(_, _, _)) => ()
  | (Image.RGBA(src1, src2, src3, src4), Image.RGBA(dst1, dst2, dst3, dst4)) =>
    blit_channel(src1, dst1);
    blit_channel(src2, dst2);
    blit_channel(src3, dst3);
    blit_channel(src4, dst4);
  };
};

let diff =
    (
      ~output,
      ~diffPixel=(255, 0, 0),
      ~ignoreRegions=[],
      ~threshold=0,
      ~original_image_data,
      ~new_image_data,
      (),
    ) => {
  let original_image = Io.PNG.loadImage(original_image_data);
  let new_image = Io.PNG.loadImage(new_image_data);

  Diff.diff(
    original_image,
    new_image,
    ~outputDiffMask=true,
    ~threshold=0.1,
    ~failOnLayoutChange=true,
    ~antialiasing=true,
    ~ignoreRegions,
    ~diffPixel,
    (),
  )
  |> (
    fun
    | Pixel((_, diffCount, _)) when diffCount <= threshold => Result.ok()
    | Layout => Result.error(Layout)
    | Pixel((diff_mask, diffCount, diffPercentage)) => {
        let original_image = original_image.image;
        let diff_mask = diff_mask.image;
        let new_image = new_image.image;
        let border_width = 5;

        let complete_width =
          original_image.width
          + border_width
          + diff_mask.width
          + border_width
          + new_image.width;

        let complete_height =
          original_image.height
          |> max(diff_mask.height)
          |> max(new_image.height);

        let complete_image =
          Image.create_rgb(~alpha=true, complete_width, complete_height);
        Image.fill_rgb(complete_image, 0, 0, 0, ~alpha=0);

        let offset = 0;
        blit(~src=original_image.pixels, ~dst=complete_image.pixels, ~offset);

        for (y in 0 to diff_mask.height - 1) {
          for (x in 0 to diff_mask.width - 1) {
            Image.read_rgba(diff_mask, x, y, (_r, _g, _b, a) =>
              if (a != 255) {
                Image.read_rgba(
                  original_image,
                  x,
                  y,
                  (r, g, b, a) => {
                    let brightness = (r * 54 + g * 182 + b * 19) / 255;
                    let mono = min(255, brightness + 80);
                    Image.write_rgba(diff_mask, x, y, mono, mono, mono, a);
                  },
                );
              }
            );
          };
        };
        let offset = offset + original_image.width + border_width;
        blit(~src=diff_mask.pixels, ~dst=complete_image.pixels, ~offset);

        let offset = offset + diff_mask.width + border_width;
        blit(~src=new_image.pixels, ~dst=complete_image.pixels, ~offset);

        ImageLib.PNG.write(
          ImageUtil_unix.chunk_writer_of_path(output),
          complete_image,
        );

        Result.error(Pixel(diffCount, diffPercentage));
      }
  );
};
