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
  ; if_channel : Stdlib.in_channel
  ; if_entries : entry list
  ; if_directory : (string, entry) Hashtbl.t
  ; if_comment : string
  }

let read_2_bytes ic =
  let b1 = input_byte ic in
  let b2 = input_byte ic in
  b1 lor (b2 lsl 8)
;;

let read_4_bytes ic =
  let lw = read_2_bytes ic in
  let hw = read_2_bytes ic in
  Int32.logor (Int32.of_int lw) (Int32.shift_left (Int32.of_int hw) 16)
;;

let read_4_bytes_int ic =
  let lw = read_2_bytes ic in
  let hw = read_2_bytes ic in
  if hw > max_int lsr 16 then raise (Failure "32-bit data too large");
  lw lor (hw lsl 16)
;;

let read_string ic len =
  let buf = Bytes.create len in
  really_input ic buf 0 len;
  Bytes.unsafe_to_string buf
;;

let int64_of_uint32 n = Int64.(logand (of_int32 n) 0xFFFF_FFFFL)

let find_last_occurrence (pattern : string) (buf : bytes) ofs len =
  let rec search i j =
    if i < ofs
    then -1
    else if j >= String.length pattern
    then i
    else if String.get pattern j = Bytes.get buf (i + j)
    then search i (j + 1)
    else search (i - 1) 0
  in
  search (ofs + len - String.length pattern) 0
;;

(* Reads a ZIP file from the given input channel and extracts information about its
   central directory. Returns a tuple containing the number of entries in the central
   directory, the size of the directory, the offset of the directory, and the comment
   field from the directory. *)
let read_ecd ic =
  let buf = Bytes.create 256 in
  let filelen = LargeFile.in_channel_length ic in
  let rec find_ecd pos len =
    (* On input, bytes 0 ... len - 1 of buf reflect what is at pos in ic *)
    if pos <= 0L || Int64.sub filelen pos >= 0x10000L
    then failwith "end of central directory not found, not a ZIP file";
    let toread = if pos >= 128L then 128 else Int64.to_int pos in
    (* Make room for "toread" extra bytes, and read them *)
    Bytes.blit buf 0 buf toread (256 - toread);
    let newpos = Int64.(sub pos (of_int toread)) in
    LargeFile.seek_in ic newpos;
    really_input ic buf 0 toread;
    let newlen = min (toread + len) 256 in
    (* Search for magic number *)
    let ofs = find_last_occurrence "PK\005\006" buf 0 newlen in
    if ofs < 0
       || newlen < 22
       ||
       let comment_len =
         Char.code (Bytes.get buf (ofs + 20))
         lor (Char.code (Bytes.get buf (ofs + 21)) lsl 8)
       in
       Int64.(add newpos (of_int (ofs + 22 + comment_len))) <> filelen
    then find_ecd newpos newlen
    else Int64.(add newpos (of_int ofs))
  in
  LargeFile.seek_in ic (find_ecd filelen 0);
  let magic = read_4_bytes ic in
  let disk_no = read_2_bytes ic in
  let cd_disk_no = read_2_bytes ic in
  let _disk_entries = read_2_bytes ic in
  let cd_entries = read_2_bytes ic in
  let cd_size = read_4_bytes ic in
  let cd_offset = read_4_bytes ic in
  let comment_len = read_2_bytes ic in
  let comment = read_string ic comment_len in
  assert (magic = Int32.of_int 0x06054b50);
  if disk_no <> 0 || cd_disk_no <> 0 then failwith "multi-disk ZIP files not supported";
  cd_entries, cd_size, cd_offset, comment
;;

let read_cd ic cd_entries cd_offset cd_bound =
  let cd_bound = int64_of_uint32 cd_bound in
  try
    LargeFile.seek_in ic (int64_of_uint32 cd_offset);
    let entries = ref [] in
    let entry_count = ref 0 in
    while LargeFile.pos_in ic < cd_bound do
      incr entry_count;
      let magic = read_4_bytes ic in
      let _version_made_by = read_2_bytes ic in
      let _version_needed = read_2_bytes ic in
      let flags = read_2_bytes ic in
      let compression_method =
        match read_2_bytes ic with
        | 0 -> Stored
        | 8 -> Deflated
        | _ -> raise (Failure "unknown compression method")
      in
      let lastmod_time = read_2_bytes ic in
      let lastmod_date = read_2_bytes ic in
      let crc = read_4_bytes ic in
      let compressed_size = read_4_bytes_int ic in
      let uncompressed_size = read_4_bytes_int ic in
      let name_len = read_2_bytes ic in
      let extra_len = read_2_bytes ic in
      let comment_len = read_2_bytes ic in
      let _disk_number = read_2_bytes ic in
      let _internal_attr = read_2_bytes ic in
      let _external_attr = read_4_bytes ic in
      let header_offset = int64_of_uint32 (read_4_bytes ic) in
      let name = read_string ic name_len in
      let extra = read_string ic extra_len in
      let comment = read_string ic comment_len in
      if magic <> Int32.of_int 0x02014b50
      then raise (Failure "wrong file header in central directory");
      if flags land 1 <> 0 then raise (Failure "encrypted entries not supported");
      entries
      := { name
         ; lastmod_time
         ; lastmod_date
         ; extra
         ; comment
         ; compression_method
         ; crc
         ; uncompressed_size
         ; compressed_size
         ; is_directory = String.length name > 0 && name.[String.length name - 1] = '/'
         ; file_offset = header_offset
         }
         :: !entries
    done;
    assert (
      cd_bound = LargeFile.pos_in ic
      && (cd_entries = 65535 || cd_entries = !entry_count land 0xffff));
    List.rev !entries
  with
  | End_of_file -> raise (Failure "end-of-file while reading central directory")
;;

let open_in filename =
  let ic = Stdlib.open_in_bin filename in
  try
    let cd_entries, cd_size, cd_offset, cd_comment = read_ecd ic in
    let entries = read_cd ic cd_entries cd_offset (Int32.add cd_offset cd_size) in
    let dir = Hashtbl.create (cd_entries / 3) in
    List.iter (fun e -> Hashtbl.add dir e.name e) entries;
    { if_filename = filename
    ; if_channel = ic
    ; if_entries = entries
    ; if_directory = dir
    ; if_comment = cd_comment
    }
  with
  | exn ->
    Stdlib.close_in ic;
    raise exn
;;

let close_in file = Stdlib.close_in file.if_channel
let entries file = file.if_entries

let goto_entry ifile e =
  try
    let ic = ifile.if_channel in
    LargeFile.seek_in ic e.file_offset;
    let magic = read_4_bytes ic in
    let _version_needed = read_2_bytes ic in
    let _flags = read_2_bytes ic in
    let _methd = read_2_bytes ic in
    let _lastmod_time = read_2_bytes ic in
    let _lastmod_date = read_2_bytes ic in
    let _crc = read_4_bytes ic in
    let _compr_size = read_4_bytes_int ic in
    let _uncompr_size = read_4_bytes_int ic in
    let filename_len = read_2_bytes ic in
    let extra_len = read_2_bytes ic in
    if magic <> Int32.of_int 0x04034b50 then failwith "wrong local file header";
    (* Could validate information read against directory entry, but
       what the heck *)
    LargeFile.seek_in
      ic
      (Int64.add e.file_offset (Int64.of_int (30 + filename_len + extra_len)));
    ic
  with
  | End_of_file -> failwith "truncated local file header"
;;

let bigstring_input ic buf off len =
  let tmp = Bytes.create len in
  try
    let len = input ic tmp 0 len in
    for i = 0 to len - 1 do
      buf.{off + i} <- Bytes.get tmp i
    done;
    len
  with
  | End_of_file -> 0
;;

let copy_entry_to_channel ifile entry oc =
  try
    let ic = goto_entry ifile entry in
    match entry.compression_method with
    | Stored ->
      if entry.compressed_size <> entry.uncompressed_size
      then raise (Failure "wrong size for stored entry");
      let buf = Bytes.create 4096 in
      let rec copy n =
        if n > 0
        then (
          let r = input ifile.if_channel buf 0 (min n (Bytes.length buf)) in
          output oc buf 0 r;
          copy (n - r))
      in
      copy entry.uncompressed_size
    | Deflated ->
      let still_available = ref entry.compressed_size in
      let input = De.bigstring_create entry.uncompressed_size in
      let output = De.bigstring_create De.io_buffer_size in
      let window = De.make_window ~bits:15 in
      let decoder = De.Inf.decoder `Manual ~o:output ~w:window in
      let rec go () =
        match De.Inf.decode decoder with
        | `Malformed err -> failwith err
        | `Await ->
          let to_read = min !still_available De.io_buffer_size in
          let len = bigstring_input ic input 0 to_read in
          still_available := !still_available - len;
          De.Inf.src decoder input 0 len;
          go ()
        | `Flush ->
          let len = De.io_buffer_size - De.Inf.dst_rem decoder in
          let str = Bigstringaf.substring output ~off:0 ~len in
          output_string oc str;
          De.Inf.flush decoder;
          go ()
        | `End ->
          let len = De.io_buffer_size - De.Inf.dst_rem decoder in
          if len > 0 then output_string oc (Bigstringaf.substring output ~off:0 ~len)
      in
      go ()
  with
  | End_of_file -> raise (Failure "truncated data")
;;
