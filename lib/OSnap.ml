module Config = OSnap_Config
module Browser = OSnap_Browser
module Test = OSnap_Test
open OSnap_Utils
module Logger = OSnap_Logger

type t =
  { config : Config.Types.global
  ; tests_to_run : (Config.Types.test * Config.Types.size * bool) list
  ; start_time : float
  ; browser : Browser.t
  }

let setup ~sw ~env ~noCreate ~noOnly ~noSkip ~config_path =
  let ( let*? ) = Result.bind in
  let open Config.Types in
  let debug = Logger.info ~header:"SETUP" in
  let start_time = Eio.Time.now (Eio.Stdenv.clock env) in
  let*? config = Config.Global.init ~env ~config_path in
  debug "initializing folder structure";
  let () = OSnap_Paths.init_folder_structure config in
  debug "getting snapshow dir";
  let snapshot_dir = OSnap_Paths.get_base_images_dir config in
  let*? all_tests = Config.Test.init config in
  debug (Printf.sprintf "found %i tests" (List.length all_tests));
  let*? only_tests, tests =
    all_tests
    |> ResultList.traverse (fun test ->
      test.sizes
      |> ResultList.traverse (fun size ->
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
  debug (Printf.sprintf "found %i tests to run" (List.length tests_to_run));
  debug "setting test priority";
  let tests_to_run =
    tests_to_run
    |> List.fast_sort (fun (_test, _size, exists1) (_test, _size, exists2) ->
      Bool.compare exists1 exists2)
  in
  let*? browser = Browser.Launcher.make ~sw ~env () in
  Result.ok { config; tests_to_run; start_time; browser }
;;

let run ~env t =
  Eio.Switch.run
  @@ fun sw ->
  let open Config.Types in
  let debug = Logger.info ~header:"RUN" in
  let { tests_to_run; config; start_time; browser } = t in
  Test.Printer.Progress.set_total (List.length tests_to_run);
  let domain_count = Domain.recommended_domain_count () in
  let parallelism = domain_count * 3 in
  debug (Printf.sprintf "creating pool of %i runners" parallelism);
  let test_stream = Eio.Stream.create 0 in
  let browser_pool =
    Eio.Pool.create
      parallelism
      (fun () -> Browser.Target.make browser)
      ~validate:(fun target -> Result.is_ok target)
  in
  debug "Assigning tests to runners...";
  for _ = 1 to parallelism do
    Eio.Fiber.fork_daemon ~sw (fun () ->
      let rec aux () =
        let request, reply = Eio.Stream.take test_stream in
        let test_result =
          Eio.Pool.use browser_pool (fun target ->
            Test.run ~env config (Result.get_ok target) request)
        in
        Eio.Promise.resolve reply test_result;
        aux ()
      in
      aux ())
  done;
  debug "Waiting for test results to come back";
  let rec run_test test =
    let reply, resolve_reply = Eio.Promise.create () in
    Eio.Stream.add test_stream (test, resolve_reply);
    let response = Eio.Promise.await reply in
    match response with
    | Ok ({ result = Some (`Retry _); _ } as test) -> run_test test
    | r -> r
  in
  let*? test_results =
    tests_to_run
    |> ResultList.traverse
       @@ fun target ->
       let test, { name = size_name; width; height }, exists = target in
       run_test
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
           ; additional_headers = test.OSnap_Config.Types.additional_headers
           ; expected_response_code = test.OSnap_Config.Types.expected_response_code
           ; threshold = test.OSnap_Config.Types.threshold
           ; retry = test.OSnap_Config.Types.retry
           ; warnings = []
           ; result = None
           }
  in
  debug "Tests were run to completion. Calculating final stats.";
  let end_time = Eio.Time.now (Eio.Stdenv.clock env) in
  let seconds = end_time -. start_time in
  Test.Printer.stats ~seconds test_results
;;

let cleanup = OSnap_Cleanup.cleanup
