module Config = OSnap_Config
module Browser = OSnap_Browser
module Test = OSnap_Test

let ( let*? ) = Result.bind

type t =
  { config : Config.Types.global
  ; tests_to_run : (Config.Types.test * Config.Types.size * bool) list
  ; start_time : float
  ; browser : Browser.t
  }

let setup ~noCreate ~noOnly ~noSkip ~parallelism ~config_path =
  let open Config.Types in
  let start_time = Unix.gettimeofday () in
  let*? config = Config.Global.init ~config_path in
  let config =
    match parallelism with
    | Some parallelism -> { config with parallelism }
    | None -> config
  in
  let () = OSnap_Paths.init_folder_structure config in
  let snapshot_dir = OSnap_Paths.get_base_images_dir config in
  let*? all_tests = Config.Test.init config in
  let*? all_tests =
    all_tests
    |> OSnap_Utils.List.map_until_exception (fun test ->
         test.sizes
         |> OSnap_Utils.List.map_until_exception (fun size ->
              let { name = _size_name; width; height } = size in
              let filename = Test.get_filename test.name width height in
              let current_image_path = snapshot_dir ^ filename in
              let exists = Sys.file_exists current_image_path in
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
                    (Printf.sprintf
                       "Flag --no-skip is set. Cannot skip test %s."
                       test.name))
              else if noOnly && test.only
              then
                Result.error
                  (`OSnap_Invalid_Run
                    (Printf.sprintf
                       "Flag --no-only is set but the following test still has only set \
                        to true %s."
                       test.name))
              else if test.only
              then Result.ok (Either.left (test, size, exists))
              else Result.ok (Either.right (test, size, exists))))
  in
  let only_tests, tests = all_tests |> List.flatten |> List.partition_map Fun.id in
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
  Result.ok { config; tests_to_run; start_time; browser }
;;

let teardown t = Browser.Launcher.shutdown t.browser

let run
  :  fs:#Eio.Fs.dir Eio.Path.t -> clock:#Eio.Time.clock -> t
  -> ( unit
     , [> `OSnap_Test_Failure
       | `OSnap_CDP_Protocol_Error of string
       | `OSnap_FS_Error of string
       ] )
     result
  =
 fun ~fs ~clock t ->
  let open Config.Types in
  let { tests_to_run; config; start_time; browser } = t in
  let parallelism = max 1 config.parallelism in
  let pool = Eio.Stream.create parallelism in
  while Eio.Stream.length pool != parallelism do
    match Browser.Target.make browser with
    | Ok browser -> Eio.Stream.add pool browser
    | Error _ -> ()
  done;
  let*? test_results =
    tests_to_run
    |> OSnap_Utils.map_p_until_error (fun test ->
         let target = Eio.Stream.take pool in
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
         Test.run ~fs ~clock config target test)
  in
  let end_time = Unix.gettimeofday () in
  let seconds = end_time -. start_time in
  Browser.Launcher.shutdown browser;
  Test.Printer.stats ~seconds test_results
;;

let cleanup = OSnap_Cleanup.cleanup
let download_chromium = OSnap_Browser.Download.download
