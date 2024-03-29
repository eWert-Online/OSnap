module Websocket = OSnap_Websocket
open OSnap_Browser_Types
open OSnap_Utils

let make () =
  let latest_chromium_revision = OSnap_Browser_Path.get_latest_revision () in
  let*? () =
    if not (OSnap_Browser_Download.is_revision_downloaded latest_chromium_revision)
    then OSnap_Browser_Download.download latest_chromium_revision
    else Lwt_result.return ()
  in
  let base_path = OSnap_Browser_Path.get_chromium_path latest_chromium_revision in
  let executable_path =
    match OSnap_Utils.detect_platform () with
    | MacOS | MacOS_ARM ->
      Filename.concat base_path "chrome-mac/Chromium.app/Contents/MacOS/Chromium"
    | Linux -> Filename.concat base_path "chrome-linux/chrome"
    | Win64 -> Filename.concat base_path "chrome-win/chrome.exe"
    | Win32 -> ""
  in
  let process =
    Lwt_process.open_process_full
      ( ""
      , [| executable_path
         ; "about:blank"
         ; "--headless"
         ; "--no-sandbox"
         ; "--hide-scrollbars"
         ; "--remote-debugging-port=0"
         ; "--mute-audio"
         ; "--disable-gpu"
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
         ; "--enable-blink-features=IdleDetection"
        |] )
  in
  let rec get_ws_url proc =
    match proc#state with
    | Lwt_process.Running ->
      Lwt.bind (Lwt_io.read_line proc#stderr) (fun line ->
        match line with
        | line
          when line |> OSnap_Utils.contains_substring ~search:"Cannot start http server"
          ->
          proc#terminate;
          Lwt_result.fail `OSnap_CDP_Connection_Failed
        | line when line |> OSnap_Utils.contains_substring ~search:"DevTools listening on"
          ->
          let offset = String.length "DevTools listening on " in
          let len = String.length line in
          let socket = String.sub line offset (len - offset) in
          socket |> Lwt_result.return
        | _ -> get_ws_url proc)
    | Lwt_process.Exited _ -> Lwt_result.fail `OSnap_CDP_Connection_Failed
  in
  let*? url = get_ws_url process in
  let _ = Websocket.connect url in
  let*? result =
    let open Cdp.Commands.Target.CreateBrowserContext in
    Request.make ?sessionId:None ~params:(Params.make ())
    |> Websocket.send
    |> Lwt.map Response.parse
    |> Lwt.map (fun response ->
         let error =
           response.Response.error
           |> Option.map (fun (error : Response.error) ->
                `OSnap_CDP_Protocol_Error error.message)
           |> Option.value ~default:`OSnap_CDP_Connection_Failed
         in
         Option.to_result response.Response.result ~none:error)
  in
  Lwt_result.return { ws = url; process; browserContextId = result.browserContextId }
;;

let shutdown browser = browser.process#terminate
