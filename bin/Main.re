open OSnap;
module IO = OSnap.Io.IO;
module Diff = Odiff.Diff.MakeDiff(IO, IO);

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
        );

      print_endline(
        "Found " ++ string_of_int(List.length(tests)) ++ " Test-Files:",
      );

      tests |> List.iter(print_endline);

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
    | Error(e) => print_endline(e) |> Lwt_result.return
    };
  },
);
