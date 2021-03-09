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

        let%lwt browser = Browser.make();
        switch (browser) {
        | Error(e) => Lwt_result.fail(e)
        | Ok(browser) =>
          let%lwt () = browser |> Browser.set_size(~width=42, ~height=50);
          let%lwt _ = browser |> Browser.go_to("https://ocaml.org/learn");
          let%lwt () = Browser.wait_for("Page.loadEventFired");
          let%lwt screenshot = browser |> Browser.screenshot;
          let data = screenshot |> Base64.decode_exn;

          let%lwt io =
            Lwt_io.open_file(~mode=Output, updated_dir ++ "/test.png");
          let%lwt () = Lwt_io.write(io, data);
          let%lwt () = Lwt_io.close(io);

          let img1 = IO.loadImage(snapshot_dir ++ "/test.png");
          let img2 = IO.loadImage(updated_dir ++ "/test.png");

          let (img, diffPixels) = Diff.compare(img1, img2, ());

          if (diffPixels == 0) {
            FileUtil.rm(~recurse=true, [updated_dir ++ "/test.png"]);
          } else {
            IO.saveImage(img, diff_dir ++ "/test.png");
          };

          (browser.process)#terminate |> Lwt_result.return;
        };
      };

    | Error(e) => print_endline(e) |> Lwt_result.return
    };
  },
);
