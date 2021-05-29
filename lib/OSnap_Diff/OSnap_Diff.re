module Io = OSnap_Diff_Io;
module Diff = Odiff.Diff.MakeDiff(Io.PNG_String, Io.PNG_String);

type failState =
  | Pixel(int, float)
  | Layout;

type point_rect = {
  min_x: int,
  max_x: int,
  min_y: int,
  max_y: int,
};

let is_in_rect = (x, y, rect) => {
  x >= rect.min_x && x <= rect.max_x && y >= rect.min_y && y <= rect.max_y;
};

let diff =
    (
      ~output,
      ~diffPixel=(255, 0, 0),
      ~threshold=0,
      ~original_image_data,
      ~new_image_data,
      (),
    ) => {
  let original_image = Io.PNG_String.loadImage(original_image_data);
  let new_image = Io.PNG_String.loadImage(new_image_data);

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

        let original_rect = {
          min_x: 1,
          min_y: 1,
          max_x: original_image.width,
          max_y: original_image.height,
        };
        let diff_rect = {
          min_x: original_rect.max_x + border_width + 1,
          min_y: 1,
          max_x: original_rect.max_x + border_width + diff_mask.width,
          max_y: diff_mask.height,
        };
        let new_rect = {
          min_x: diff_rect.max_x + border_width + 1,
          min_y: 1,
          max_x: diff_rect.max_x + border_width + new_image.width,
          max_y: new_image.height,
        };

        for (y in 1 to complete_height) {
          for (x in 1 to complete_width) {
            let write = Image.write_rgba(complete_image, x - 1, y - 1);
            let is_in_rect = is_in_rect(x, y);

            if (is_in_rect(original_rect)) {
              let read_x = x - original_rect.min_x;
              let read_y = y - original_rect.min_y;

              let (r, g, b, a) =
                Io.PNG_String.readDirectPixel(
                  ~x=read_x,
                  ~y=read_y,
                  original_image,
                );

              write(r, g, b, a);
            } else if (is_in_rect(diff_rect)) {
              let read_x = x - diff_rect.min_x;
              let read_y = y - diff_rect.min_y;

              let (r, g, b, a) =
                Io.PNG_String.readDirectPixel(
                  ~x=read_x,
                  ~y=read_y,
                  diff_mask,
                );

              if (a != 0) {
                write(r, g, b, a);
              } else {
                let (r, g, b, a) =
                  Io.PNG_String.readDirectPixel(
                    ~x=read_x,
                    ~y=read_y,
                    original_image,
                  );
                let brightness = (r * 54 + g * 182 + b * 19) / 255;
                let mono = min(255, brightness + 80);
                write(mono, mono, mono, a);
              };
            } else if (is_in_rect(new_rect)) {
              let read_x = x - new_rect.min_x;
              let read_y = y - new_rect.min_y;

              let (r, g, b, a) =
                Io.PNG_String.readDirectPixel(
                  ~x=read_x,
                  ~y=read_y,
                  new_image,
                );
              write(r, g, b, a);
            } else {
              write(0, 0, 0, 0);
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
