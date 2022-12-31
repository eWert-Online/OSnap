type compression_method =
  | Stored
  | Deflated

type entry =
  { lastmod_time : int
  ; lastmod_date : int
  ; crc : int32
  ; compressed_size : int
  ; uncompressed_size : int
  ; file_offset : int64
  ; name : string
  ; extra : string
  ; comment : string
  ; compression_method : compression_method
  ; is_directory : bool
  }

type in_file =
  { if_filename : string
  ; if_channel : in_channel
  ; if_entries : entry list
  ; if_directory : (string, entry) Hashtbl.t
  ; if_comment : string
  }

val open_in : string -> in_file
val close_in : in_file -> unit
val entries : in_file -> entry list
val copy_entry_to_channel : in_file -> entry -> out_channel -> unit
