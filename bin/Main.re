open OSnap;
module IO = OSnap.Io.IO;
module Diff = Odiff.Diff.MakeDiff(IO, IO);

module TestSet =
  Set.Make({
    type t = Test.t;
    let compare = (a, b) => {
      String.compare(a.Test.name, b.Test.name);
    };
  });

Lwt_main.run(
  {
    let config = Config.find() |> Result.bind(_, Config.parse);

    switch (config) {
    | Ok(config) =>
      let tests =
        Test.find(
          ~root_path=config.Config.root_path,
          ~pattern=config.Config.test_pattern,
          ~ignore_patterns=config.Config.ignore_patterns,
          (),
        )
        |> List.map(Test.parse)
        |> List.flatten;

      let duplicates = tests |> Utils.find_duplicates(t => t.Test.name);

      if (List.length(duplicates) != 0) {
        print_endline("Found tests with the same name.");
        duplicates |> List.iter(test => print_endline(test.Test.name));
        Lwt_result.return();
      } else {
        print_endline(
          "Found " ++ string_of_int(List.length(tests)) ++ " Test-Files:",
        );

        tests |> List.iter(test => print_endline(test.Test.name));

        Lwt_result.return();
        // let%lwt browser = Browser.make();
        // switch (browser) {
        // | Error(e) => Lwt_result.fail(e)
        // | Ok(browser) =>
        //   let%lwt _ = browser |> Browser.go_to("https://www.google.com/");
        //   let%lwt screenshot = browser |> Browser.screenshot;
        //   let data = screenshot |> Base64.decode_exn;
        //   print_endline(data);
        //   let%lwt io =
        //     Lwt_io.open_file(
        //       ~mode=Output,
        //       config.Config.root_path
        //       ++ config.Config.snapshot_directory
        //       ++ "/test.png",
        //     );
        //   let%lwt () = Lwt_io.write(io, data);
        //   (browser.process)#terminate |> Lwt_result.return;
        // };
      };

    | Error(e) => print_endline(e) |> Lwt_result.return
    };
  },
);
