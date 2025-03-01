open OSnap_Utils

let get_uri ~version (platform : OSnap_Utils.platform) =
  let host = "storage.googleapis.com" in
  let port = 80 in
  let path =
    match platform with
    | MacOS ->
      "/chrome-for-testing-public/"
      ^ version
      ^ "/mac-x64/chrome-headless-shell-mac-x64.zip"
    | MacOS_ARM ->
      "/chrome-for-testing-public/"
      ^ version
      ^ "/mac-arm64/chrome-headless-shell-mac-arm64.zip"
    | Linux ->
      "/chrome-for-testing-public/"
      ^ version
      ^ "/linux64/chrome-headless-shell-linux64.zip"
    | Win64 ->
      "/chrome-for-testing-public/" ^ version ^ "/win64/chrome-headless-shell-win64.zip"
    | Win32 ->
      print_endline "Error: x86 is currently not supported on Windows";
      exit 1
  in
  host, port, path
;;

let download ~revision dir =
  let zip_path = Eio.Path.(dir / "chromium.zip") in
  Eio.Path.rmtree ~missing_ok:true zip_path;
  let version = OSnap_Browser_Path.revision_to_version revision in
  let host, port, path = get_uri ~version (OSnap_Utils.detect_platform ()) in
  print_endline
    (Printf.sprintf
       "Downloading chromium version %s.\n\
        This is a one time setup and will only happen, if there are updates from OSnap..."
       version);
  Eio.Switch.run
  @@ fun sw ->
  let fd = Unix.socket ~cloexec:true Unix.PF_INET Unix.SOCK_STREAM 0 in
  let addrs =
    Eio_unix.run_in_systhread (fun () ->
      Unix.getaddrinfo host (Int.to_string port) [ Unix.(AI_FAMILY PF_INET) ])
  in
  Eio_unix.run_in_systhread (fun () -> Unix.connect fd (List.hd addrs).ai_addr);
  let socket = Eio_unix.Net.import_socket_stream ~sw ~close_unix:true fd in
  let headers = Httpun.Headers.of_list [ "host", host ] in
  let connection = Httpun_eio.Client.create_connection ~sw socket in
  let p, resolver = Eio.Promise.create () in
  Eio.Fiber.fork ~sw (fun () ->
    let request_body =
      Httpun_eio.Client.request
        connection
        (Httpun.Request.create ~headers `GET path)
        ~error_handler:(fun _ -> ())
        ~response_handler:(fun response body ->
          let buf = Buffer.create (1024 * 1024 * 1024 * 140) in
          match response with
          | { Httpun.Response.status = `OK; _ } ->
            let on_eof () = Eio.Promise.resolve_ok resolver (Buffer.contents buf) in
            let rec on_read bs ~off ~len =
              let chunk = Bigstringaf.substring ~off ~len bs in
              Eio.Path.with_open_out
                ~append:true
                ~create:(`If_missing 0o755)
                zip_path
                (Eio.Flow.copy_string chunk);
              Httpun.Body.Reader.schedule_read body ~on_read ~on_eof
            in
            Httpun.Body.Reader.schedule_read body ~on_read ~on_eof
          | response ->
            Format.fprintf
              Format.err_formatter
              "Chrome could not be downloaded:\n%a\n%!"
              Httpun.Response.pp_hum
              response;
            Eio.Promise.resolve_error resolver `OSnap_Chromium_Download_Failed)
    in
    Httpun.Body.Writer.close request_body);
  let*? _response_body = Eio.Promise.await p in
  Httpun_eio.Client.shutdown connection |> Eio.Promise.await;
  Result.ok zip_path
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
      let temp_dir = Filename.get_temp_dir_name () in
      Eio.Path.with_open_dir
        Eio.Path.(fs / temp_dir)
        (fun dir ->
           let*? path = download dir ~revision in
           extract_zip path ~dest:extract_path;
           Eio.Path.rmtree ~missing_ok:true path;
           Result.ok ()))
  in
  cleanup_old_revisions ();
  Result.ok ()
;;
