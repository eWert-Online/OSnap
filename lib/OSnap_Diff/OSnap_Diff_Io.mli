open Bigarray
open Odiff

type data = (int32, int32_elt, c_layout) Array1.t

module PNG : ImageIO.ImageIO with type t = data
