type revision = string

let revision_to_string revision = revision

(* https://googlechromelabs.github.io/chrome-for-testing/known-good-versions-with-downloads.json *)
let revisions =
  [ "1331488"; "1298002"; "1289146"; "1056772"; "961656"; "960312"; "884014"; "856583" ]
;;

let get_latest_revision () = List.hd revisions
let get_previous_revisions () = List.tl revisions
let get_folder_name revision = Printf.sprintf "osnap_chromium_%s" revision

let get_chromium_path revision =
  match Sys.getenv_opt "HOME" with
  | Some home when Sys.is_directory home ->
    Filename.concat home (get_folder_name revision)
  | _ -> Filename.concat (Filename.get_temp_dir_name ()) (get_folder_name revision)
;;
