open Cohttp_lwt_unix
open OSnap_Utils

let get_uri revision (platform : OSnap_Utils.platform) =
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
       | Win64 -> "/chromium-browser-snapshots/Win_x64/" ^ revision ^ "/chrome-win.zip"
       | Win32 ->
         print_endline "Error: x86 is currently not supported on Windows";
         exit 1)
    ()
;;

let download ~revision dir =
  let zip_path = Filename.concat dir "chromium.zip" in
  let revision_string = OSnap_Browser_Path.revision_to_string revision in
  let* io = Lwt_io.open_file ~mode:Output zip_path in
  print_endline
    (Printf.sprintf
       "Downloading chromium revision %s.\n\
        This is a one time setup and will only happen, if there are updates from OSnap..."
       revision_string);
  let uri = get_uri revision_string (OSnap_Utils.detect_platform ()) in
  let* response, body = Client.get uri in
  match response with
  | { status = `OK; _ } ->
    let* () = Cohttp_lwt.Body.to_stream body |> Lwt_stream.iter_s (Lwt_io.write io) in
    let* () = Lwt_io.close io in
    zip_path |> Lwt_result.return
  | response ->
    let* () = Lwt_io.close io in
    Format.fprintf
      Format.err_formatter
      "Chrome could not be downloaded:\n%a\n%!"
      Response.pp_hum
      response;
    Lwt_result.fail `OSnap_Chromium_Download_Failed
;;

let extract_zip ?(dest = "") source =
  let extract_entry in_file (entry : Zip.entry) =
    let out_file = Filename.concat dest entry.name in
    if entry.is_directory && not (Sys.file_exists out_file)
    then FileUtil.mkdir ~parent:true ~mode:(`Octal 511) out_file
    else (
      let parent_dir = FilePath.dirname out_file in
      if not (Sys.file_exists parent_dir)
      then FileUtil.mkdir ~parent:true ~mode:(`Octal 511) parent_dir;
      let oc = open_out_gen [ Open_creat; Open_binary; Open_append ] 511 out_file in
      try
        Zip.copy_entry_to_channel in_file entry oc;
        close_out oc
      with
      | err ->
        close_out oc;
        Sys.remove out_file;
        raise err)
  in
  print_endline "Extracting Chromium...";
  let ic = Zip.open_in source in
  ic |> Zip.entries |> List.iter (extract_entry ic);
  Zip.close_in ic;
  print_endline "Done!"
;;

let get_downloaded_revision revision =
  let p = OSnap_Browser_Path.get_chromium_path revision in
  if Sys.file_exists p && Sys.is_directory p then Some p else None
;;

let is_revision_downloaded revision =
  let p = OSnap_Browser_Path.get_chromium_path revision in
  Sys.file_exists p && Sys.is_directory p
;;

let cleanup_old_revisions () =
  let old_revisions = OSnap_Browser_Path.get_previous_revisions () in
  old_revisions
  |> List.iter (fun revision ->
       match get_downloaded_revision revision with
       | None -> ()
       | Some path ->
         Printf.sprintf "Removing old chrome revision at path %s ..." path
         |> print_endline;
         FileUtil.rm ~recurse:true ~force:Force [ path ])
;;

let download revision =
  print_newline ();
  print_newline ();
  let*? () =
    if not (is_revision_downloaded revision)
    then (
      let extract_path = OSnap_Browser_Path.get_chromium_path revision in
      Unix.mkdir extract_path 511;
      Lwt_io.with_temp_dir ~prefix:"osnap_chromium_" (fun dir ->
        let*? path = dir |> download ~revision in
        extract_zip path ~dest:extract_path;
        Lwt_result.return ()))
    else Lwt_result.return ()
  in
  cleanup_old_revisions () |> Lwt_result.return
;;
