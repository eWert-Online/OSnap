module IO = OSnap.Io.IO;
module Diff = Odiff.Diff.MakeDiff(IO, IO);

Lwt_main.run(
  {
    let config = OSnap.Config.find();
    let _ = config |> Result.map(print_endline);
    let config = config |> Result.bind(_, OSnap.Config.parse);
    switch (config) {
    | Ok(config) =>
      let%lwt browser = OSnap.Browser.make();

      switch (browser) {
      | Error(e) => Lwt_result.fail(e)
      | Ok(browser) =>
        let%lwt _ = browser |> OSnap.Browser.go_to("https://www.google.com/");

        let%lwt screenshot = browser |> OSnap.Browser.screenshot;
        let data = screenshot |> Base64.decode_exn;
        print_endline(data);
        let%lwt io =
          Lwt_io.open_file(
            ~mode=Output,
            config.OSnap.Config.root_path
            ++ config.OSnap.Config.snapshot_directory
            ++ "/test.png",
          );

        let%lwt () = Lwt_io.write(io, data);

        (browser.process)#terminate |> Lwt_result.return;
      };
    | Error(e) => print_endline(e) |> Lwt_result.return
    };
  },
);
