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

let setup ~sw ~env ~noCreate ~noOnly ~noSkip ~parallelism ~config_path =
  let ( let*? ) = Result.bind in
  let open Config.Types in
  let start_time = Unix.gettimeofday () in
  let*? config = Config.Global.init ~env ~config_path in
  let config =
    match parallelism with
    | Some parallelism -> { config with parallelism }
    | None -> config
  in
  let () = OSnap_Paths.init_folder_structure config in
  let snapshot_dir = OSnap_Paths.get_base_images_dir config in
  let*? all_tests = Config.Test.init config in
  let*? only_tests, tests =
    all_tests
    |> ResultList.map_p_until_first_error (fun test ->
      test.sizes
      |> ResultList.map_p_until_first_error (fun size ->
        let { name = _size_name; width; height } = size in
        let filename = Test.get_filename test.name width height in
        let current_image_path = Eio.Path.(snapshot_dir / filename) in
        let exists = Eio.Path.is_file current_image_path in
        if noCreate && not exists
        then
          Result.error
            (`OSnap_Invalid_Run
              (Printf.sprintf
                 "Flag --no-create is set. Cannot create new images for %s."
                 test.name))
        else if noSkip && test.skip
        then
          Result.error
            (`OSnap_Invalid_Run
              (Printf.sprintf "Flag --no-skip is set. Cannot skip test %s." test.name))
        else if noOnly && test.only
        then
          Result.error
            (`OSnap_Invalid_Run
              (Printf.sprintf
                 "Flag --no-only is set but the following test still has only set to \
                  true %s."
                 test.name))
        else if test.only
        then Result.ok (Either.left (test, size, exists))
        else Result.ok (Either.right (test, size, exists))))
    |> Result.map List.flatten
    |> Result.map (List.partition_map Fun.id)
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
  let*? browser = Browser.Launcher.make ~sw ~env () in
  Result.ok { config; tests_to_run; start_time; browser }
;;

let teardown t = Browser.Launcher.shutdown t.browser

let run ~env t =
  let ( let*? ) = Result.bind in
  let open Config.Types in
  let { tests_to_run; config; start_time; browser } = t in
  let parallelism = max 1 config.parallelism in
  let pool =
    Eio.Pool.create
      ~validate:(fun target -> Result.is_ok target)
      parallelism
      (fun () -> Browser.Target.make browser)
  in
  Test.Printer.Progress.set_total (List.length tests_to_run);
  let*? test_results =
    tests_to_run
    |> ResultList.map_p_until_first_error (fun test ->
      Eio.Pool.use pool (fun target ->
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
        Test.run ~env config (Result.get_ok target) test))
  in
  let end_time = Unix.gettimeofday () in
  let seconds = end_time -. start_time in
  Browser.Launcher.shutdown browser;
  Test.Printer.stats ~seconds test_results
;;

let cleanup = OSnap_Cleanup.cleanup
