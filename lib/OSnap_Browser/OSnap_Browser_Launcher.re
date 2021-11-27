open OSnap_Browser_Types;
module Logger = OSnap_Logger;
open Lwt_result.Syntax;

module Websocket = OSnap_Websocket;

let debug = Logger.debug(~header="BROWSER");

let make = () => {
  let base_path = OSnap_Browser_Path.get_chromium_path();

  let executable_path =
    switch (OSnap_Utils.detect_platform()) {
    | Darwin =>
      Filename.concat(
        base_path,
        "chrome-mac/Chromium.app/Contents/MacOS/Chromium",
      )
    | Linux => Filename.concat(base_path, "chrome-linux/chrome")
    | Win64 => Filename.concat(base_path, "chrome-win/chrome.exe")
    | Win32 => ""
    };

  debug(Printf.sprintf("Launching browser from %S", executable_path));

  let process =
    Lwt_process.open_process_full((
      "",
      [|
        executable_path,
        "about:blank",
        "--headless",
        "--no-sandbox",
        "--hide-scrollbars",
        "--remote-debugging-port=0",
        "--mute-audio",
        "--disable-gpu",
        "--disable-background-networking",
        "--enable-features=NetworkService,NetworkServiceInProcess",
        "--disable-background-timer-throttling",
        "--disable-backgrounding-occluded-windows",
        "--disable-breakpad",
        "--disable-client-side-phishing-detection",
        "--disable-component-extensions-with-background-pages",
        "--disable-default-apps",
        "--disable-dev-shm-usage",
        "--disable-extensions",
        "--disable-features=Translate",
        "--disable-hang-monitor",
        "--disable-ipc-flooding-protection",
        "--disable-popup-blocking",
        "--disable-prompt-on-repost",
        "--disable-renderer-backgrounding",
        "--disable-sync",
        "--force-color-profile=srgb",
        "--metrics-recording-only",
        "--no-first-run",
        "--enable-automation",
        "--password-store=basic",
        "--use-mock-keychain",
        "--enable-blink-features=IdleDetection",
      |],
    ));

  let rec get_ws_url = proc => {
    switch (proc#state) {
    | Lwt_process.Running =>
      Lwt.bind(
        Lwt_io.read_line(proc#stderr),
        line => {
          debug(Printf.sprintf("STDERR: %S", line));
          switch (line) {
          | line
              when
                line
                |> OSnap_Utils.contains_substring(
                     ~search="Cannot start http server",
                   ) =>
            proc#terminate;
            Lwt_result.fail(OSnap_Response.CDP_Connection_Failed);
          | line
              when
                line
                |> OSnap_Utils.contains_substring(
                     ~search="DevTools listening on",
                   ) =>
            let offset = String.length("DevTools listening on ");
            let len = String.length(line);
            let socket = String.sub(line, offset, len - offset);
            socket |> Lwt_result.return;
          | _ => get_ws_url(proc)
          };
        },
      )
    | Lwt_process.Exited(_) =>
      Lwt_result.fail(OSnap_Response.CDP_Connection_Failed)
    };
  };

  let* url = get_ws_url(process);

  debug(Printf.sprintf("Connecting to: %S", url));
  let _ = Websocket.connect(url);

  debug(Printf.sprintf("Connected!"));

  let* result =
    Cdp.Commands.Target.CreateBrowserContext.(
      Request.make(~sessionId=?None, ~params=Params.make())
      |> Websocket.send
      |> Lwt.map(Response.parse)
      |> Lwt.map(response => {
           let error =
             response.Response.error
             |> Option.map((error: Response.error) =>
                  OSnap_Response.CDP_Protocol_Error(error.message)
                )
             |> Option.value(~default=OSnap_Response.CDP_Connection_Failed);

           Option.to_result(response.Response.result, ~none=error);
         })
    );

  Lwt_result.return({
    ws: url,
    process,
    browserContextId: result.browserContextId,
  });
};

let shutdown = browser => (browser.process)#terminate;
