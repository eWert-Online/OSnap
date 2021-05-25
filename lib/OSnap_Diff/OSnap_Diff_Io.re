open Odiff;
open Odiff.ImageIO;
open Bigarray;

module PNG: ImageIO.ImageIO = {
  type t = {
    image: Image.image,
    pixel_cache: Array1.t(int, int_elt, c_layout),
  };
  type row = int;

  let readRow = (_, y) => y;

  let loadImage = (filename): ImageIO.img(t) => {
    let image = ImageLib_unix.openfile(filename);
    let width = image.Image.width;
    let height = image.Image.height;

    {
      width,
      height,
      image: {
        image,
        pixel_cache: Array1.create(int, c_layout, width * height),
      },
    };
  };

  let saveImage = (img: ImageIO.img(t), filename) => {
    ImageLib.PNG.write(
      ImageUtil_unix.chunk_writer_of_path(filename),
      img.image.image,
    );
  };

  let readDirectPixel = (~x, ~y, img: ImageIO.img(t)) => {
    let index = y * img.width + x;
    let color = Array1.get(img.image.pixel_cache, index);
    let a = color lsr 24 land 0xFF;
    let r = color lsr 16 land 0xFF;
    let g = color lsr 8 land 0xFF;
    let b = color land 0xFF;

    (r, g, b, a);
  };

  let readImgColor = (x, y, img: ImageIO.img(t)) => {
    Image.read_rgba(
      img.image.image,
      x,
      y,
      (r, g, b, a) => {
        let alpha_channel = (a land 0xFF) lsl 24;
        let blue_channel = (b land 0xFF) lsl 16;
        let green_channel = (g land 0xFF) lsl 8;
        let red_channel = r land 0xFF;

        let color =
          alpha_channel lor blue_channel lor green_channel lor red_channel;
        let index = y * img.width + x;
        Array1.set(img.image.pixel_cache, index, color);

        (r, g, b, a);
      },
    );
  };

  let setImgColor = (x, y, (r, g, b), img: ImageIO.img(t)) => {
    Image.write_rgba(img.image.image, x, y, r, g, b, 255);
  };

  let freeImage = _ => ();

  let makeSameAsLayout = (img: ImageIO.img(t)) => {
    let image = Image.create_rgb(~alpha=true, img.width, img.height);

    {
      ...img,
      image: {
        ...img.image,
        image,
      },
    };
  };
};
