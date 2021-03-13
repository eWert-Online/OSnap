open OSnap_CDP;

module Utils = {
  let detect_platform = () => {
    let win = Sys.win32 || Sys.cygwin;
    let ic = Unix.open_process_in("uname");
    let uname = input_line(ic);
    let () = close_in(ic);

    switch (win, uname) {
    | (true, _) =>
      switch (Sys.word_size) {
      | 64 => "win64"
      | _ => "win32"
      }
    | (_, "Darwin") => "darwin"
    | _ => "linux"
    };
  };

  let contains_substring = (search, str) => {
    let re = Str.regexp_string(search);
    try(Str.search_forward(re, str, 0) >= 0) {
    | Not_found => false
    };
  };
  // module Download = {
  //   let get_url = () => {
  //     switch (detect_platform()) {
  //     | "darwin" => "https://storage.googleapis.com/chromium-browser-snapshots/Mac/856583/chrome-mac.zip"
  //     | "linux" => "https://storage.googleapis.com/chromium-browser-snapshots/Linux_x64/856583/chrome-linux.zip"
  //     | "win64" => "https://storage.googleapis.com/chromium-browser-snapshots/Win_x64/856583/chrome-win.zip"
  //     | _ => ""
  //     };
  //   };
  // };
};

exception Connection_failed;

type t = {
  ws: string,
  targetId: TargetID.t,
  sessionId: SessionID.t,
  process: {
    .
    close: Lwt.t(Unix.process_status),
    kill: int => unit,
    pid: int,
    rusage: Lwt.t(Lwt_unix.resource_usage),
    state: Lwt_process.state,
    status: Lwt.t(Unix.process_status),
    stderr: Lwt_io.channel(Lwt_io.input),
    stdin: Lwt_io.channel(Lwt_io.output),
    stdout: Lwt_io.channel(Lwt_io.input),
    terminate: unit,
  },
};

let make = () => {
  let assets_path = Sys.getcwd() ++ "/assets/";
  let executable_path =
    switch (Utils.detect_platform()) {
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
      print_endline("[CHROME] " ++ line);
      if (Utils.contains_substring(
            "Cannot start http server for devtools",
            line,
          )) {
        proc#terminate;
      };
      if (line |> Utils.contains_substring("DevTools listening on")) {
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

  let _ = OSnap_Websocket.connect(url);

  let%lwt browserContextId =
    Target.CreateBrowserContext.make()
    |> OSnap_Websocket.send
    |> Lwt.map(Target.CreateBrowserContext.parse)
    |> Lwt.map(response => {
         response.Response.result.Target.CreateBrowserContext.browserContextId
       });

  let%lwt targetId =
    Target.CreateTarget.make(~browserContextId, "about:blank")
    |> OSnap_Websocket.send
    |> Lwt.map(Target.CreateTarget.parse)
    |> Lwt.map(response =>
         response.Response.result.Target.CreateTarget.targetId
       );

  let%lwt sessionId =
    Target.AttachToTarget.make(targetId, ~flatten=true)
    |> OSnap_Websocket.send
    |> Lwt.map(Target.AttachToTarget.parse)
    |> Lwt.map(response =>
         response.Response.result.Target.AttachToTarget.sessionId
       );

  let%lwt _ = Page.Enable.make(~sessionId, ()) |> OSnap_Websocket.send;
  let%lwt _ =
    Page.SetLifecycleEventsEnabled.make(~sessionId, ~enabled=true)
    |> OSnap_Websocket.send;
  let%lwt _ =
    Target.SetAutoAttach.make(
      ~sessionId,
      ~flatten=true,
      ~waitForDebuggerOnStart=false,
      ~autoAttach=true,
    )
    |> OSnap_Websocket.send;
  // let%lwt _ = Performance.Enable.make(~sessionId, ()) |> OSnap_Websocket.send;
  // let%lwt _ = Log.Enable.make(~sessionId, ()) |> OSnap_Websocket.send;
  // let%lwt _ = Runtime.Enable.make(~sessionId, ()) |> OSnap_Websocket.send;
  // let%lwt _ = Network.Enable.make(~sessionId, ()) |> OSnap_Websocket.send;

  Lwt.return({ws: url, process, targetId, sessionId});
};

let wait_for = event => {
  let (p, resolver) = Lwt.wait();
  let callback = () => {
    Lwt.wakeup_later(resolver, ());
  };
  OSnap_Websocket.listen(event, callback);
  p;
};

let go_to = (url, browser) => {
  print_endline("[BROWSER]\t Going to: " ++ url);
  let%lwt payload =
    Page.Navigate.make(url, ~sessionId=browser.sessionId)
    |> OSnap_Websocket.send
    |> Lwt.map(Page.Navigate.parse);
  payload.Response.result.Page.Navigate.frameId |> Lwt.return;
};

// let run_action: (action, t) => Lwt_result.t(t, string) =
//   (_a, _b) => {
//     Lwt_result.return(Obj.magic());
//   };

let set_size = (~width, ~height, browser) => {
  print_endline(
    "[BROWSER]\t Set size to: "
    ++ string_of_int(width)
    ++ "x"
    ++ string_of_int(height),
  );
  let%lwt _ =
    Emulation.SetDeviceMetricsOverride.make(
      ~sessionId=browser.sessionId,
      ~width,
      ~height,
      ~scale=1,
      ~deviceScaleFactor=0,
      ~mobile=false,
      (),
    )
    |> OSnap_Websocket.send;

  Lwt.return();
};

let screenshot = (~full_size as _=false, browser) => {
  print_endline("[BROWSER]\t Making screenshot!");
  let%lwt _ =
    Target.ActivateTarget.make(browser.targetId) |> OSnap_Websocket.send;

  let%lwt response =
    Page.CaptureScreenshot.make(
      ~format="png",
      ~sessionId=browser.sessionId,
      ~captureBeyondViewport=true,
      (),
    )
    |> OSnap_Websocket.send
    |> Lwt.map(Page.CaptureScreenshot.parse);

  response.Response.result.Page.CaptureScreenshot.data |> Lwt.return;
};
