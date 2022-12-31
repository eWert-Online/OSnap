open Bigarray
open Odiff

type data = (int32, int32_elt, c_layout) Array1.t

module PNG = struct
  type t = data

  let readDirectPixel ~(x : int) ~(y : int) (img : t ImageIO.img) =
    let image = (img.image : data) in
    Array1.unsafe_get image ((y * img.width) + x)
  ;;

  let setImgColor ~x ~y color (img : t ImageIO.img) =
    let image = (img.image : data) in
    Array1.unsafe_set image ((y * img.width) + x) color
  ;;

  let loadImage buffer : t ImageIO.img =
    let width, height, data = ReadPng.read_png_buffer buffer (String.length buffer) in
    { width; height; image = data }
  ;;

  let saveImage (img : t ImageIO.img) filename =
    WritePng.write_png_bigarray filename img.image img.width img.height
  ;;

  let freeImage (_img : t ImageIO.img) = ()

  let makeSameAsLayout (img : t ImageIO.img) =
    let image = Array1.create int32 c_layout (Array1.dim img.image) in
    { img with image }
  ;;
end
