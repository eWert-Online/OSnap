module Io = OSnap_Io;
module Test = OSnap_Test;
module Utils = OSnap_Utils;
module Config = OSnap_Config;
module Browser = OSnap_Browser;
module Websocket = OSnap_Websocket;

module Diff = Odiff.Diff.MakeDiff(Io.PNG, Io.PNG);

let test_name = (~name, ~width, ~height, ()) => {
  let width = Int.to_string(width);
  let height = Int.to_string(height);
  let padding_length = 8 - String.length(width) - String.length(height);
  let padding = padding_length > 0 ? String.make(padding_length, ' ') : "";
  <Pastel>
    name
    " "
    <Pastel dim=true> "(" width "x" height ")" padding </Pastel>
    "\t"
  </Pastel>;
};

let created_message = (~name, ~width, ~height, ()) =>
  <Pastel>
    <Pastel color=Blue bold=true> "CREATE" </Pastel>
    "\t"
    {test_name(~name, ~width, ~height, ())}
  </Pastel>;

let success_message = (~name, ~width, ~height, ()) =>
  <Pastel>
    <Pastel color=Green bold=true> "PASS" </Pastel>
    "\t"
    {test_name(~name, ~width, ~height, ())}
  </Pastel>;

let layout_message = (~name, ~width, ~height, ()) =>
  <Pastel>
    <Pastel color=Red bold=true> "FAIL" </Pastel>
    "\t"
    {test_name(~name, ~width, ~height, ())}
    <Pastel color=Red> "Images have different layout." </Pastel>
  </Pastel>;

let diff_message = (~name, ~width, ~height, ~diffCount, ()) =>
  <Pastel>
    <Pastel color=Red bold=true> "FAIL" </Pastel>
    "\t"
    {test_name(~name, ~width, ~height, ())}
    <Pastel color=Red>
      "Different pixels: "
      {Int.to_string(diffCount)}
    </Pastel>
  </Pastel>;

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

  let create_count = ref(0);
  let passed_count = ref(0);
  let failed_count = ref(0);
  let test_count = ref(0);

  let%lwt () =
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
              test_count := test_count^ + 1;
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

              let%lwt screenshot =
                browser
                |> Browser.screenshot(~full_size=config.Config.fullscreen)
                |> Lwt.map(Base64.decode_exn);

              if (!Sys.file_exists(snapshot_dir ++ filename)) {
                let%lwt io =
                  Lwt_io.open_file(~mode=Output, snapshot_dir ++ filename);
                let%lwt () = Lwt_io.write(io, screenshot);
                let%lwt () = Lwt_io.close(io);
                create_count := create_count^ + 1;
                Console.log(
                  created_message(~name=test.name, ~width, ~height, ()),
                );
                Lwt.return();
              } else {
                let%lwt io =
                  Lwt_io.open_file(~mode=Output, updated_dir ++ filename);
                let%lwt () = Lwt_io.write(io, screenshot);
                let%lwt () = Lwt_io.close(io);

                let img1 = Io.PNG.loadImage(snapshot_dir ++ filename);
                let img2 = Io.PNG.loadImage(updated_dir ++ filename);

                if (img1.width != img2.width || img1.height != img2.height) {
                  failed_count := failed_count^ + 1;
                  Console.log(
                    layout_message(~name=test.name, ~width, ~height, ()),
                  );
                } else {
                  switch (
                    Diff.diff(
                      img1,
                      img2,
                      ~outputDiffMask=true,
                      ~threshold=0.1,
                      ~failOnLayoutChange=true,
                      ~diffPixel=(255, 0, 0),
                      (),
                    )
                  ) {
                  | Pixel((_diffOutput, diffCount)) when diffCount == 0 =>
                    FileUtil.rm(~recurse=true, [updated_dir ++ filename]);
                    passed_count := passed_count^ + 1;
                    Console.log(
                      success_message(~name=test.name, ~width, ~height, ()),
                    );
                  | Layout =>
                    failed_count := failed_count^ + 1;
                    Console.log(
                      layout_message(~name=test.name, ~width, ~height, ()),
                    );
                  | Pixel((diffImg, diffCount)) =>
                    Io.PNG.saveImage(diffImg, diff_dir ++ filename);
                    failed_count := failed_count^ + 1;
                    Console.log(
                      diff_message(
                        ~name=test.name,
                        ~width,
                        ~height,
                        ~diffCount,
                        (),
                      ),
                    );
                  };
                };

                Lwt.return();
              };
            });
       });

  Console.log(
    <Pastel>
      "\n"
      "\n"
      "Done! 🚀\n"
      "I did run "
      <Pastel bold=true>
        {Int.to_string(List.length(tests))}
        " Test-Suites"
      </Pastel>
      " with a total of "
      <Pastel bold=true> {Int.to_string(test_count^)} " snapshots" </Pastel>
      " in "
      <Pastel bold=true> {Float.to_string(Sys.time())} " seconds" </Pastel>
      "!"
      "\n\nResults:\n"
      {create_count^ > 0
         ? <Pastel bold=true>
             {Int.to_string(create_count^)}
             " Snapshots created \n"
           </Pastel>
         : ""}
      <Pastel color=Green bold=true>
        {Int.to_string(passed_count^)}
        " Snapshots passed \n"
      </Pastel>
      {failed_count^ > 0
         ? <Pastel color=Red bold=true>
             {Int.to_string(failed_count^)}
             " Snapshots failed \n"
           </Pastel>
         : ""}
    </Pastel>,
  );

  Lwt.return();
};

let teardown = t => {
  (t.browser.process)#terminate;
};
