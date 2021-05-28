open Odiff;
open Odiff.ImageIO;

module PNG: ImageIO.ImageIO = {
  type t = {image: Image.image};
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
        image: image,
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
    Image.read_rgba(img.image.image, x, y, (r, g, b, a) => (r, g, b, a));
  };

  let readImgColor = (x, y, img: ImageIO.img(t)) => {
    readDirectPixel(~x, ~y, img);
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
        image: image,
      },
    };
  };
};
