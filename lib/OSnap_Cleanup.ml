module Config = OSnap_Config
open OSnap_Utils

let cleanup ~config_path =
  print_newline ();
  let*? config = Config.Global.init ~config_path |> Lwt.return in
  let () = OSnap_Paths.init_folder_structure config in
  let snapshot_dir = OSnap_Paths.get_base_images_dir config in
  let*? tests = Config.Test.init config |> Lwt.return in
  let test_file_paths =
    tests
    |> List.map (fun (test : Config.Types.test) ->
      test.sizes
      |> List.filter_map (fun (size : Config.Types.size) ->
        let Config.Types.{ width; height; _ } = size in
        let filename = OSnap_Test.get_filename test.name width height in
        let current_image_path = Filename.concat snapshot_dir filename in
        let exists = Sys.file_exists current_image_path in
        if exists then Some filename else None))
    |> List.flatten
  in
  let files_to_delete =
    Sys.readdir snapshot_dir
    |> Array.to_list
    |> List.filter_map (fun file ->
      if not (List.mem file test_file_paths)
      then Some (Filename.concat snapshot_dir file)
      else None)
  in
  let num_files_to_delete = List.length files_to_delete in
  let open Fmt in
  if num_files_to_delete > 0
  then (
    Fmt.pr
      "%a @."
      (styled `Bold string)
      (Printf.sprintf "Deleting %i files...\n" num_files_to_delete);
    files_to_delete
    |> List.iter (fun file ->
      Sys.remove file;
      Fmt.pr "%a @." (styled `Faint string) (Printf.sprintf "Deleted %s" file));
    Fmt.pr "\n%a @." (styled `Bold (styled `Green string)) "Done!")
  else
    Fmt.pr
      "%a @."
      (styled `Bold (styled `Green string))
      "Everything clean. No files to remove!";
  print_newline ();
  Lwt_result.return ()
;;
