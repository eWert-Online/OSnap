module Io = OSnap_Diff_Io;
module Diff = Odiff.Diff.MakeDiff(Io.PNG, Io.PNG);
open Bigarray;

type failState =
  | Pixel(int, float)
  | Layout;

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
        let original_image = original_image;
        let diff_mask = diff_mask;
        let new_image = new_image;
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

        let original_image_start = 0;
        let original_image_end = original_image.width;

        let diff_mask_start = original_image_end + border_width;
        let diff_mask_end = diff_mask_start + diff_mask.width;

        let new_image_start = diff_mask_end + border_width;
        let new_image_end = new_image_start + new_image.width;

        let complete_image =
          Array1.create(
            int32,
            c_layout,
            complete_width * complete_height * 4,
          );

        let size = Array1.dim(complete_image) / 4;
        let row = ref(-1);
        for (offset in 0 to size) {
          let col = offset mod complete_width;
          if (col == 0) {
            incr(row);
          };

          let fill_with =
            if (col >= original_image_start && col < original_image_end) {
              Array1.unsafe_get(
                original_image.image,
                row^ * original_image.width + col,
              );
            } else if (col > diff_mask_start && col < diff_mask_end) {
              let (>>) = Int32.shift_right;
              let (&) = Int32.logand;

              let pixel =
                Array1.unsafe_get(
                  diff_mask.image,
                  row^ * diff_mask.width + (col - diff_mask_start),
                );

              let alpha = pixel >> 24 & 0xFFl;

              if (alpha == 0xFFl) {
                pixel;
              } else {
                let pixel =
                  Array1.unsafe_get(
                    original_image.image,
                    row^ * original_image.width + (col - diff_mask_start),
                  )
                  |> Int32.to_int;

                let a = pixel lsr 24 land 0xFF;
                let b = pixel lsr 16 land 0xFF;
                let g = pixel lsr 8 land 0xFF;
                let r = pixel lsr 0 land 0xFF;

                let brightness = (r * 54 + g * 182 + b * 19) / 255;
                let mono = min(255, brightness + 80);
                let a = (a land 0xFF) lsl 24;
                let b = (mono land 0xFF) lsl 16;
                let g = (mono land 0xFF) lsl 8;
                let r = (mono land 0xFF) lsl 0;
                Int32.of_int(a lor b lor g lor r);
              };
            } else if (col > new_image_start && col <= new_image_end) {
              Array1.unsafe_get(
                new_image.image,
                row^ * new_image.width + (col - new_image_start),
              );
            } else {
              0xFFFFFFFFl;
              // AA BB GG RR
            };

          Array1.unsafe_set(complete_image, offset, fill_with);
        };

        WritePng.write_png_bigarray(
          output,
          complete_image,
          complete_width,
          complete_height,
        );

        Result.error(Pixel(diffCount, diffPercentage));
      }
  );
};
