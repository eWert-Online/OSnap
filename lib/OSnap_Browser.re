open OSnap_CDP;

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
//     | "darwin" => "https://storage.googleapis.com/chromium-browser-snapshots/Mac/856098/chrome-mac.zip"
//     | "linux" => "https://storage.googleapis.com/chromium-browser-snapshots/Linux_x64/856098/chrome-linux.zip"
//     | "win64" => "https://storage.googleapis.com/chromium-browser-snapshots/Win_x64/856098/chrome-win.zip"
//     | _ => ""
//     };
//   };
// };

type action =
  | Click(string)
  | Type(string, string);

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
  let assets_path = Unix.getcwd() ++ "/assets/";
  let executable_path =
    switch (detect_platform()) {
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
        "--headless",
        "--hide-scrollbars",
        "--remote-debugging-port=0",
        // "--disable-gpu",
      |],
    ));

  let rec get_ws_url = proc => {
    switch (proc#state) {
    | Lwt_process.Running =>
      let%lwt line = proc#stderr |> Lwt_io.read_line;
      print_endline("[CHROME] " ++ line);
      if (contains_substring("Cannot start http server for devtools", line)) {
        proc#terminate;
      };
      if (line |> contains_substring("DevTools listening on")) {
        let offset = String.length("DevTools listening on");
        let len = String.length(line);
        let socket = String.sub(line, offset, len - offset);
        Lwt_result.return(socket);
      } else {
        get_ws_url(proc);
      };
    | Lwt_process.Exited(status) => Lwt_result.fail(status)
    };
  };

  let%lwt url = get_ws_url(process);

  switch (url) {
  | Error(e) => Lwt_result.fail(e)
  | Ok(url) =>
    let _ = OSnap_Websocket.connect(url);
    let%lwt targets = Target.GetTargets.make() |> OSnap_Websocket.send;
    let target = targets |> Target.GetTargets.parse;
    let targetId =
      target.Response.result.Target.GetTargets.targetInfos[0].targetId;

    let%lwt response =
      Target.AttachToTarget.make(targetId, ~flatten=true)
      |> OSnap_Websocket.send
      |> Lwt.map(Target.AttachToTarget.parse);
    let sessionId = response.Response.result.Target.AttachToTarget.sessionId;

    let%lwt _ = Page.Enable.make(~sessionId, ()) |> OSnap_Websocket.send;

    Lwt_result.return({ws: url, process, targetId, sessionId});
  };
};

let go_to = (url, browser) => {
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

// let set_size: (~width: int, ~height: int, t) => Lwt_result.t(t, string) =
//   (~width as _a, ~height as _b, _c) => {
//     Lwt_result.return(Obj.magic());
//   };

let screenshot = (~full_size as _=false, browser) => {
  let%lwt _ =
    Emulation.SetDeviceMetricsOverride.make(
      ~sessionId=browser.sessionId,
      ~width=200,
      ~height=200,
      ~scale=1,
      ~deviceScaleFactor=0,
      ~mobile=false,
      (),
    )
    |> OSnap_Websocket.send;

  let%lwt _ =
    Target.ActivateTarget.make(browser.targetId) |> OSnap_Websocket.send;

  let%lwt response =
    Page.CaptureScreenshot.make(
      ~format="jpeg",
      ~quality=1,
      ~sessionId=browser.sessionId,
      (),
    )
    |> OSnap_Websocket.send
    |> Lwt.map(Page.CaptureScreenshot.parse);

  response.Response.result.Page.CaptureScreenshot.data |> Lwt.return;
};
