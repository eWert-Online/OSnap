open OSnap_Browser_Types;

module CDP = OSnap_CDP;
module Websocket = OSnap_Websocket;

exception Connection_failed;

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
      switch%lwt (Lwt_io.read_line(proc#stderr)) {
      | exception _ =>
        proc#terminate;
        raise(Connection_failed);
      | line
          when
            line
            |> OSnap_Utils.contains_substring(
                 ~search="Cannot start http server",
               ) =>
        proc#terminate;
        raise(Connection_failed);
      | line
          when
            line
            |> OSnap_Utils.contains_substring(~search="DevTools listening on") =>
        let offset = String.length("DevTools listening on");
        let len = String.length(line);
        let socket = String.sub(line, offset, len - offset);
        socket |> Lwt.return;
      | _ => get_ws_url(proc)
      }
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
