module Config = OSnap_Config;
module Browser = OSnap_Browser;

module Printer = OSnap_Printer;
module Logger = OSnap_Logger;

module Utils = OSnap_Utils;

module Lwt_list = {
  include Lwt_list;

  let map_p_until_exception = (fn, list) => {
    open! Lwt.Syntax;
    let rec loop = (acc, list) => {
      switch (list) {
      | [] => Lwt_result.return(acc)
      | list =>
        let* (resolved, pending) = Lwt.nchoose_split(list);
        let (success, error) =
          resolved
          |> List.partition_map(
               fun
               | Ok(v) => Either.left(v)
               | Error(e) => Either.right(e),
             );

        switch (error) {
        | [] => loop(success @ acc, pending)
        | [hd, ..._tl] =>
          pending |> List.iter(Lwt.cancel);
          Lwt_result.fail(hd);
        };
      };
    };

    let promises = list |> List.map(Lwt.apply(fn));
    loop([], promises);
  };
};

type t = {
  config: Config.Types.global,
  all_tests: list((Config.Types.test, Config.Types.size, bool)),
  tests_to_run: list((Config.Types.test, Config.Types.size, bool)),
  start_time: float,
  browser: Browser.t,
};

let init_folder_structure = config => {
  let debug = Logger.debug(~header="SETUP");

  let dirs = OSnap_Paths.get(config);

  if (!Sys.file_exists(dirs.base)) {
    debug("creating base images folder at " ++ dirs.base);
    FileUtil.mkdir(~parent=true, ~mode=`Octal(0o755), dirs.base);
  };

  debug("(re)creating " ++ dirs.updated);
  FileUtil.rm(~recurse=true, [dirs.updated]);
  FileUtil.mkdir(~parent=true, ~mode=`Octal(0o755), dirs.updated);

  debug("(re)creating " ++ dirs.diff);
  FileUtil.rm(~recurse=true, [dirs.diff]);
  FileUtil.mkdir(~parent=true, ~mode=`Octal(0o755), dirs.diff);
};

let setup = (~noCreate, ~noOnly, ~noSkip, ~parallelism, ~config_path) => {
  open Config.Types;
  open Lwt_result.Syntax;

  let debug = Logger.debug(~header="SETUP");

  let start_time = Unix.gettimeofday();

  let* config = Config.Global.init(~config_path) |> Lwt_result.lift;
  let config =
    switch (parallelism) {
    | Some(parallelism) => {...config, parallelism}
    | None => config
    };

  let () = init_folder_structure(config);
  let snapshot_dir = OSnap_Paths.get_base_images_dir(config);
  debug("looking for test files");
  let* tests = Config.Test.init(config) |> Lwt_result.lift;
  debug(Printf.sprintf("found %i test files", List.length(tests)));

  debug("collecting test sizes to run");
  let* all_tests =
    tests
    |> Lwt_list.map_p_until_exception(test => {
         test.sizes
         |> Lwt_list.map_p_until_exception(size => {
              let {name: _size_name, width, height} = size;
              let filename =
                OSnap_Test.get_filename(test.name, width, height);
              let current_image_path = snapshot_dir ++ filename;
              let exists = Sys.file_exists(current_image_path);

              if (noCreate && !exists) {
                Lwt_result.fail(
                  OSnap_Response.Invalid_Run(
                    Printf.sprintf(
                      "Flag --no-create is set. Cannot create new images for %s.",
                      test.name,
                    ),
                  ),
                );
              } else {
                Lwt_result.return((test, size, exists));
              };
            })
       })
    |> Lwt_result.map(List.flatten);

  debug("checking for \"only\" flags");
  let only_tests = all_tests |> List.find_all(((test, _, _)) => test.only);

  let* tests_to_run =
    if (noOnly && List.length(only_tests) > 0) {
      Lwt_result.fail(
        OSnap_Response.Invalid_Run(
          only_tests
          |> List.map(((test: Config.Types.test, _, _)) => test.name)
          |> List.sort_uniq(String.compare)
          |> String.concat(",\n")
          |> Printf.sprintf(
               "Flag --no-only is set, but the following tests still have only set to true:\n%s",
             ),
        ),
      );
    } else if (List.length(only_tests) > 0) {
      Lwt_result.return(only_tests);
    } else {
      Lwt_result.return(all_tests);
    };

  debug("checking for \"skip\" flags");
  let (skipped_tests, tests_to_run) =
    tests_to_run |> List.partition(((test, _, _)) => test.skip);

  let* tests_to_run =
    if (noSkip && List.length(skipped_tests) > 0) {
      Lwt_result.fail(
        OSnap_Response.Invalid_Run(
          skipped_tests
          |> List.map(((test: Config.Types.test, _, _)) => test.name)
          |> List.sort_uniq(String.compare)
          |> String.concat(",\n")
          |> Printf.sprintf(
               "Flag --no-skip is set, but the following tests still have \"skip\" set to true:\n%s",
             ),
        ),
      );
    } else if (List.length(skipped_tests) > 0) {
      skipped_tests
      |> List.iter(((test: Config.Types.test, {width, height, _}, _)) =>
           Printer.skipped_message(~name=test.name, ~width, ~height)
         );
      Lwt_result.return(tests_to_run);
    } else {
      Lwt_result.return(tests_to_run);
    };

  debug("setting test priority");
  let tests_to_run =
    tests_to_run
    |> List.fast_sort(((_test, _size, exists1), (_test, _size, exists2)) => {
         Bool.compare(exists1, exists2)
       });

  debug("launching browser");
  let* browser = Browser.Launcher.make();

  Lwt_result.return({config, all_tests, tests_to_run, start_time, browser});
};

let teardown = t => {
  Browser.Launcher.shutdown(t.browser);
};

let run = t => {
  open Config.Types;
  open Lwt_result.Syntax;

  let debug = Logger.debug(~header="RUN");
  let {tests_to_run, all_tests, config, start_time, browser} = t;

  debug(Printf.sprintf("creating pool of %i runners", config.parallelism));
  let pool =
    Lwt_pool.create(
      config.parallelism,
      () => Browser.Target.make(browser),
      ~validate=target => Lwt.return(Result.is_ok(target)),
    );

  let* test_results =
    tests_to_run
    |> Lwt_list.map_p_until_exception(test => {
         Lwt_pool.use(
           pool,
           target => {
             let (test, {name: size_name, width, height}, exists) = test;

             let test: OSnap_Test.t = {
               exists,
               size_name,
               width,
               height,
               url: test.url,
               name: test.name,
               actions: test.actions,
               ignore_regions: test.ignore,
               threshold: test.threshold,
             };

             /* Targets are validated at creation time. They are guaranteed to be created. */
             OSnap_Test.run(config, Result.get_ok(target), test);
           },
         )
       });

  let end_time = Unix.gettimeofday();
  let seconds = end_time -. start_time;

  Browser.Launcher.shutdown(browser);

  let create_count =
    test_results |> List.filter(r => r == `Created) |> List.length;
  let passed_count =
    test_results |> List.filter(r => r == `Passed) |> List.length;
  let failed_count =
    test_results |> List.filter(r => r == `Failed) |> List.length;
  let test_count = tests_to_run |> List.length;

  Printer.stats(
    ~test_count,
    ~create_count,
    ~passed_count,
    ~failed_count,
    ~skipped_count=List.length(all_tests) - test_count,
    ~seconds,
  );

  if (failed_count == 0) {
    Lwt_result.return();
  } else {
    Lwt_result.fail(OSnap_Response.Test_Failure);
  };
};

let cleanup = (~config_path) => {
  let ( let* ) = Result.bind;

  print_newline();

  let* config = Config.Global.init(~config_path);
  let () = init_folder_structure(config);
  let snapshot_dir = OSnap_Paths.get_base_images_dir(config);
  let* tests = Config.Test.init(config);

  let test_file_paths =
    tests
    |> List.map((test: Config.Types.test) => {
         test.sizes
         |> List.filter_map((size: Config.Types.size) => {
              let Config.Types.{width, height, _} = size;
              let filename =
                OSnap_Test.get_filename(test.name, width, height);
              let current_image_path = snapshot_dir ++ filename;
              let exists = Sys.file_exists(current_image_path);

              if (exists) {
                Some(current_image_path);
              } else {
                None;
              };
            })
       })
    |> List.flatten;

  let files_to_delete =
    FileUtil.ls(snapshot_dir)
    |> List.find_all(file => !List.mem(file, test_file_paths));
  let num_files_to_delete = List.length(files_to_delete);
  open Fmt;
  if (num_files_to_delete > 0) {
    Fmt.pr(
      "%a @.",
      styled(`Bold, string),
      Printf.sprintf("Deleting %i files...\n", num_files_to_delete),
    );
    files_to_delete
    |> List.iter(file => {
         FileUtil.rm([file]);
         Fmt.pr(
           "%a @.",
           styled(`Faint, string),
           Printf.sprintf("Deleted %s", file),
         );
       });

    Fmt.pr("\n%a @.", styled(`Bold, styled(`Green, string)), "Done!");
  } else {
    Fmt.pr(
      "%a @.",
      styled(`Bold, styled(`Green, string)),
      "Everything clean. No files to remove!",
    );
  };

  print_newline();

  Result.ok();
};

let download_chromium = OSnap_Browser.Download.download;
