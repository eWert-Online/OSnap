module Config = OSnap_Config;
module Browser = OSnap_Browser;

module Diff = OSnap_Diff;
module Printer = OSnap_Printer;

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
  let%lwt browser =
    browser
    |> Browser.Launcher.create_targets(config.Config.Global.parallelism);

  Lwt.return({config, browser, tests, snapshot_dir, updated_dir, diff_dir});
};

let run = t => {
  let {browser, snapshot_dir, updated_dir, diff_dir, tests, config} = t;

  let create_count = ref(0);
  let passed_count = ref(0);
  let failed_count = ref(0);
  let test_count = ref(0);

  let target_queue = Queue.create();
  browser
  |> Browser.Launcher.get_targets
  |> List.iter(target => Queue.add(target, target_queue));

  let rec get_available_target = () => {
    switch (Queue.take_opt(target_queue)) {
    | Some(target) => Lwt.return(target)
    | None =>
      let%lwt () = Lwt.pause();
      get_available_target();
    };
  };

  let get_filename = (name, width, height) =>
    Printf.sprintf("/%s_%ix%i.png", name, width, height);

  let tests =
    tests
    |> List.map(test => {
         test.Config.Test.sizes
         |> Option.value(~default=config.Config.Global.default_sizes)
         |> List.map(size => (test, size))
       })
    |> List.flatten;

  let diff_queue = Queue.create();
  let screenshots_done = ref(false);

  let rec diff_task = () =>
    switch (Queue.take_opt(diff_queue)) {
    | None when screenshots_done^ => Lwt.return()
    | None =>
      let%lwt () = Lwt.pause();
      diff_task();
    | Some((name, width, height)) =>
      let filename = get_filename(name, width, height);
      let current_image_path = snapshot_dir ++ filename;
      let new_image_path = updated_dir ++ filename;
      let diff_image_path = diff_dir ++ filename;

      let result =
        Diff.diff(
          current_image_path,
          new_image_path,
          ~diffPixel=config.Config.Global.diff_pixel_color,
          ~output=diff_image_path,
        );

      switch (result) {
      | Ok () =>
        passed_count := passed_count^ + 1;
        FileUtil.rm(~recurse=true, [new_image_path]);
        Printer.success_message(~name, ~width, ~height);
      | Error(Layout) =>
        failed_count := failed_count^ + 1;
        Printer.layout_message(~name, ~width, ~height);
      | Error(Pixel(diffCount)) =>
        failed_count := failed_count^ + 1;
        Printer.diff_message(~name, ~width, ~height, ~diffCount);
      };

      diff_task();
    };

  let screenshot_task =
    tests
    |> Lwt_list.iter_p(((test: Config.Test.t, (width, height))) => {
         let%lwt target = get_available_target();
         test_count := test_count^ + 1;
         let url = config.Config.Global.base_url ++ test.url;
         let filename = get_filename(test.name, width, height);
         let full_size = config.Config.Global.fullscreen;

         let%lwt () = target |> Browser.Actions.set_size(~width, ~height);
         let%lwt loaderId = target |> Browser.Actions.go_to(~url);
         let%lwt () =
           target |> Browser.Actions.wait_for_network_idle(~loaderId);

         let%lwt screenshot =
           target
           |> Browser.Actions.screenshot(~full_size)
           |> Lwt.map(Base64.decode_exn);

         let create_new = !Sys.file_exists(snapshot_dir ++ filename);

         let%lwt io =
           Lwt_io.open_file(
             ~mode=Output,
             create_new ? snapshot_dir ++ filename : updated_dir ++ filename,
           );
         let%lwt () = Lwt_io.write(io, screenshot);
         let%lwt () = Lwt_io.close(io);

         if (create_new) {
           create_count := create_count^ + 1;
           passed_count := passed_count^ + 1;
           Printer.created_message(~name=test.name, ~width, ~height);
         } else {
           diff_queue |> Queue.add((test.name, width, height));
         };

         target_queue |> Queue.add(target);

         Lwt.return();
       });

  let%lwt () = Lwt.pick([screenshot_task, diff_task()]);
  screenshots_done := true;
  let%lwt () = diff_task();

  Printer.stats(
    ~test_suites=List.length(tests),
    ~test_count=test_count^,
    ~create_count=create_count^,
    ~passed_count=passed_count^,
    ~failed_count=failed_count^,
  );

  Lwt.return();
};

let teardown = t => {
  t.browser |> Browser.Launcher.shutdown;
};
