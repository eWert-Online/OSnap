type buffer = (int32, Bigarray.int32_elt, Bigarray.c_layout) Bigarray.Array1.t

external read_png_buffer : string -> int -> int * int * buffer = "read_png_buffer"

external write_png_bigarray
  :  string
  -> buffer
  -> int
  -> int
  -> unit
  = "write_png_bigarray"
[@@noalloc]
