open Cmdliner;

let main = (noCreate, noOnly, noSkip) => {
  open Lwt.Syntax;
  let run = {
    let* t = OSnap.setup();
    let* res =
      try(OSnap.run(t, ~noCreate, ~noOnly, ~noSkip)) {
      | Failure(message) =>
        print_endline(message);
        Lwt_result.fail();
      | exn => raise(exn)
      };
    OSnap.teardown(t);

    let exit_code =
      switch (res) {
      | Ok () => 0
      | Error () => 1
      };

    Lwt.return(exit_code);
  };

  Lwt_main.run(run);
};

let info =
  Term.info(
    "osnap",
    ~version="0.0.1",
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
  );

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

let cmd = Term.(const(main) $ noCreate $ noOnly $ noSkip);

let () = Term.eval((cmd, info)) |> Term.exit_status;
