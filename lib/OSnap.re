module Config = OSnap_Config;
module Browser = OSnap_Browser;

module Diff = OSnap_Diff;
module Printer = OSnap_Printer;
module Logger = OSnap_Logger;

module Utils = OSnap_Utils;

module List = {
  include List;

  let find_all_or_input = (fn, list) => {
    switch (List.find_all(fn, list)) {
    | [] => list
    | list => list
    };
  };
};

type t = {
  config: Config.Types.global,
  all_tests: list((Config.Types.test, Config.Types.size, bool)),
  tests_to_run: list((Config.Types.test, Config.Types.size, bool)),
  snapshot_dir: string,
  updated_dir: string,
  diff_dir: string,
  start_time: float,
};

let get_filename = (name, width, height) =>
  Printf.sprintf("/%s_%ix%i.png", name, width, height);

let init_folder_structure = config => {
  open Config.Types;
  let debug = Logger.debug(~header="SETUP");

  let base_path = config.root_path ++ config.snapshot_directory;

  debug("initializing folder structure in " ++ base_path);

  let snapshot_dir = base_path ++ "/__base_images__";
  if (!Sys.file_exists(snapshot_dir)) {
    debug("creating base images folder at " ++ snapshot_dir);
    FileUtil.mkdir(~parent=true, ~mode=`Octal(0o755), snapshot_dir);
  };

  let updated_dir = base_path ++ "/__updated__";
  debug("(re)creating " ++ updated_dir);
  FileUtil.rm(~recurse=true, [updated_dir]);
  FileUtil.mkdir(~parent=true, ~mode=`Octal(0o755), updated_dir);

  let diff_dir = base_path ++ "/__diff__";
  debug("(re)creating " ++ diff_dir);
  FileUtil.rm(~recurse=true, [diff_dir]);
  FileUtil.mkdir(~parent=true, ~mode=`Octal(0o755), diff_dir);

  (snapshot_dir, updated_dir, diff_dir);
};

let setup = (~noCreate, ~noOnly, ~noSkip, ~config_path) => {
  open Config.Types;

  let debug = Logger.debug(~header="SETUP");

  let start_time = Unix.gettimeofday();

  debug("looking for global config file");
  let config = Config.Global.find(~config_path);
  debug("found global config file at " ++ config);
  debug("parsing config file");
  let config = config |> Config.Global.parse;
  let (snapshot_dir, updated_dir, diff_dir) = init_folder_structure(config);
  debug("looking for test files");
  let tests = Config.Test.init(config);
  debug(Printf.sprintf("found %i test files", List.length(tests)));

  debug("collecting test sizes to run");
  let all_tests =
    tests
    |> List.map(test => {
         test.sizes
         |> List.map(size => {
              let {name: _size_name, width, height} = size;
              let filename = get_filename(test.name, width, height);
              let current_image_path = snapshot_dir ++ filename;
              let exists = Sys.file_exists(current_image_path);

              if (noCreate && !exists) {
                raise(
                  Failure(
                    Printf.sprintf(
                      "Flag --no-create is set. Cannot create new images for %s.",
                      test.name,
                    ),
                  ),
                );
              };
              (test, size, exists);
            })
       })
    |> List.flatten;

  debug("checking for \"only\" flags");
  let tests_to_run =
    all_tests
    |> List.find_all_or_input(((test, _size, _exists)) =>
         if (test.only) {
           if (noOnly) {
             raise(
               Failure(
                 Printf.sprintf(
                   "Flag --no-only is set, but test %s still has only set to true.",
                   test.name,
                 ),
               ),
             );
           };
           true;
         } else {
           false;
         }
       );

  debug("checking for \"skip\" flags");
  let tests_to_run =
    tests_to_run
    |> List.find_all(((test, {width, height, _}, _exists)) =>
         if (test.skip) {
           if (noSkip) {
             raise(
               Failure(
                 Printf.sprintf(
                   "Flag --no-skip is set, but test %s still has skip set to true.",
                   test.name,
                 ),
               ),
             );
           };
           Printer.skipped_message(~name=test.name, ~width, ~height);
           false;
         } else {
           true;
         }
       );

  debug("setting test priority");
  let tests_to_run =
    tests_to_run
    |> List.fast_sort(((_test, _size, exists1), (_test, _size, exists2)) => {
         Bool.compare(exists1, exists2)
       });

  Lwt_result.return({
    config,
    all_tests,
    tests_to_run,
    snapshot_dir,
    updated_dir,
    diff_dir,
    start_time,
  });
};

let save_screenshot = (~path, data) => {
  let%lwt io = Lwt_io.open_file(~mode=Output, path);
  let%lwt () = Lwt_io.write(io, data);
  Lwt_io.close(io);
};

let read_file_contents = (~path) => {
  let%lwt io = Lwt_io.open_file(~mode=Input, path);
  let%lwt data = Lwt_io.read(io);
  let%lwt () = Lwt_io.close(io);
  Lwt.return(data);
};

let run = t => {
  open Config.Types;

  let debug = Logger.debug(~header="RUN");
  let {
    snapshot_dir,
    updated_dir,
    diff_dir,
    all_tests,
    tests_to_run,
    config,
    start_time,
  } = t;

  let create_count = ref(0);
  let passed_count = ref(0);
  let failed_count = ref(0);
  let test_count = ref(0);

  debug("launching browser");
  let%lwt browser = Browser.Launcher.make();

  debug("creating pool of runners");
  let pool =
    Lwt_pool.create(config.parallelism, () => Browser.Target.make(browser));

  let%lwt () =
    tests_to_run
    |> Lwt_stream.of_list
    |> Lwt_stream.iter_n(
         ~max_concurrency=config.parallelism,
         (
           (
             test: Config.Types.test,
             {name: size_name, width, height},
             exists,
           ),
         ) => {
         Lwt_pool.use(
           pool,
           target => {
             incr(test_count);
             let filename = get_filename(test.name, width, height);
             let current_image_path = snapshot_dir ++ filename;
             let new_image_path = updated_dir ++ filename;
             let diff_image_path = diff_dir ++ filename;

             let create_new = !exists;

             let url = config.base_url ++ test.url;
             let full_size = config.fullscreen;

             let%lwt () =
               target
               |> Browser.Actions.set_size(
                    ~width=Float.of_int(width),
                    ~height=Float.of_int(height),
                  );

             let%lwt loaderId =
               target
               |> Browser.Actions.go_to(~url)
               |> Lwt.map(
                    fun
                    | Ok(id) => id
                    | Error(message) => {
                        Browser.Launcher.shutdown(browser);
                        raise(
                          Failure(
                            Printf.sprintf(
                              "Could not connect to url %S. \nError was: %S",
                              url,
                              message,
                            ),
                          ),
                        );
                      },
                  );

             let%lwt () =
               target |> Browser.Actions.wait_for_network_idle(~loaderId);

             let%lwt () =
               test.actions
               |> Lwt_list.iter_s(action => {
                    switch (action, size_name) {
                    | (Click(_, Some(_)), None) => Lwt.return()
                    | (Click(selector, None), _) =>
                      target |> Browser.Actions.click(~selector)
                    | (Click(selector, Some(size_restr)), Some(size_name)) =>
                      if (size_restr |> List.mem(size_name)) {
                        target |> Browser.Actions.click(~selector);
                      } else {
                        Lwt.return();
                      }

                    | (Type(_, _, Some(_)), None) => Lwt.return()
                    | (Type(selector, text, None), _) =>
                      target |> Browser.Actions.type_text(~selector, ~text)
                    | (Type(selector, text, Some(size)), Some(size_name)) =>
                      if (size |> List.mem(size_name)) {
                        target |> Browser.Actions.type_text(~selector, ~text);
                      } else {
                        Lwt.return();
                      }

                    | (Wait(_, Some(_)), None) => Lwt.return()
                    | (Wait(ms, Some(size)), Some(size_name)) =>
                      if (size |> List.mem(size_name)) {
                        let timeout = float_of_int(ms) /. 1000.0;
                        Lwt_unix.sleep(timeout);
                      } else {
                        Lwt.return();
                      }
                    | (Wait(ms, None), _) =>
                      let timeout = float_of_int(ms) /. 1000.0;
                      Lwt_unix.sleep(timeout);
                    }
                  });

             let%lwt screenshot =
               target
               |> Browser.Actions.screenshot(~full_size)
               |> Lwt.map(Base64.decode_exn);

             if (create_new) {
               incr(create_count);
               incr(passed_count);
               Printer.created_message(~name=test.name, ~width, ~height);
               save_screenshot(
                 ~path=create_new ? current_image_path : new_image_path,
                 screenshot,
               );
             } else {
               let%lwt original_image_data =
                 read_file_contents(~path=current_image_path);

               if (original_image_data == screenshot) {
                 incr(passed_count);
                 Printer.success_message(~name=test.name, ~width, ~height);
                 Lwt.return();
               } else {
                 let%lwt ignoreRegions =
                   test.ignore
                   |> Lwt_list.filter_map_p(region => {
                        switch (region, size_name) {
                        | (Coordinates(a, b, None), _) =>
                          Lwt.return(Some((a, b)))
                        | (Coordinates(_, _, Some(_)), None) =>
                          Lwt.return(None)
                        | (
                            Coordinates(a, b, Some(size_restr)),
                            Some(size_name),
                          ) =>
                          if (size_restr |> List.mem(size_name)) {
                            Lwt.return(Some((a, b)));
                          } else {
                            Lwt.return(None);
                          }

                        | (Selector(_, Some(_)), None) => Lwt.return(None)
                        | (
                            Selector(selector, Some(size_restr)),
                            Some(size_name),
                          ) =>
                          if (size_restr |> List.mem(size_name)) {
                            let%lwt ((x1, y1), (x2, y2)) =
                              target |> Browser.Actions.get_quads(~selector);
                            let x1 = Int.of_float(x1);
                            let y1 = Int.of_float(y1);
                            let x2 = Int.of_float(x2);
                            let y2 = Int.of_float(y2);
                            Lwt.return(Some(((x1, y1), (x2, y2))));
                          } else {
                            Lwt.return(None);
                          }
                        | (Selector(selector, None), _) =>
                          let%lwt ((x1, y1), (x2, y2)) =
                            target |> Browser.Actions.get_quads(~selector);
                          let x1 = Int.of_float(x1);
                          let y1 = Int.of_float(y1);
                          let x2 = Int.of_float(x2);
                          let y2 = Int.of_float(y2);
                          Lwt.return(Some(((x1, y1), (x2, y2))));
                        }
                      });

                 let diff =
                   Diff.diff(
                     ~threshold=test.threshold,
                     ~diffPixel=config.diff_pixel_color,
                     ~ignoreRegions,
                     ~output=diff_image_path,
                     ~original_image_data,
                     ~new_image_data=screenshot,
                   );

                 switch (diff()) {
                 | Ok () =>
                   incr(passed_count);
                   FileUtil.rm(~recurse=true, [new_image_path]);
                   Printer.success_message(~name=test.name, ~width, ~height);
                   Lwt.return();
                 | Error(Layout) =>
                   incr(failed_count);
                   Printer.layout_message(~name=test.name, ~width, ~height);
                   save_screenshot(~path=new_image_path, screenshot);
                 | Error(Pixel(diffCount, diffPercentage)) =>
                   incr(failed_count);
                   Printer.diff_message(
                     ~name=test.name,
                     ~width,
                     ~height,
                     ~diffCount,
                     ~diffPercentage,
                   );
                   save_screenshot(~path=new_image_path, screenshot);
                 };
               };
             };
           },
         )
       });

  let end_time = Unix.gettimeofday();
  let seconds = end_time -. start_time;

  Browser.Launcher.shutdown(browser);

  Printer.stats(
    ~test_count=List.length(tests_to_run),
    ~create_count=create_count^,
    ~passed_count=passed_count^,
    ~failed_count=failed_count^,
    ~skipped_count=List.length(all_tests) - List.length(tests_to_run),
    ~seconds,
  );

  if (failed_count^ == 0) {
    Lwt_result.return();
  } else {
    Lwt_result.fail();
  };
};

let cleanup = (~config_path) => {
  open Fmt;
  print_newline();
  let config = Config.Global.find(~config_path) |> Config.Global.parse;
  let (snapshot_dir, _updated_dir, _diff_dir) =
    init_folder_structure(config);
  let tests = Config.Test.init(config);

  let test_file_paths =
    tests
    |> List.map((test: Config.Types.test) => {
         test.sizes
         |> List.filter_map((size: Config.Types.size) => {
              let Config.Types.{width, height, _} = size;
              let filename = get_filename(test.name, width, height);
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
