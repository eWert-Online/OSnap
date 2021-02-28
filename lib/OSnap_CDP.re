module TargetID = {
  [@deriving yojson]
  type t = string;
};

module SessionID = {
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
    id: int,
    method: string,
    [@yojson.option]
    params: option('a),
    [@yojson.option]
    sessionId: option(SessionID.t),
  };

  let make = (~params=?, ~sessionId=?, method) => {
    let id = id();
    {id, method, params, sessionId};
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
  module ActivateTarget = {
    [@deriving yojson]
    type params = {targetId: TargetID.t};

    [@deriving yojson]
    type request = Request.t(params);

    let make = targetId => {
      Request.make("Target.activateTarget", ~params={targetId: targetId})
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

    let make = () => {
      Request.make("Target.getTargets")
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

    let make = (~flatten=?, targetId) => {
      Request.make("Target.attachToTarget", ~params={targetId, flatten})
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
      print_endline("PARSING");
      response |> Yojson.Safe.from_string |> response_of_yojson;
    };

    let make = (~quality=?, ~format=?, ~sessionId=?, ()) => {
      Request.make(
        "Page.captureScreenshot",
        ~params={
          quality,
          format,
          captureBeyondViewport: Some(false),
          fromSurface: Some(true),
        },
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

    let make = (~referrer=?, ~sessionId=?, url) => {
      Request.make("Page.navigate", ~params={url, referrer}, ~sessionId?)
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
