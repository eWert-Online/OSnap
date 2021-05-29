open Odiff;

module PNG_Common = {
  type t = {image: Image.image};
  type row = int;

  let readRow = (_, y) => y;

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
    Image.fill_rgb(image, 0, 0, 0, ~alpha=0);

    {
      ...img,
      image: {
        image: image,
      },
    };
  };
};

module PNG_File: ImageIO.ImageIO = {
  include PNG_Common;

  let loadImage = (filename): ImageIO.img(t) => {
    let image = filename |> ImageLib_unix.openfile;

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
};

module PNG_String: ImageIO.ImageIO = {
  include PNG_Common;

  let loadImage = (data): ImageIO.img(t) => {
    let image =
      data |> ImageUtil.chunk_reader_of_string |> ImageLib.PNG.parsefile;

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
};
