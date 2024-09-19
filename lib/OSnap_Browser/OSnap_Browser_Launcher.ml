module Websocket = OSnap_Websocket
open OSnap_Browser_Types

let make ~sw ~env () =
  let ( let*? ) = Result.bind in
  let latest_chromium_revision = OSnap_Browser_Path.get_latest_revision () in
  let*? () =
    if not (OSnap_Browser_Download.is_revision_downloaded latest_chromium_revision)
    then OSnap_Browser_Download.download ~env latest_chromium_revision
    else Result.ok ()
  in
  let base_path = OSnap_Browser_Path.get_chromium_path latest_chromium_revision in
  let executable =
    match OSnap_Utils.detect_platform () with
    | MacOS | MacOS_ARM ->
      Filename.concat base_path "chrome-mac/Chromium.app/Contents/MacOS/Chromium"
    | Linux -> Filename.concat base_path "chrome-linux/chrome"
    | Win64 -> Filename.concat base_path "chrome-win/chrome.exe"
    | Win32 -> ""
  in
  let process_manager = Eio.Stdenv.process_mgr env in
  let read_stderr, write_stderr = Eio.Process.pipe process_manager ~sw in
  let process =
    Eio.Process.spawn
      ~stderr:write_stderr
      ~sw
      ~executable
      process_manager
      [ executable
      ; "about:blank"
      ; "--headless=old"
      ; "--disable-gpu"
      ; "--no-sandbox"
      ; "--hide-scrollbars"
      ; "--remote-debugging-port=0"
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
  let rec get_ws_url ~from proc =
    let line = from |> Eio.Buf_read.line in
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
  let*? () = Websocket.connect ~sw ~env url in
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
