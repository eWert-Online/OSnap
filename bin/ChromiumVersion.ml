open Cohttp_lwt_unix
open Lwt.Syntax

type platform =
  | Win64
  | MacOS
  | MacOS_ARM
  | Linux
;;

Fmt.set_style_renderer Fmt.stdout `Ansi_tty

let print_green msg =
  let printer = Fmt.pr " %a " (Fmt.styled `Green Fmt.string) in
  printer msg
;;

let print_red msg =
  let printer = Fmt.pr " %a " (Fmt.styled `Red Fmt.string) in
  printer msg
;;

let check_availability revision platform =
  let uri =
    Uri.make
      ~scheme:"http"
      ~host:"storage.googleapis.com"
      ~port:80
      ~path:
        (match platform with
         | MacOS -> "/chromium-browser-snapshots/Mac/" ^ revision ^ "/chrome-mac.zip"
         | MacOS_ARM ->
           "/chromium-browser-snapshots/Mac_Arm/" ^ revision ^ "/chrome-mac.zip"
         | Linux ->
           "/chromium-browser-snapshots/Linux_x64/" ^ revision ^ "/chrome-linux.zip"
         | Win64 -> "/chromium-browser-snapshots/Win_x64/" ^ revision ^ "/chrome-win.zip")
      ()
  in
  let* response = Client.head uri in
  match response with
  | { status = `OK; _ } -> Lwt_result.return platform
  | _ -> Lwt_result.fail platform
;;

let platform_to_string = function
  | Win64 -> "win64"
  | MacOS -> "mac"
  | MacOS_ARM -> "mac_arm"
  | Linux -> "linux"
;;

let rec check_revision revision =
  let revision_str = string_of_int revision in
  let* results =
    Lwt.all
      [ check_availability revision_str Linux
      ; check_availability revision_str MacOS
      ; check_availability revision_str MacOS_ARM
      ; check_availability revision_str Win64
      ]
  in
  flush stdout;
  print_int revision;
  print_string ":";
  let _available, not_available =
    results
    |> List.partition_map (function
         | Result.Ok platform ->
           print_green (platform_to_string platform);
           Either.left platform
         | Result.Error platform ->
           print_red (platform_to_string platform);
           Either.right platform)
  in
  flush stdout;
  print_endline "";
  match not_available with
  | [] -> Lwt.return revision
  | _ -> check_revision (pred revision)
;;

Lwt_main.run
  (let uri = Uri.make ~scheme:"http" ~host:"storage.googleapis.com" ~port:80 in
   let* latestRevisions =
     Lwt.all
       [ Client.get (uri ~path:"/chromium-browser-snapshots/Mac/LAST_CHANGE" ())
       ; Client.get (uri ~path:"/chromium-browser-snapshots/Mac_Arm/LAST_CHANGE" ())
       ; Client.get (uri ~path:"/chromium-browser-snapshots/Linux_x64/LAST_CHANGE" ())
       ; Client.get (uri ~path:"/chromium-browser-snapshots/Win_x64/LAST_CHANGE" ())
       ]
   in
   let* latestRevisions =
     latestRevisions
     |> Lwt_list.map_p (function
          | { Response.status = `OK; _ }, body -> Cohttp_lwt.Body.to_string body
          | response, _body ->
            Format.fprintf
              Format.err_formatter
              "Latest version could not be checked:\n%a\n%!"
              Response.pp_hum
              response;
            exit 1)
   in
   latestRevisions
   |> List.map int_of_string
   |> List.fold_left min Int.max_int
   |> check_revision)
