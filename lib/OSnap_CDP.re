module TargetID = {
  [@deriving yojson]
  type t = string;
};

module SessionID = {
  [@deriving yojson]
  type t = string;
};

module BrowserContextID = {
  [@deriving yojson]
  type t = string;
};

module TargetInfo = {
  [@deriving yojson]
  type t = {
    targetId: TargetID.t,
    [@key "type"]
    type_: string,
    title: string,
    url: string,
    attached: bool,
    canAccessOpener: bool,
    browserContextId: string,
  };
};

module Request = {
  let id = ref(0);

  let id = () => {
    let new_id = id^ + 1;
    id := new_id;
    new_id;
  };

  [@deriving yojson]
  type t('a) = {
    [@yojson.option]
    sessionId: option(SessionID.t),
    method: string,
    [@yojson.option]
    params: option('a),
    id: int,
  };

  let make = (~sessionId=?, ~params=?, method) => {
    let id = id();
    {sessionId, method, params, id};
  };
};

module Response = {
  [@deriving yojson]
  type t('a) = {
    id: int,
    result: 'a,
    [@yojson.option]
    sessionId: option(SessionID.t),
  };
};

module Target = {
  module CreateTarget = {
    [@deriving yojson]
    type params = {
      url: string,
      [@yojson.option]
      width: option(int),
      [@yojson.option]
      height: option(int),
      [@yojson.option]
      browserContextId: option(BrowserContextID.t),
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
    type result = {targetId: TargetID.t};

    [@deriving yojson]
    type response = Response.t(result);

    let parse = response => {
      response |> Yojson.Safe.from_string |> response_of_yojson;
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
    type result = {browserContextId: BrowserContextID.t};

    [@deriving yojson]
    type response = Response.t(result);

    let parse = response => {
      response |> Yojson.Safe.from_string |> response_of_yojson;
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
    type params = {targetId: TargetID.t};

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
    type result = {targetInfos: array(TargetInfo.t)};

    [@deriving yojson]
    type response = Response.t(result);

    let parse = response => {
      response |> Yojson.Safe.from_string |> response_of_yojson;
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
      targetId: TargetID.t,
      [@yojson.option]
      flatten: option(bool),
    };

    [@deriving yojson]
    type request = Request.t(params);

    [@deriving yojson]
    type result = {sessionId: SessionID.t};

    [@deriving yojson]
    type response = Response.t(result);

    let parse = response => {
      response |> Yojson.Safe.from_string |> response_of_yojson;
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

    let parse = response => {
      response |> Yojson.Safe.from_string |> response_of_yojson;
    };

    let make =
        (~sessionId=?, ~flatten=?, ~waitForDebuggerOnStart, ~autoAttach) => {
      Request.make(
        "Target.setAutoAttach",
        ~params={autoAttach, waitForDebuggerOnStart, flatten},
        ~sessionId?,
      )
      |> yojson_of_request
      |> Yojson.Safe.to_string;
    };
  };
};

module Page = {
  module Enable = {
    [@deriving yojson]
    type params;

    [@deriving yojson]
    type request = Request.t(params);

    let make = (~sessionId=?, ()) => {
      Request.make("Page.enable", ~sessionId?)
      |> yojson_of_request
      |> Yojson.Safe.to_string;
    };
  };

  module SetLifecycleEventsEnabled = {
    [@deriving yojson]
    type params = {enabled: bool};

    [@deriving yojson]
    type request = Request.t(params);

    [@deriving yojson]
    type result;

    [@deriving yojson]
    type response = Response.t(result);

    let parse = response => {
      response |> Yojson.Safe.from_string |> response_of_yojson;
    };

    let make = (~sessionId=?, ~enabled) => {
      Request.make(
        "Page.setLifecycleEventsEnabled",
        ~params={enabled: enabled},
        ~sessionId?,
      )
      |> yojson_of_request
      |> Yojson.Safe.to_string;
    };
  };

  module CaptureScreenshot = {
    [@deriving yojson]
    type params = {
      [@yojson.option]
      quality: option(int),
      [@yojson.option]
      format: option(string),
      [@yojson.option]
      captureBeyondViewport: option(bool),
      [@yojson.option]
      fromSurface: option(bool),
    };

    [@deriving yojson]
    type request = Request.t(params);

    [@deriving yojson]
    type result = {data: string};

    [@deriving yojson]
    type response = Response.t(result);

    let parse = response => {
      response |> Yojson.Safe.from_string |> response_of_yojson;
    };

    let make =
        (~quality=?, ~format=?, ~sessionId=?, ~captureBeyondViewport=?, ()) => {
      Request.make(
        "Page.captureScreenshot",
        ~params={quality, format, captureBeyondViewport, fromSurface: None},
        ~sessionId?,
      )
      |> yojson_of_request
      |> Yojson.Safe.to_string;
    };
  };

  module Navigate = {
    [@deriving yojson]
    type params = {
      url: string,
      [@yojson.option]
      referrer: option(string),
    };

    [@deriving yojson]
    type request = Request.t(params);

    [@deriving yojson]
    type result = {
      frameId: string,
      [@yojson.option]
      errorText: option(string),
      [@yojson.option]
      loaderId: option(string),
    };

    [@deriving yojson]
    type response = Response.t(result);

    let parse = response => {
      response |> Yojson.Safe.from_string |> response_of_yojson;
    };

    let make = (~sessionId=?, ~referrer=?, url) => {
      Request.make("Page.navigate", ~params={url, referrer}, ~sessionId?)
      |> yojson_of_request
      |> Yojson.Safe.to_string;
    };
  };
};

module Performance = {
  module Enable = {
    [@deriving yojson]
    type params;

    [@deriving yojson]
    type request = Request.t(params);

    let make = (~sessionId=?, ()) => {
      Request.make("Performance.enable", ~sessionId?)
      |> yojson_of_request
      |> Yojson.Safe.to_string;
    };
  };
};

module Log = {
  module Enable = {
    [@deriving yojson]
    type params;

    [@deriving yojson]
    type request = Request.t(params);

    let make = (~sessionId=?, ()) => {
      Request.make("Log.enable", ~sessionId?)
      |> yojson_of_request
      |> Yojson.Safe.to_string;
    };
  };
};

module Runtime = {
  module Enable = {
    [@deriving yojson]
    type params;

    [@deriving yojson]
    type request = Request.t(params);

    let make = (~sessionId=?, ()) => {
      Request.make("Runtime.enable", ~sessionId?)
      |> yojson_of_request
      |> Yojson.Safe.to_string;
    };
  };
};

module Network = {
  module Enable = {
    [@deriving yojson]
    type params;

    [@deriving yojson]
    type request = Request.t(params);

    let make = (~sessionId=?, ()) => {
      Request.make("Network.enable", ~sessionId?)
      |> yojson_of_request
      |> Yojson.Safe.to_string;
    };
  };
};

module Emulation = {
  module SetDeviceMetricsOverride = {
    [@deriving yojson]
    type params = {
      width: int,
      height: int,
      deviceScaleFactor: int,
      mobile: bool,
      [@yojson.option]
      scale: option(int),
      [@yojson.option]
      screenWidth: option(int),
      [@yojson.option]
      screenHeight: option(int),
    };

    [@deriving yojson]
    type request = Request.t(params);

    [@deriving yojson]
    type result;

    [@deriving yojson]
    type response = Response.t(result);

    let parse = response => {
      response |> Yojson.Safe.from_string |> response_of_yojson;
    };

    let make =
        (
          ~sessionId=?,
          ~scale=?,
          ~screenWidth=?,
          ~screenHeight=?,
          ~width,
          ~height,
          ~deviceScaleFactor,
          ~mobile,
          (),
        ) => {
      Request.make(
        "Emulation.setDeviceMetricsOverride",
        ~params={
          width,
          height,
          deviceScaleFactor,
          mobile,
          scale,
          screenWidth,
          screenHeight,
        },
        ~sessionId?,
      )
      |> yojson_of_request
      |> Yojson.Safe.to_string;
    };
  };
};
