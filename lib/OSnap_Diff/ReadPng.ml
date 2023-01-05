type buffer = (int32, Bigarray.int32_elt, Bigarray.c_layout) Bigarray.Array1.t

external read_png_buffer : string -> int -> int * int * buffer = "read_png_buffer"
external free_png_buffer : buffer -> 'void = "caml_ba_finalize"
