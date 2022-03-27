open Bigarray;
open Odiff;

type data = Array1.t(int32, int32_elt, c_layout);

module PNG: ImageIO.ImageIO with type t = data;
