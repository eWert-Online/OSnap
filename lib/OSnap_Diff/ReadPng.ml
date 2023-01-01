external read_png_buffer
  :  string
  -> int
  -> int * int * (int32, Bigarray.int32_elt, Bigarray.c_layout) Bigarray.Array1.t
  = "read_png_buffer"
