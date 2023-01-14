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
  let* io = Lwt_io.open_file ~mode:Output zip_path in
  print_endline
    (Printf.sprintf "Downloading Chrome Revision %s. This could take a while..." revision);
  let uri = get_uri revision (OSnap_Utils.detect_platform ()) in
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
    Lwt_result.fail ()
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
  Zip.close_in ic
;;

let download () =
  let revision = OSnap_Browser_Path.get_revision () in
  let extract_path = OSnap_Browser_Path.get_chromium_path () in
  print_newline ();
  print_newline ();
  if (not (Sys.file_exists extract_path)) || not (Sys.is_directory extract_path)
  then (
    Unix.mkdir extract_path 511;
    Lwt_io.with_temp_dir ~prefix:"osnap_chromium_" (fun dir ->
      let*? path = dir |> download ~revision in
      extract_zip path ~dest:extract_path;
      print_endline "Done!";
      Lwt_result.return ()))
  else (
    print_endline
      (Printf.sprintf "Found Chromium at \"%s\". Skipping Download!" extract_path);
    Lwt_result.return ())
;;
