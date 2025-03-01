type revision = string * string

let revision_to_string (revision, _version) = revision
let revision_to_version (_revision, version) = version

(* https://googlechromelabs.github.io/chrome-for-testing/ *)
let revisions =
  [ "1402768", "133.0.6943.141"
  ; "1331488", "128.0.6613.7"
  ; "1298002", "126.0.6468.2"
  ; "1289146", "126.0.6439.0"
  ; "1056772", "108.0.5355.0"
  ; "961656", "99.0.4844.11"
  ; "960312", "99.0.4840.0"
  ; "884014", "92.0.4512.4"
  ; "856583", "90.0.4427.5"
  ]
;;

let get_latest_revision () = List.hd revisions
let get_previous_revisions () = List.tl revisions
let get_folder_name (revision, _version) = Printf.sprintf "osnap_chromium_%s" revision

let get_chromium_path revision =
  match Sys.getenv_opt "HOME" with
  | Some home when Sys.is_directory home ->
    Filename.concat home (get_folder_name revision)
  | _ -> Filename.concat (Filename.get_temp_dir_name ()) (get_folder_name revision)
;;
