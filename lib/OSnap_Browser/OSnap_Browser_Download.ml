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

let download ~env ~revision dir =
  let ( let*? ) = Result.bind in
  let zip_path = Eio.Path.(dir / "chromium.zip") in
  let revision_string = OSnap_Browser_Path.revision_to_string revision in
  let uri = get_uri revision_string (OSnap_Utils.detect_platform ()) in
  print_endline
    (Printf.sprintf
       "Downloading chromium revision %s.\n\
        This is a one time setup and will only happen, if there are updates from OSnap..."
       revision_string);
  Eio.Switch.run
  @@ fun sw ->
  let*? client =
    Piaf.Client.create
      ~sw
      ~config:
        { Piaf.Config.default with
          follow_redirects = true
        ; allow_insecure = true
        ; flush_headers_immediately = true
        }
      env
      uri
    |> Result.map_error (fun _ -> `OSnap_Chromium_Download_Failed)
  in
  Eio.Path.with_open_out ~create:(`Or_truncate 0o755) zip_path
  @@ fun io ->
  let*? response =
    Piaf.Client.get client (Uri.path_and_query uri)
    |> Result.map_error (fun _ -> `OSnap_Chromium_Download_Failed)
  in
  match response with
  | { status = `OK; _ } ->
    let*? () =
      Piaf.Body.iter_string ~f:(fun chunk -> Eio.Flow.copy_string chunk io) response.body
      |> Result.map_error (fun _ -> `OSnap_Chromium_Download_Failed)
    in
    Piaf.Client.shutdown client;
    Result.ok zip_path
  | response ->
    Format.fprintf
      Format.err_formatter
      "Chrome could not be downloaded:\n%a\n%!"
      Piaf.Response.pp_hum
      response;
    Result.error `OSnap_Chromium_Download_Failed
;;

let extract_zip ~dest source =
  let extract_entry in_file (entry : Zip.entry) =
    let out_file = Eio.Path.(dest / entry.name) in
    if entry.is_directory && not (Eio.Path.is_directory out_file)
    then Eio.Path.mkdirs ~perm:0o755 out_file
    else (
      let parent_dir =
        Eio.Path.split out_file |> Option.map fst |> Option.value ~default:dest
      in
      if not (Eio.Path.is_directory parent_dir)
      then Eio.Path.mkdirs ~perm:0o755 parent_dir;
      let oc =
        open_out_gen
          [ Open_creat; Open_binary; Open_append ]
          511
          (Eio.Path.native_exn out_file)
      in
      try
        Zip.copy_entry_to_channel in_file entry oc;
        close_out oc
      with
      | err ->
        close_out oc;
        Eio.Path.rmtree ~missing_ok:true out_file;
        raise err)
  in
  print_endline "Extracting Chromium...";
  let ic = Zip.open_in (Eio.Path.native_exn source) in
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
      Printf.sprintf "Removing old chrome revision at path %s ..." path |> print_endline;
      FileUtil.rm ~recurse:true ~force:Force [ path ])
;;

let download ~env revision =
  let ( let*? ) = Result.bind in
  print_newline ();
  print_newline ();
  let fs = Eio.Stdenv.fs env in
  let*? () =
    if is_revision_downloaded revision
    then Result.ok ()
    else (
      let extract_path = Eio.Path.(fs / OSnap_Browser_Path.get_chromium_path revision) in
      Eio.Path.mkdirs ~exists_ok:true ~perm:0o755 extract_path;
      let _temp_dir = Filename.get_temp_dir_name () in
      Eio.Path.with_open_dir
        Eio.Path.(fs / "/tmp")
        (fun dir ->
          let*? path = dir |> download ~env ~revision in
          extract_zip path ~dest:extract_path;
          Result.ok ()))
  in
  Result.ok (cleanup_old_revisions ())
;;
