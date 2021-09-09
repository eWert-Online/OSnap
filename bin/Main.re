open Cmdliner;

Printexc.record_backtrace(true);
Fmt.set_style_renderer(Fmt.stdout, `Ansi_tty);

let setup_log = {
  Term.(
    const(OSnap.Logger.init) $ Fmt_cli.style_renderer() $ Logs_cli.level()
  );
};

let print_error = msg => {
  Fmt.pr("%a @.", Fmt.styled(`Red, Fmt.string), msg);
};

let config = {
  let doc = "
    The relative path to the global config file.
  ";
  let default = "";
  Arg.(value & opt(file, default) & info(["config"], ~doc));
};

let default_cmd = {
  let noCreate = {
    let doc = "
      With this option enabled, new snapshots will not be created, but fail the whole test run instead.
      This option is recommended for ci environments.
    ";
    Arg.(value & flag & info(["no-create"], ~doc));
  };

  let noOnly = {
    let doc = "
      With this option enabled, the test run will fail, if you have any test with \"only\" set to true.
      This option is recommended for ci environments.
    ";
    Arg.(value & flag & info(["no-only"], ~doc));
  };

  let noSkip = {
    let doc = "
      With this option enabled, the test run will fail, if you have any test with \"skip\" set to true.
      This option is recommended for ci environments.
    ";
    Arg.(value & flag & info(["no-skip"], ~doc));
  };

  let exec = (noCreate, noOnly, noSkip, config_path, ()) => {
    open Lwt_result.Syntax;
    let run = {
      let* t =
        try%lwt(OSnap.setup(~config_path, ~noCreate, ~noOnly, ~noSkip)) {
        | Failure(message) =>
          print_error(message);
          Lwt_result.fail();
        | OSnap_Config.Types.Parse_Error(s) =>
          print_error(
            Printf.sprintf(
              "Your osnap.config.json is in an invalid format: \n%S",
              s,
            ),
          );
          Lwt_result.fail();
        | OSnap_Config.Global.No_Config_Found =>
          print_error("Unable to find a global config file.");
          print_error(
            "Please create a \"osnap.config.json\" at the root of your project or specifiy the location using the --config option.",
          );
          Lwt_result.fail();
        | OSnap_Config.Types.Unsupported_Format =>
          print_error("Your global config file has an unknown format.");
          print_error("Known formats are json and yaml");
          Lwt_result.fail();
        | OSnap_Config.Test.Duplicate_Tests(tests) =>
          print_error(
            "Found some tests with duplicate names. Every test has to have a unique name.",
          );
          print_error("Please rename the following tests: \n");
          tests |> List.iter(print_error);
          Lwt_result.fail();
        | OSnap_Config.Types.Duplicate_Size_Names(sizes) =>
          print_error(
            "Found some sizes with duplicate names. Every size has to have a unique name inside it's list.",
          );
          print_error("Please rename the following sizes: \n");
          sizes |> List.iter(print_error);
          Lwt_result.fail();
        | OSnap_Config.Test.Invalid_format =>
          print_error("Found some tests with an invalid format.");
          Lwt_result.fail();
        | exn => raise(exn)
        };

      let* () =
        try%lwt(OSnap.run(t)) {
        | Failure(message) =>
          print_error(message);
          Lwt_result.fail();
        | exn => raise(exn)
        };

      Lwt_result.return();
    };

    switch (Lwt_main.run(run)) {
    | Ok () => 0
    | Error () => 1
    };
  };

  (
    Term.(const(exec) $ noCreate $ noOnly $ noSkip $ config $ setup_log),
    Term.info(
      "osnap",
      ~man=[
        `S(Manpage.s_description),
        `P(
          "OSnap is a snapshot testing tool, which uses chrome to take screenshots and compares them with a base image taken previously.",
        ),
        `P("If both images are equal, the test passes."),
        `P(
          "If the images aren't equal, the test fails and puts the new image into the \"__updated__\" folder inside of your snapshot folder.
          It also generates a new image, which shows the base image (how it looked before), an image with the differing pixels
          highlighted and the new image side by side.",
        ),
        `P(
          "There is no \"update\" command to update the snapshots. If the changes shown in the diff image are expected,
          you just have to move and replace the image from the \"__updated__\" folder into the \"__base_images__\" folder.",
        ),
      ],
      ~exits=
        Term.[
          exit_info(0, ~doc="on success"),
          exit_info(1, ~doc="on failed test runs"),
          exit_info(124, ~doc="on command line parsing errors."),
          exit_info(125, ~doc="on unexpected internal errors."),
        ],
    ),
  );
};

let cleanup_cmd = {
  let exec = config_path => {
    let result =
      try(OSnap.cleanup(~config_path)) {
      | Failure(message) =>
        print_error(message);
        Result.error();
      | OSnap_Config.Types.Parse_Error(_) =>
        print_error("Your osnap.config.json is in an invalid format.");
        Result.error();
      | OSnap_Config.Global.No_Config_Found =>
        print_error("Unable to find a global config file.");
        print_error(
          "Please create a \"osnap.config.json\" at the root of your project or specifiy the location using the --config option.",
        );
        Result.error();
      | OSnap_Config.Test.Duplicate_Tests(tests) =>
        print_error(
          "Found some tests with duplicate names. Every test has to have a unique name.",
        );
        print_error("Please rename the following tests: \n");
        tests |> List.iter(print_error);
        Result.error();
      | OSnap_Config.Test.Invalid_format =>
        print_error("Found some tests with an invalid format.");
        Result.error();
      | exn => raise(exn)
      };

    switch (result) {
    | Ok () => 0
    | Error () => 1
    };
  };

  (
    Term.(const(exec) $ config),
    Term.info(
      "cleanup",
      ~man=[
        `S(Manpage.s_description),
        `P(
          "
          The cleanup command removes all unused base images from the snapshot folder.
          This may happen, when a test is removed or renamed.
          ",
        ),
      ],
      ~exits=
        Term.[
          exit_info(0, ~doc="on success"),
          exit_info(124, ~doc="on command line parsing errors."),
          exit_info(125, ~doc="on unexpected internal errors."),
        ],
    ),
  );
};

let download_chromium_cmd = {
  let exec = () => {
    let run = {
      try%lwt(OSnap.download_chromium()) {
      | Failure(message) =>
        print_error(message);
        Lwt_result.fail();
      | exn => raise(exn)
      };
    };

    switch (Lwt_main.run(run)) {
    | Ok () => 0
    | Error () => 1
    };
  };

  (
    Term.(const(exec) $ const()),
    Term.info(
      "download-chromium",
      ~man=[
        `S(Manpage.s_description),
        `P(
          "
          The download-chromium command downloads the latest compatible version of chromium.
          ",
        ),
      ],
      ~exits=
        Term.[
          exit_info(0, ~doc="on success"),
          exit_info(124, ~doc="on command line parsing errors."),
          exit_info(125, ~doc="on unexpected internal errors."),
        ],
    ),
  );
};

let cmds = [cleanup_cmd, download_chromium_cmd];

let () = Term.eval_choice(default_cmd, cmds) |> Term.exit_status;
