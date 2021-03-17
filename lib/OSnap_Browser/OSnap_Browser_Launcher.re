open OSnap_Browser_Types;

module CDP = OSnap_CDP;
module Websocket = OSnap_Websocket;

exception Connection_failed;

let enable_events = sessionId => {
  let%lwt _ = CDP.Page.Enable.make(~sessionId, ()) |> Websocket.send;
  let%lwt _ =
    CDP.Page.SetLifecycleEventsEnabled.make(~sessionId, ~enabled=true)
    |> Websocket.send;
  // let%lwt _ = Performance.Enable.make(~sessionId, ()) |> Websocket.send;
  // let%lwt _ = Log.Enable.make(~sessionId, ()) |> Websocket.send;
  // let%lwt _ = Runtime.Enable.make(~sessionId, ()) |> Websocket.send;
  // let%lwt _ = Network.Enable.make(~sessionId, ()) |> Websocket.send;
  Lwt.return();
};

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

  let%lwt targetId =
    CDP.Target.CreateTarget.make(~browserContextId, "about:blank")
    |> Websocket.send
    |> Lwt.map(CDP.Target.CreateTarget.parse)
    |> Lwt.map(response =>
         response.CDP.Types.Response.result.CDP.Target.CreateTarget.targetId
       );

  let%lwt sessionId =
    CDP.Target.AttachToTarget.make(targetId, ~flatten=true)
    |> Websocket.send
    |> Lwt.map(CDP.Target.AttachToTarget.parse)
    |> Lwt.map(response =>
         response.CDP.Types.Response.result.CDP.Target.AttachToTarget.sessionId
       );

  let%lwt () = enable_events(sessionId);

  Lwt.return({
    ws: url,
    process,
    targets: [(targetId, sessionId)],
    browserContextId,
  });
};

let create_targets = (count, browser) => {
  let%lwt new_targets =
    List.init(count, _ => {
      CDP.Target.CreateTarget.make(
        ~browserContextId=browser.browserContextId,
        "about:blank",
      )
    })
    |> Lwt_list.map_p(create_target => {
         let%lwt targetId =
           create_target
           |> Websocket.send
           |> Lwt.map(CDP.Target.CreateTarget.parse)
           |> Lwt.map(response =>
                response.CDP.Types.Response.result.CDP.Target.CreateTarget.targetId
              );
         let%lwt sessionId =
           CDP.Target.AttachToTarget.make(targetId, ~flatten=true)
           |> Websocket.send
           |> Lwt.map(CDP.Target.AttachToTarget.parse)
           |> Lwt.map(response =>
                response.CDP.Types.Response.result.CDP.Target.AttachToTarget.sessionId
              );

         let%lwt () = enable_events(sessionId);

         Lwt.return((targetId, sessionId));
       });

  Lwt.return({...browser, targets: browser.targets @ new_targets});
};

let get_targets = browser => browser.targets;

let shutdown = browser => (browser.process)#terminate;
