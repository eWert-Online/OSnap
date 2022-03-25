open Bigarray;
open Odiff;

type data = Array1.t(int32, int32_elt, c_layout);

module PNG = {
  type t = data;

  let readDirectPixel = (~x: int, ~y: int, img: ImageIO.img(t)) => {
    let image: data = img.image;
    Array1.unsafe_get(image, y * img.width + x);
  };

  let setImgColor = (~x, ~y, color, img: ImageIO.img(t)) => {
    let image: data = img.image;
    Array1.unsafe_set(image, y * img.width + x, color);
  };

  let loadImage = (buffer): ImageIO.img(t) => {
    let (width, height, data, _buffer) = ReadPng.read_png_buffer(buffer);

    {width, height, image: data};
  };

  let saveImage = (img: ImageIO.img(t), filename) => {
    WritePng.write_png_bigarray(filename, img.image, img.width, img.height);
  };

  let freeImage = (_img: ImageIO.img(t)) => {
    ();
  };

  let makeSameAsLayout = (img: ImageIO.img(t)) => {
    let image = Array1.create(int32, c_layout, Array1.dim(img.image));
    {...img, image};
  };
};
