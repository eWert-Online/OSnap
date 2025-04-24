module Logger = OSnap_Logger
module Websocket = OSnap_Websocket
open OSnap_Browser_Types

let debug = Logger.info ~header:"BROWSER"

let make ~sw ~env () =
  let ( let*? ) = Result.bind in
  let latest_chromium_revision = OSnap_Browser_Path.get_latest_revision () in
  debug
    (Printf.sprintf
       "Found browser version %s to launch"
       (OSnap_Browser_Path.revision_to_version latest_chromium_revision));
  let*? () =
    if not (OSnap_Browser_Download.is_revision_downloaded latest_chromium_revision)
    then (
      debug "Browser is not downloaded";
      OSnap_Browser_Download.download ~env latest_chromium_revision)
    else (
      debug "Browser is already downloaded";
      Result.ok ())
  in
  let base_path = OSnap_Browser_Path.get_chromium_path latest_chromium_revision in
  let executable =
    match OSnap_Utils.detect_platform () with
    | MacOS ->
      Filename.concat base_path "chrome-headless-shell-mac-x64/chrome-headless-shell"
    | MacOS_ARM ->
      Filename.concat base_path "chrome-headless-shell-mac-arm64/chrome-headless-shell"
    | Linux ->
      Filename.concat base_path "chrome-headless-shell-linux64/chrome-headless-shell"
    | Win64 ->
      Filename.concat base_path "chrome-headless-shell-win64/chrome-headless-shell.exe"
    | Win32 -> ""
  in
  let process_manager = Eio.Stdenv.process_mgr env in
  let read_stderr, write_stderr = Eio.Process.pipe process_manager ~sw in
  debug (Printf.sprintf "Spawning browser process from %S" executable);
  let process =
    Eio.Process.spawn
      ~stderr:write_stderr
      ~sw
      ~executable
      process_manager
      [ executable
      ; "about:blank"
      ; "--remote-debugging-address=0.0.0.0"
      ; "--remote-debugging-port=0"
      ; "--disable-gpu"
      ; "--no-sandbox"
      ; "--hide-scrollbars"
      ; "--mute-audio"
      ; "--disable-background-networking"
      ; "--enable-features=NetworkService,NetworkServiceInProcess"
      ; "--disable-background-timer-throttling"
      ; "--disable-backgrounding-occluded-windows"
      ; "--disable-breakpad"
      ; "--disable-client-side-phishing-detection"
      ; "--disable-component-extensions-with-background-pages"
      ; "--disable-default-apps"
      ; "--disable-dev-shm-usage"
      ; "--disable-extensions"
      ; "--disable-features=Translate"
      ; "--disable-hang-monitor"
      ; "--disable-ipc-flooding-protection"
      ; "--disable-popup-blocking"
      ; "--disable-prompt-on-repost"
      ; "--disable-renderer-backgrounding"
      ; "--disable-sync"
      ; "--force-color-profile=srgb"
      ; "--metrics-recording-only"
      ; "--no-first-run"
      ; "--enable-automation"
      ; "--password-store=basic"
      ; "--use-mock-keychain"
      ]
  in
  debug "Launching Browser was successful. Trying to establish a connection";
  let rec get_ws_url ~from proc =
    let line = from |> Eio.Buf_read.line in
    debug (Printf.sprintf "CHROME: %S" line);
    match line with
    | line when line |> OSnap_Utils.contains_substring ~search:"Cannot start http server"
      ->
      Eio.Process.signal proc Sys.sigkill;
      Result.error `OSnap_CDP_Connection_Failed
    | line when line |> OSnap_Utils.contains_substring ~search:"DevTools listening on" ->
      let offset = String.length "DevTools listening on " in
      let len = String.length line in
      let socket = String.sub line offset (len - offset) in
      Result.ok socket
    | _ -> get_ws_url ~from proc
  in
  let stderr_buffer = Eio.Buf_read.of_flow read_stderr ~max_size:max_int in
  let*? url = get_ws_url ~from:stderr_buffer process in
  debug (Printf.sprintf "Connecting to: %S" url);
  let*? () = Websocket.connect ~sw ~env url in
  debug (Printf.sprintf "Connected!");
  let*? result =
    let open Cdp.Commands.Target.CreateBrowserContext in
    Request.make ?sessionId:None ~params:(Params.make ())
    |> Websocket.send
    |> Response.parse
    |> fun response ->
    let error =
      response.Response.error
      |> Option.map (fun (error : Response.error) ->
        `OSnap_CDP_Protocol_Error error.message)
      |> Option.value ~default:`OSnap_CDP_Connection_Failed
    in
    Option.to_result response.Response.result ~none:error
  in
  Result.ok { ws = url; process; browserContextId = result.browserContextId }
;;
