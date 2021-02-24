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

module Download = {
  let get_url = () => {
    switch (detect_platform()) {
    | "darwin" => "https://storage.googleapis.com/chromium-browser-snapshots/Mac/856098/chrome-mac.zip"
    | "linux" => "https://storage.googleapis.com/chromium-browser-snapshots/Linux_x64/856098/chrome-linux.zip"
    | "win64" => "https://storage.googleapis.com/chromium-browser-snapshots/Win_x64/856098/chrome-win.zip"
    | _ => ""
    };
  };
};

type action =
  | Click(string)
  | Type(string, string);

type t = {
  ws: string,
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
    | "linux" =>
      assets_path ++ "chrome-mac/Chromium.app/Contents/MacOS/Chromium"
    | "win64" =>
      assets_path ++ "chrome-mac/Chromium.app/Contents/MacOS/Chromium"
    | _ => ""
    };

  let process =
    Lwt_process.open_process_full((
      "",
      [|
        executable_path,
        "--headless",
        "--hide-scrollbars",
        "--remote-debugging-port=9222",
        "--disable-gpu",
      |],
    ));

  let rec read_output = proc => {
    switch (proc#state) {
    | Lwt_process.Running =>
      proc#stderr
      |> Lwt_io.read_line
      |> Lwt.bind(_, s =>
           if (s |> contains_substring("DevTools listening on")) {
             let offset = String.length("DevTools listening on");
             let len = String.length(s);
             let socket = String.sub(s, offset, len - offset);
             Lwt_result.return({ws: socket, process: proc});
           } else {
             read_output(proc);
           }
         )
    | Lwt_process.Exited(status) => Lwt_result.fail(status)
    };
  };

  read_output(process);
};

let go_to: (string, t) => Lwt_result.t(t, string) =
  (_a, _b) => {
    Lwt_result.return(Obj.magic());
  };

let run_action: (action, t) => Lwt_result.t(t, string) =
  (_a, _b) => {
    Lwt_result.return(Obj.magic());
  };

let set_size: (~width: int, ~height: int, t) => Lwt_result.t(t, string) =
  (~width as _a, ~height as _b, _c) => {
    Lwt_result.return(Obj.magic());
  };

let screenshot: (~full_size: bool, t) => Lwt_result.t(string, string) =
  (~full_size as _a, _b) => {
    Lwt_result.return(Obj.magic());
  };
