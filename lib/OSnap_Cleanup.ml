module Config = OSnap_Config
open OSnap_Utils

let cleanup ~env ~config_path =
  let ( let*? ) = Result.bind in
  print_newline ();
  let*? config = Config.Global.init ~env ~config_path in
  let () = OSnap_Paths.init_folder_structure config in
  let snapshot_dir = OSnap_Paths.get_base_images_dir config in
  let*? tests = Config.Test.init config in
  let test_file_paths =
    tests
    |> List.map (fun (test : Config.Types.test) ->
      test.sizes
      |> List.filter_map (fun (size : Config.Types.size) ->
        let Config.Types.{ width; height; _ } = size in
        let filename = OSnap_Test.get_filename test.name width height in
        let current_image_path = Eio.Path.(snapshot_dir / filename) in
        let exists = Eio.Path.is_file current_image_path in
        if exists then Some filename else None))
    |> List.flatten
  in
  let files_to_delete =
    Eio.Path.read_dir snapshot_dir
    |> List.filter_map (fun file ->
      if not (List.mem file test_file_paths)
      then Some Eio.Path.(snapshot_dir / file)
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
      Eio.Path.rmtree ~missing_ok:true file;
      Fmt.pr "%a @." (styled `Faint string) (Printf.sprintf "Deleted %s" (snd file)));
    Fmt.pr "\n%a @." (styled `Bold (styled `Green string)) "Done!")
  else
    Fmt.pr
      "%a @."
      (styled `Bold (styled `Green string))
      "Everything clean. No files to remove!";
  print_newline ();
  Result.ok ()
;;
