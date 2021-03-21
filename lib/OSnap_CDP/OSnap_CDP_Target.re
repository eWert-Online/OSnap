open OSnap_CDP_Types;

module CreateTarget = {
  [@deriving yojson]
  type params = {
    url: string,
    [@yojson.option]
    width: option(int),
    [@yojson.option]
    height: option(int),
    [@yojson.option]
    browserContextId: option(Browser.BrowserContextID.t),
    [@yojson.option]
    enableBeginFrameControl: option(bool),
    [@yojson.option]
    newWindow: option(bool),
    [@yojson.option]
    background: option(bool),
  };

  [@deriving yojson]
  type request = Request.t(params);

  [@deriving yojson]
  type result = {targetId: Target.TargetId.t};

  [@deriving yojson]
  type response = Response.t(result);

  let parse = response =>
    try(response |> Yojson.Safe.from_string |> response_of_yojson) {
    | _ as exn =>
      print_endline("Error parsing CreateTarget!");
      raise(exn);
    };

  let make =
      (
        ~width=?,
        ~height=?,
        ~browserContextId=?,
        ~enableBeginFrameControl=?,
        ~newWindow=?,
        ~background=?,
        url,
      ) => {
    Request.make(
      "Target.createTarget",
      ~params={
        url,
        width,
        height,
        browserContextId,
        enableBeginFrameControl,
        newWindow,
        background,
      },
    )
    |> yojson_of_request
    |> Yojson.Safe.to_string;
  };
};

module CreateBrowserContext = {
  [@deriving yojson]
  type params = {
    [@yojson.option]
    disposeOnDetach: option(bool),
    [@yojson.option]
    proxyServer: option(string),
    [@yojson.option]
    proxyBypassList: option(string),
  };

  [@deriving yojson]
  type request = Request.t(params);

  [@deriving yojson]
  type result = {browserContextId: Browser.BrowserContextID.t};

  [@deriving yojson]
  type response = Response.t(result);

  let parse = response =>
    try(response |> Yojson.Safe.from_string |> response_of_yojson) {
    | _ as exn =>
      print_endline("Error parsing CreateBrowserContext!");
      raise(exn);
    };

  let make =
      (
        ~sessionId=?,
        ~disposeOnDetach=?,
        ~proxyServer=?,
        ~proxyBypassList=?,
        (),
      ) => {
    Request.make(
      "Target.createBrowserContext",
      ~params={disposeOnDetach, proxyServer, proxyBypassList},
      ~sessionId?,
    )
    |> yojson_of_request
    |> Yojson.Safe.to_string;
  };
};

module ActivateTarget = {
  [@deriving yojson]
  type params = {targetId: Target.TargetId.t};

  [@deriving yojson]
  type request = Request.t(params);

  let make = (~sessionId=?, targetId) => {
    Request.make(
      "Target.activateTarget",
      ~params={targetId: targetId},
      ~sessionId?,
    )
    |> yojson_of_request
    |> Yojson.Safe.to_string;
  };
};

module GetTargets = {
  [@deriving yojson]
  type params;

  [@deriving yojson]
  type request = Request.t(params);

  [@deriving yojson]
  type result = {targetInfos: array(Target.TargetInfo.t)};

  [@deriving yojson]
  type response = Response.t(result);

  let parse = response =>
    try(response |> Yojson.Safe.from_string |> response_of_yojson) {
    | _ as exn =>
      print_endline("Error parsing GetTargets!");
      raise(exn);
    };

  let make = (~sessionId=?, ()) => {
    Request.make("Target.getTargets", ~sessionId?)
    |> yojson_of_request
    |> Yojson.Safe.to_string;
  };
};

module AttachToTarget = {
  [@deriving yojson]
  type params = {
    targetId: Target.TargetId.t,
    [@yojson.option]
    flatten: option(bool),
  };

  [@deriving yojson]
  type request = Request.t(params);

  [@deriving yojson]
  type result = {sessionId: Target.SessionId.t};

  [@deriving yojson]
  type response = Response.t(result);

  let parse = response =>
    try(response |> Yojson.Safe.from_string |> response_of_yojson) {
    | _ as exn =>
      print_endline("Error parsing AttachToTarget!");
      raise(exn);
    };

  let make = (~sessionId=?, ~flatten=?, targetId) => {
    Request.make(
      "Target.attachToTarget",
      ~params={targetId, flatten},
      ~sessionId?,
    )
    |> yojson_of_request
    |> Yojson.Safe.to_string;
  };
};

module SetAutoAttach = {
  [@deriving yojson]
  type params = {
    autoAttach: bool,
    waitForDebuggerOnStart: bool,
    [@yojson.option]
    flatten: option(bool),
  };

  [@deriving yojson]
  type request = Request.t(params);

  [@deriving yojson]
  type result;

  [@deriving yojson]
  type response = Response.t(result);

  let parse = response =>
    try(response |> Yojson.Safe.from_string |> response_of_yojson) {
    | _ as exn =>
      print_endline("Error parsing SetAutoAttach!");
      raise(exn);
    };

  let make = (~sessionId=?, ~flatten=?, ~waitForDebuggerOnStart, ~autoAttach) => {
    Request.make(
      "Target.setAutoAttach",
      ~params={autoAttach, waitForDebuggerOnStart, flatten},
      ~sessionId?,
    )
    |> yojson_of_request
    |> Yojson.Safe.to_string;
  };
};
