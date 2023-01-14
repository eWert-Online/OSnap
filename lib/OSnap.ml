module Config = OSnap_Config
module Browser = OSnap_Browser
module Printer = OSnap_Printer
open OSnap_Utils

let print_error msg =
  let printer = Fmt.pr "%a @." (Fmt.styled `Red Fmt.string) in
  Printf.ksprintf printer msg
;;

type t =
  { config : Config.Types.global
  ; all_tests : (Config.Types.test * Config.Types.size * bool) list
  ; tests_to_run : (Config.Types.test * Config.Types.size * bool) list
  ; start_time : float
  ; browser : Browser.t
  }

let setup ~noCreate ~noOnly ~noSkip ~parallelism ~config_path =
  let open Config.Types in
  let open Lwt_result.Syntax in
  let start_time = Unix.gettimeofday () in
  let* config = Config.Global.init ~config_path |> Lwt_result.lift in
  let config =
    match parallelism with
    | Some parallelism -> { config with parallelism }
    | None -> config
  in
  let () = OSnap_Paths.init_folder_structure config in
  let snapshot_dir = OSnap_Paths.get_base_images_dir config in
  let* tests = Config.Test.init config |> Lwt_result.lift in
  let* all_tests =
    tests
    |> Lwt_list.map_p_until_exception (fun test ->
         test.sizes
         |> Lwt_list.map_p_until_exception (fun size ->
              let { name = _size_name; width; height } = size in
              let filename = OSnap_Test.get_filename test.name width height in
              let current_image_path = snapshot_dir ^ filename in
              let exists = Sys.file_exists current_image_path in
              if noCreate && not exists
              then
                Lwt_result.fail
                  (`OSnap_Invalid_Run
                    (Printf.sprintf
                       "Flag --no-create is set. Cannot create new images for %s."
                       test.name))
              else Lwt_result.return (test, size, exists)))
    |> Lwt_result.map List.flatten
  in
  let only_tests = all_tests |> List.find_all (fun (test, _, _) -> test.only) in
  let* tests_to_run =
    if noOnly && List.length only_tests > 0
    then
      Lwt_result.fail
        (`OSnap_Invalid_Run
          (only_tests
          |> List.map (fun ((test : Config.Types.test), _, _) -> test.name)
          |> List.sort_uniq String.compare
          |> String.concat ",\n"
          |> Printf.sprintf
               "Flag --no-only is set, but the following tests still have only set to \
                true:\n\
                %s"))
    else if List.length only_tests > 0
    then Lwt_result.return only_tests
    else Lwt_result.return all_tests
  in
  let skipped_tests, tests_to_run =
    tests_to_run |> List.partition (fun (test, _, _) -> test.skip)
  in
  let* tests_to_run =
    if noSkip && List.length skipped_tests > 0
    then
      Lwt_result.fail
        (`OSnap_Invalid_Run
          (skipped_tests
          |> List.map (fun ((test : Config.Types.test), _, _) -> test.name)
          |> List.sort_uniq String.compare
          |> String.concat ",\n"
          |> Printf.sprintf
               "Flag --no-skip is set, but the following tests still have \"skip\" set \
                to true:\n\
                %s"))
    else if List.length skipped_tests > 0
    then (
      skipped_tests
      |> List.iter (fun ((test : Config.Types.test), { width; height; _ }, _) ->
           Printer.skipped_message ~name:test.name ~width ~height);
      Lwt_result.return tests_to_run)
    else Lwt_result.return tests_to_run
  in
  let tests_to_run =
    tests_to_run
    |> List.fast_sort (fun (_test, _size, exists1) (_test, _size, exists2) ->
         Bool.compare exists1 exists2)
  in
  let* browser = Browser.Launcher.make () in
  Lwt_result.return { config; all_tests; tests_to_run; start_time; browser }
;;

let teardown t = Browser.Launcher.shutdown t.browser

let run t =
  let open Config.Types in
  let open Lwt_result.Syntax in
  let { tests_to_run; all_tests; config; start_time; browser } = t in
  let parallelism = max 1 config.parallelism in
  let pool =
    Lwt_pool.create
      parallelism
      (fun () -> Browser.Target.make browser)
      ~validate:(fun target -> Lwt.return (Result.is_ok target))
  in
  let* test_results =
    tests_to_run
    |> Lwt_list.map_p_until_exception (fun test ->
         Lwt_pool.use pool (fun target ->
           let test, { name = size_name; width; height }, exists = test in
           let test =
             ({ exists
              ; size_name
              ; width
              ; height
              ; url = test.url
              ; name = test.name
              ; actions = test.actions
              ; ignore_regions = test.ignore
              ; threshold = test.threshold
              }
               : OSnap_Test.t)
           in
           OSnap_Test.run config (Result.get_ok target) test))
  in
  let end_time = Unix.gettimeofday () in
  let seconds = end_time -. start_time in
  Browser.Launcher.shutdown browser;
  let create_count = test_results |> List.filter (fun r -> r = `Created) |> List.length in
  let passed_count = test_results |> List.filter (fun r -> r = `Passed) |> List.length in
  let failed_tests =
    test_results
    |> List.filter_map (function
         | `Passed | `Created -> None
         | `Failed _ as r -> Some r)
  in
  let test_count = tests_to_run |> List.length in
  Printer.stats
    ~test_count
    ~create_count
    ~passed_count
    ~failed_tests
    ~skipped_count:(List.length all_tests - test_count)
    ~seconds;
  match failed_tests with
  | [] -> Lwt_result.return ()
  | _ -> Lwt_result.fail `OSnap_Test_Failure
;;

let cleanup ~config_path =
  let ( let* ) = Result.bind in
  print_newline ();
  let* config = Config.Global.init ~config_path in
  let () = OSnap_Paths.init_folder_structure config in
  let snapshot_dir = OSnap_Paths.get_base_images_dir config in
  let* tests = Config.Test.init config in
  let test_file_paths =
    tests
    |> List.map (fun (test : Config.Types.test) ->
         test.sizes
         |> List.filter_map (fun (size : Config.Types.size) ->
              let Config.Types.{ width; height; _ } = size in
              let filename = OSnap_Test.get_filename test.name width height in
              let current_image_path = snapshot_dir ^ filename in
              let exists = Sys.file_exists current_image_path in
              if exists then Some filename else None))
    |> List.flatten
  in
  let files_to_delete =
    Sys.readdir snapshot_dir
    |> Array.to_list
    |> List.filter_map (fun file ->
         if List.mem file test_file_paths
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
  Result.ok ()
;;

let download_chromium = OSnap_Browser.Download.download
