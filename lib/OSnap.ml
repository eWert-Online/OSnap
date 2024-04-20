module Config = OSnap_Config
module Browser = OSnap_Browser
module Test = OSnap_Test
open OSnap_Utils

type t =
  { config : Config.Types.global
  ; tests_to_run : (Config.Types.test * Config.Types.size * bool) list
  ; start_time : float
  ; browser : Browser.t
  }

let setup ~noCreate ~noOnly ~noSkip ~parallelism ~config_path =
  let open Config.Types in
  let start_time = Unix.gettimeofday () in
  let*? config = Config.Global.init ~config_path |> Lwt_result.lift in
  let config =
    match parallelism with
    | Some parallelism -> { config with parallelism }
    | None -> config
  in
  let () = OSnap_Paths.init_folder_structure config in
  let snapshot_dir = OSnap_Paths.get_base_images_dir config in
  let*? all_tests = Config.Test.init config |> Lwt_result.lift in
  let*? only_tests, tests =
    all_tests
    |> Lwt_list.map_p_until_exception (fun test ->
      test.sizes
      |> Lwt_list.map_p_until_exception (fun size ->
        let { name = _size_name; width; height } = size in
        let filename = Test.get_filename test.name width height in
        let current_image_path = Filename.concat snapshot_dir filename in
        let exists = Sys.file_exists current_image_path in
        if noCreate && not exists
        then
          Lwt_result.fail
            (`OSnap_Invalid_Run
              (Printf.sprintf
                 "Flag --no-create is set. Cannot create new images for %s."
                 test.name))
        else if noSkip && test.skip
        then
          Lwt_result.fail
            (`OSnap_Invalid_Run
              (Printf.sprintf "Flag --no-skip is set. Cannot skip test %s." test.name))
        else if noOnly && test.only
        then
          Lwt_result.fail
            (`OSnap_Invalid_Run
              (Printf.sprintf
                 "Flag --no-only is set but the following test still has only set to \
                  true %s."
                 test.name))
        else if test.only
        then Lwt_result.return (Either.left (test, size, exists))
        else Lwt_result.return (Either.right (test, size, exists))))
    |> Lwt_result.map List.flatten
    |> Lwt_result.map (List.partition_map Fun.id)
  in
  let tests_to_run =
    match only_tests, tests with
    | [], tests -> tests
    | only_tests, _ -> only_tests
  in
  let tests_to_run =
    tests_to_run
    |> List.fast_sort (fun (_test, _size, exists1) (_test, _size, exists2) ->
      Bool.compare exists1 exists2)
  in
  let*? browser = Browser.Launcher.make () in
  Lwt_result.return { config; tests_to_run; start_time; browser }
;;

let teardown t = Browser.Launcher.shutdown t.browser

let run t =
  let open Config.Types in
  let { tests_to_run; config; start_time; browser } = t in
  let parallelism = max 1 config.parallelism in
  let pool =
    Lwt_pool.create
      parallelism
      (fun () -> Browser.Target.make browser)
      ~validate:(fun target -> Lwt.return (Result.is_ok target))
  in
  let*? test_results =
    tests_to_run
    |> Lwt_list.map_p_until_exception (fun test ->
      Lwt_pool.use pool (fun target ->
        let test, { name = size_name; width; height }, exists = test in
        let test =
          Test.Types.
            { exists
            ; size_name
            ; width
            ; height
            ; skip = test.OSnap_Config.Types.skip
            ; url = test.OSnap_Config.Types.url
            ; name = test.OSnap_Config.Types.name
            ; actions = test.OSnap_Config.Types.actions
            ; ignore_regions = test.OSnap_Config.Types.ignore
            ; threshold = test.OSnap_Config.Types.threshold
            ; warnings = []
            ; result = None
            }
        in
        Test.run config (Result.get_ok target) test))
  in
  let end_time = Unix.gettimeofday () in
  let seconds = end_time -. start_time in
  Browser.Launcher.shutdown browser;
  Test.Printer.stats ~seconds test_results
;;

let cleanup = OSnap_Cleanup.cleanup
