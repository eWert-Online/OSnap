open Bigarray
open Odiff

type data = (int32, int32_elt, c_layout) Array1.t

module PngIo = PngIo

module IO = struct
  type t = data

  let readRawPixelAtOffset offset (img : t Odiff.ImageIO.img) =
    let image = (img.image : data) in
    Array1.unsafe_get image offset
  [@@inline always]
  ;;

  let readRawPixel ~(x : int) ~(y : int) (img : t Odiff.ImageIO.img) =
    let image : data = img.image in
    Array1.unsafe_get image ((y * img.width) + x)
  [@@inline always]
  ;;

  let setImgColor ~x ~y color (img : t ImageIO.img) =
    let image = (img.image : data) in
    Array1.set image ((y * img.width) + x) color
  ;;

  let loadImage buffer : t ImageIO.img =
    let width, height, data = PngIo.read_png_buffer buffer (String.length buffer) in
    { width; height; image = data }
  ;;

  let saveImage (img : t ImageIO.img) filename =
    PngIo.write_png_bigarray filename img.image img.width img.height
  ;;

  let freeImage (img : t ImageIO.img) = ()

  let makeSameAsLayout (img : t ImageIO.img) =
    let image = Array1.create int32 c_layout (Array1.dim img.image) in
    let () = Array1.fill image 0x00000000l in
    { img with image }
  ;;
end
