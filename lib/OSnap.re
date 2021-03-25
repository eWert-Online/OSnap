module Config = OSnap_Config;
module Browser = OSnap_Browser;

module Diff = OSnap_Diff;
module Printer = OSnap_Printer;

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
  config: Config.Global.t,
  browser: Browser.t,
  tests: list(Config.Test.t),
  snapshot_dir: string,
  updated_dir: string,
  diff_dir: string,
};

let init_folder_structure = config => {
  let base_path =
    config.Config.Global.root_path ++ config.Config.Global.snapshot_directory;

  let snapshot_dir = base_path ++ "/__base_images__";
  if (!Sys.file_exists(snapshot_dir)) {
    FileUtil.mkdir(~parent=true, ~mode=`Octal(0o755), snapshot_dir);
  };

  let updated_dir = base_path ++ "/__updated__";
  FileUtil.rm(~recurse=true, [updated_dir]);
  FileUtil.mkdir(~parent=true, ~mode=`Octal(0o755), updated_dir);

  let diff_dir = base_path ++ "/__diff__";
  FileUtil.rm(~recurse=true, [diff_dir]);
  FileUtil.mkdir(~parent=true, ~mode=`Octal(0o755), diff_dir);

  (snapshot_dir, updated_dir, diff_dir);
};

let setup = () => {
  let config = Config.Global.find() |> Config.Global.parse;
  let (snapshot_dir, updated_dir, diff_dir) = init_folder_structure(config);
  let tests = Config.Test.init(config);

  let%lwt browser = Browser.Launcher.make();

  Lwt.return({config, browser, tests, snapshot_dir, updated_dir, diff_dir});
};

let run = t => {
  let {browser, snapshot_dir, updated_dir, diff_dir, tests, config} = t;

  let create_count = ref(0);
  let passed_count = ref(0);
  let failed_count = ref(0);
  let test_count = ref(0);

  let get_filename = (name, width, height) =>
    Printf.sprintf("/%s_%ix%i.png", name, width, height);

  let all_tests =
    tests
    |> List.map(test => {
         test.Config.Test.sizes |> List.map(size => (test, size))
       })
    |> List.flatten;

  let tests_to_run =
    all_tests
    |> List.find_all_or_input(((test, _)) => test.Config.Test.only)
    |> List.find_all(((test, (width, height))) =>
         if (test.Config.Test.skip) {
           Printer.skipped_message(~name=test.name, ~width, ~height);
           false;
         } else {
           true;
         }
       );

  let pool =
    Lwt_pool.create(config.Config.Global.parallelism, () => {
      Browser.Target.make(browser)
    });

  let%lwt () =
    tests_to_run
    |> Lwt_list.iter_p(((test: Config.Test.t, (width, height))) => {
         Lwt_pool.use(
           pool,
           target => {
             test_count := test_count^ + 1;
             let filename = get_filename(test.name, width, height);
             let current_image_path = snapshot_dir ++ filename;
             let new_image_path = updated_dir ++ filename;
             let diff_image_path = diff_dir ++ filename;

             let url = config.Config.Global.base_url ++ test.url;
             let full_size = config.Config.Global.fullscreen;

             let%lwt () = target |> Browser.Actions.set_size(~width, ~height);
             let%lwt loaderId = target |> Browser.Actions.go_to(~url);
             let%lwt () =
               target |> Browser.Actions.wait_for_network_idle(~loaderId);

             let%lwt screenshot =
               target
               |> Browser.Actions.screenshot(~full_size)
               |> Lwt.map(Base64.decode_exn);

             let create_new = !Sys.file_exists(current_image_path);

             let%lwt io =
               Lwt_io.open_file(
                 ~mode=Output,
                 create_new ? current_image_path : new_image_path,
               );
             let%lwt () = Lwt_io.write(io, screenshot);
             let%lwt () = Lwt_io.close(io);

             if (create_new) {
               create_count := create_count^ + 1;
               passed_count := passed_count^ + 1;
               Printer.created_message(~name=test.name, ~width, ~height);
             } else {
               switch (
                 Diff.diff(
                   current_image_path,
                   new_image_path,
                   ~diffPixel=config.Config.Global.diff_pixel_color,
                   ~output=diff_image_path,
                 )
               ) {
               | Ok () =>
                 passed_count := passed_count^ + 1;
                 FileUtil.rm(~recurse=true, [new_image_path]);
                 Printer.success_message(~name=test.name, ~width, ~height);
               | Error(Layout) =>
                 failed_count := failed_count^ + 1;
                 Printer.layout_message(~name=test.name, ~width, ~height);
               | Error(Pixel(diffCount)) =>
                 failed_count := failed_count^ + 1;
                 Printer.diff_message(
                   ~name=test.name,
                   ~width,
                   ~height,
                   ~diffCount,
                 );
               };
             };
             Lwt.return();
           },
         )
       });

  Printer.stats(
    ~test_count=List.length(tests_to_run),
    ~create_count=create_count^,
    ~passed_count=passed_count^,
    ~failed_count=failed_count^,
    ~skipped_count=List.length(all_tests) - List.length(tests_to_run),
  );

  Lwt.return();
};

let teardown = t => {
  t.browser |> Browser.Launcher.shutdown;
};
