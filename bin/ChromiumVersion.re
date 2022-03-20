open Cohttp_lwt_unix;
open Lwt.Syntax;

Fmt.set_style_renderer(Fmt.stdout, `Ansi_tty);

let print_green = msg => {
  let printer = Fmt.pr(" %a ", Fmt.styled(`Green, Fmt.string));
  printer(msg);
};

let print_red = msg => {
  let printer = Fmt.pr(" %a ", Fmt.styled(`Red, Fmt.string));
  printer(msg);
};

let check_availability = (revision, platform) => {
  let uri = OSnap_Browser.Download.get_uri(revision, platform);

  let* response = Client.head(uri);

  switch (response) {
  | {status: `OK, _} => Lwt_result.return(platform)
  | _ => Lwt_result.fail(platform)
  };
};

let platform_to_string =
  fun
  | OSnap_Utils.Win32 => "win32"
  | OSnap_Utils.Win64 => "win64"
  | OSnap_Utils.MacOS => "mac"
  | OSnap_Utils.MacOS_ARM => "mac_arm"
  | OSnap_Utils.Linux => "linux";

let rec check_revision = revision => {
  let revision_str = string_of_int(revision);
  let* results =
    Lwt.all([
      check_availability(revision_str, OSnap_Utils.Linux),
      check_availability(revision_str, OSnap_Utils.MacOS),
      check_availability(revision_str, OSnap_Utils.MacOS_ARM),
      check_availability(revision_str, OSnap_Utils.Win64),
    ]);

  flush(stdout);
  print_int(revision);
  print_string(":");

  let (_available, not_available) =
    results
    |> List.partition_map(
         fun
         | Result.Ok(platform) => {
             print_green(platform_to_string(platform));
             Either.left(platform);
           }
         | Result.Error(platform) => {
             print_red(platform_to_string(platform));
             Either.right(platform);
           },
       );

  flush(stdout);
  print_endline("");

  switch (not_available) {
  | [] => Lwt.return(revision)
  | _ => check_revision(pred(revision))
  };
};

Lwt_main.run(
  {
    let uri =
      Uri.make(~scheme="http", ~host="storage.googleapis.com", ~port=80);

    let* latestRevisions =
      Lwt.all([
        Client.get(
          uri(~path="/chromium-browser-snapshots/Mac/LAST_CHANGE", ()),
        ),
        Client.get(
          uri(~path="/chromium-browser-snapshots/Mac_Arm/LAST_CHANGE", ()),
        ),
        Client.get(
          uri(~path="/chromium-browser-snapshots/Linux_x64/LAST_CHANGE", ()),
        ),
        Client.get(
          uri(~path="/chromium-browser-snapshots/Win_x64/LAST_CHANGE", ()),
        ),
      ]);
    let* latestRevisions =
      latestRevisions
      |> Lwt_list.map_p(
           fun
           | ({Response.status: `OK, _}, body) =>
             Cohttp_lwt.Body.to_string(body)
           | (response, _body) => {
               Format.fprintf(
                 Format.err_formatter,
                 "Latest version could not be checked:\n%a\n%!",
                 Response.pp_hum,
                 response,
               );
               exit(1);
             },
         );

    latestRevisions
    |> List.map(int_of_string)
    |> List.fold_left(min, Int.max_int)
    |> check_revision;
  },
);
