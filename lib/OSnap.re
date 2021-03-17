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
  let snapshot_dir =
    config.Config.Global.root_path ++ config.Config.Global.snapshot_directory;
  if (!Sys.file_exists(snapshot_dir)) {
    FileUtil.mkdir(~parent=true, ~mode=`Octal(0o755), snapshot_dir);
  };

  let updated_dir = snapshot_dir ++ "/__updated__";
  FileUtil.rm(~recurse=true, [updated_dir]);
  FileUtil.mkdir(~parent=true, ~mode=`Octal(0o755), updated_dir);

  let diff_dir = snapshot_dir ++ "/__diff__";
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

  let%lwt () =
    tests
    |> Lwt_list.iter_s(test => {
         open Config.Test;

         let sizes =
           switch (test.sizes) {
           | Some(sizes) => sizes
           | None => config.Config.Global.default_sizes
           };

         let url = config.Config.Global.base_url ++ test.url;

         sizes
         |> Lwt_list.iter_s(((width, height)) => {
              test_count := test_count^ + 1;
              let name =
                test.name
                ++ "_"
                ++ string_of_int(width)
                ++ "x"
                ++ string_of_int(height);
              let filename = "/" ++ name ++ ".png";

              let%lwt () =
                browser |> Browser.Actions.set_size(~width, ~height);

              let%lwt _ = browser |> Browser.Actions.go_to(url);
              let%lwt () = Browser.Actions.wait_for("Page.loadEventFired");

              let%lwt screenshot =
                browser
                |> Browser.Actions.screenshot(
                     ~full_size=config.Config.Global.fullscreen,
                   )
                |> Lwt.map(Base64.decode_exn);

              if (!Sys.file_exists(snapshot_dir ++ filename)) {
                let%lwt io =
                  Lwt_io.open_file(~mode=Output, snapshot_dir ++ filename);
                let%lwt () = Lwt_io.write(io, screenshot);
                let%lwt () = Lwt_io.close(io);
                create_count := create_count^ + 1;
                Printer.created_message(~name=test.name, ~width, ~height);
                Lwt.return();
              } else {
                let%lwt io =
                  Lwt_io.open_file(~mode=Output, updated_dir ++ filename);
                let%lwt () = Lwt_io.write(io, screenshot);
                let%lwt () = Lwt_io.close(io);

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

                Lwt.return();
              };
            });
       });

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
