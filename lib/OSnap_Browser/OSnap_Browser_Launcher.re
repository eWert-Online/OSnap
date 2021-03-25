open OSnap_Browser_Types;

module CDP = OSnap_CDP;
module Websocket = OSnap_Websocket;

exception Connection_failed;

let make = () => {
  let assets_path = Sys.getcwd() ++ "/assets/";
  let executable_path =
    switch (OSnap_Browser_Utils.detect_platform()) {
    | "darwin" =>
      assets_path ++ "chrome-mac/Chromium.app/Contents/MacOS/Chromium"
    | "linux" => assets_path ++ "chrome-linux/chrome"
    | "win64" => assets_path ++ "chrome-win/chrome.exe"
    | _ => ""
    };

  let process =
    Lwt_process.open_process_full((
      "",
      [|
        executable_path,
        "about:blank",
        "--headless",
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
      let%lwt line = proc#stderr |> Lwt_io.read_line;
      if (OSnap_Browser_Utils.contains_substring(
            "Cannot start http server for devtools",
            line,
          )) {
        proc#terminate;
      };
      if (line
          |> OSnap_Browser_Utils.contains_substring("DevTools listening on")) {
        let offset = String.length("DevTools listening on");
        let len = String.length(line);
        let socket = String.sub(line, offset, len - offset);
        socket |> Lwt.return;
      } else {
        get_ws_url(proc);
      };
    | Lwt_process.Exited(_) => raise(Connection_failed)
    };
  };

  let%lwt url = get_ws_url(process);

  let _ = Websocket.connect(url);

  let%lwt browserContextId =
    CDP.Target.CreateBrowserContext.make()
    |> Websocket.send
    |> Lwt.map(CDP.Target.CreateBrowserContext.parse)
    |> Lwt.map(response => {
         response.CDP.Types.Response.result.CDP.Target.CreateBrowserContext.browserContextId
       });

  Lwt.return({ws: url, process, browserContextId});
};

let shutdown = browser => (browser.process)#terminate;
