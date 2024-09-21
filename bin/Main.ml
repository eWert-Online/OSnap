open Cmdliner;;

Fmt.set_style_renderer Fmt.stdout `Ansi_tty

let print_warning msg =
  let printer = Fmt.pr "%a @." (Fmt.styled `Yellow Fmt.string) in
  Printf.ksprintf printer msg
;;

let print_error msg =
  let printer = Fmt.pr "%a @." (Fmt.styled `Red Fmt.string) in
  Printf.ksprintf printer msg
;;

let handle_response response =
  match response with
  | Ok () -> 0
  | Error `OSnap_Test_Failure -> 1
  | Error `OSnap_Chromium_Download_Failed -> 1
  | Error (`OSnap_Config_Duplicate_Tests tests) ->
    print_error
      "Found some tests with duplicate names. Every test has to have a unique name.";
    print_error "Please rename the following tests: \n";
    tests |> List.iter (print_error "%s");
    1
  | Error `OSnap_Config_Global_Not_Found ->
    print_error "Unable to find a global config file.";
    print_error
      "Please create a \"osnap.config.json\" at the root of your project or specifiy the \
       location using the --config option.";
    1
  | Error (`OSnap_Config_Global_Invalid s) ->
    print_error "Your global config file is invalid.";
    print_error "%s" s;
    1
  | Error (`OSnap_Config_Unsupported_Format path) ->
    let path = Eio.Path.native_exn path in
    print_error "Your config file has an unknown format.";
    print_error "Tried to parse %s." path;
    print_error "Known formats are json and yaml";
    1
  | Error `OSnap_CDP_Connection_Failed ->
    print_error "Could not connect to Chrome.";
    1
  | Error (`OSnap_Config_Parse_Error (msg, path)) ->
    let path = Eio.Path.native_exn path in
    print_error "Your config is in an invalid format";
    print_error "Tried to parse %s" path;
    print_error "%s" msg;
    1
  | Error (`OSnap_Config_Invalid (s, path)) ->
    let path = Eio.Path.native_exn path in
    print_error "Found some tests with an invalid format.";
    print_error "Tried to parse %s" path;
    print_error "%s" s;
    1
  | Error (`OSnap_Config_Undefined_Function (s, path)) ->
    let path = Eio.Path.native_exn path in
    print_error "Tried to call non existant function `%s` in file %s" s path;
    1
  | Error (`OSnap_Config_Duplicate_Size_Names sizes) ->
    print_error
      "Found some sizes with duplicate names. Every size has to have a unique name \
       inside it's list.";
    print_error "Please rename the following sizes: \n";
    sizes |> List.iter (print_error "%s");
    1
  | Error (`OSnap_CDP_Protocol_Error e) ->
    print_error "CDP failed to run some commands. Message was: \n";
    print_error "%s" e;
    1
  | Error (`OSnap_Invalid_Run msg) ->
    print_error "%s" msg;
    1
  | Error (`OSnap_FS_Error _) -> 1
  | Error (`OSnap_Unknown_Error exn) ->
    print_error "An unexpected error occured: \n";
    print_error "%s" (Printexc.to_string exn);
    1
;;

let config =
  let doc = "The relative path to the global config file." in
  let default = "" in
  let open Arg in
  value & opt file default & info [ "config" ] ~doc
;;

let default_cmd =
  let noCreate =
    let doc =
      "With this option enabled, new snapshots will not be created, but fail the whole \
       test run instead. This option is recommended for ci environments."
    in
    let open Arg in
    value & flag & info [ "no-create" ] ~doc
  in
  let noOnly =
    let doc =
      "With this option enabled, the test run will fail, if you have any test with \
       \"only\" set to true. This option is recommended for ci environments."
    in
    let open Arg in
    value & flag & info [ "no-only" ] ~doc
  in
  let noSkip =
    let doc =
      "With this option enabled, the test run will fail, if you have any test with \
       \"skip\" set to true. This option is recommended for ci environments."
    in
    let open Arg in
    value & flag & info [ "no-skip" ] ~doc
  in
  let parallelism =
    let doc =
      "Overwrite the parallelism defined in the global config file with the specified \
       value. (DEPRECATED)"
    in
    let open Arg in
    value & opt (some int) None & info [ "p"; "parallelism" ] ~doc
  in
  let exec noCreate noOnly noSkip parallelism config_path =
    let ( let*? ) = Result.bind in
    let run ~sw ~env =
      let*? t = OSnap.setup ~sw ~env ~config_path ~noCreate ~noOnly ~noSkip in
      let result =
        try OSnap.run ~env t with
        | exn -> Result.error (`OSnap_Unknown_Error exn)
      in
      let () =
        match parallelism, t.config.parallelism with
        | None, None -> ()
        | Some _, _ ->
          print_warning
            "\n\
             The --parallelism cli flag is deprecated. The best suitable setting is now \
             determined automatically.\n\
             Please remove the flag from your scripts."
        | _, Some _ ->
          print_warning
            "\n\
             The parallelism option is deprecated. The best suitable setting is now \
             determined automatically.\n\
             Please remove the configuration setting from your global osnap config file."
      in
      result
    in
    handle_response
    @@ Eio_main.run
    @@ fun env ->
    Mirage_crypto_rng_eio.run (module Mirage_crypto_rng.Fortuna) env
    @@ fun () ->
    Lwt_eio.with_event_loop ~clock:(Eio.Stdenv.clock env)
    @@ fun _ -> Eio.Switch.run @@ fun sw -> run ~sw ~env
  in
  ( (let open Term in
     const exec $ noCreate $ noOnly $ noSkip $ parallelism $ config)
  , Cmd.info
      "osnap"
      ~man:
        [ `S Manpage.s_description
        ; `P
            "OSnap is a snapshot testing tool, which uses chrome to take screenshots and \
             compares them with a base image taken previously."
        ; `P "If both images are equal, the test passes."
        ; `P
            "If the images aren't equal, the test fails and puts the new image into the \
             \"__updated__\" folder inside of your snapshot folder. It also generates a \
             new image, which shows the base image (how it looked before), an image with \
             the differing pixels highlighted and the new image side by side."
        ; `P
            "There is no \"update\" command to update the snapshots. If the changes \
             shown in the diff image are expected, you just have to move and replace the \
             image from the \"__updated__\" folder into the \"__base_images__\" folder."
        ]
      ~exits:
        (let open Cmd.Exit in
         [ info 0 ~doc:"on success"
         ; info 1 ~doc:"on failed test runs"
         ; info 124 ~doc:"on command line parsing errors."
         ; info 125 ~doc:"on unexpected internal errors."
         ]) )
;;

let cleanup_cmd =
  let exec config_path =
    handle_response @@ Eio_main.run @@ fun env -> OSnap.cleanup ~env ~config_path
  in
  ( (let open Term in
     const exec $ config)
  , Cmd.info
      "cleanup"
      ~man:
        [ `S Manpage.s_description
        ; `P
            "The cleanup command removes all unused base images from the snapshot \
             folder. This may happen, when a test is removed or renamed."
        ]
      ~exits:
        (let open Cmd.Exit in
         [ info 0 ~doc:"on success"
         ; info 124 ~doc:"on command line parsing errors."
         ; info 125 ~doc:"on unexpected internal errors."
         ]) )
;;

let cmds = [ Cmd.v (snd cleanup_cmd) (fst cleanup_cmd) ]
let default, info = default_cmd

let () =
  Printexc.record_backtrace true;
  cmds |> Cmd.group ~default info |> Cmd.eval' |> exit
;;
