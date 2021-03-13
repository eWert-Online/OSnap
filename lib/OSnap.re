module Io = OSnap_Io;
module Test = OSnap_Test;
module Utils = OSnap_Utils;
module Config = OSnap_Config;
module Browser = OSnap_Browser;
module Websocket = OSnap_Websocket;

module Diff = Odiff.Diff.MakeDiff(Io.PNG, Io.PNG);

type t = {
  config: Config.t,
  browser: Browser.t,
  tests: list(Test.t),
  snapshot_dir: string,
  updated_dir: string,
  diff_dir: string,
};

let init_folder_structure = config => {
  let snapshot_dir =
    config.Config.root_path ++ config.Config.snapshot_directory;
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
  let config = Config.find() |> Config.parse;
  let (snapshot_dir, updated_dir, diff_dir) = init_folder_structure(config);
  let tests = Test.init(config);

  let%lwt browser = Browser.make();

  Lwt.return({config, browser, tests, snapshot_dir, updated_dir, diff_dir});
};

let run = t => {
  let {browser, snapshot_dir, updated_dir, diff_dir, tests, config} = t;

  tests
  |> Lwt_list.iter_s(test => {
       open Test;

       let sizes =
         switch (test.sizes) {
         | Some(sizes) => sizes
         | None => config.Config.default_sizes
         };

       let url = config.Config.base_url ++ test.url;

       sizes
       |> Lwt_list.iter_s(((width, height)) => {
            let name =
              test.name
              ++ "_"
              ++ string_of_int(width)
              ++ "x"
              ++ string_of_int(height);
            let filename = "/" ++ name ++ ".png";

            let%lwt () = browser |> Browser.set_size(~width, ~height);

            let%lwt _ = browser |> Browser.go_to(url);
            let%lwt () = Browser.wait_for("Page.loadEventFired");

            let%lwt screenshot = browser |> Browser.screenshot;
            let data = screenshot |> Base64.decode_exn;

            if (Sys.file_exists(snapshot_dir ++ filename)) {
              let%lwt io =
                Lwt_io.open_file(~mode=Output, updated_dir ++ filename);
              let%lwt () = Lwt_io.write(io, data);
              let%lwt () = Lwt_io.close(io);

              let img1 = Io.PNG.loadImage(snapshot_dir ++ filename);
              let img2 = Io.PNG.loadImage(updated_dir ++ filename);

              let (img, diffPixels) = Diff.compare(img1, img2, ());

              if (diffPixels == 0) {
                FileUtil.rm(~recurse=true, [updated_dir ++ filename]);
              } else {
                Io.PNG.saveImage(img, diff_dir ++ filename);
              };
              Lwt.return();
            } else {
              let%lwt io =
                Lwt_io.open_file(~mode=Output, snapshot_dir ++ filename);
              let%lwt () = Lwt_io.write(io, data);
              let%lwt () = Lwt_io.close(io);
              Lwt.return();
            };
          });
     });
};

let teardown = t => {
  (t.browser.process)#terminate;
};
