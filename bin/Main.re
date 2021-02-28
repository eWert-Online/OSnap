Lwt_main.run(
  {
    let%lwt browser = OSnap.Browser.make();

    switch (browser) {
    | Error(e) => Lwt_result.fail(e)
    | Ok(browser) =>
      let%lwt _frameId =
        browser |> OSnap.Browser.go_to("https://www.google.com/");

      let%lwt screenshot = browser |> OSnap.Browser.screenshot;

      print_newline();
      print_newline();
      print_endline(screenshot);
      print_newline();
      print_newline();

      (browser.process)#terminate |> Lwt_result.return;
    };
  },
);
