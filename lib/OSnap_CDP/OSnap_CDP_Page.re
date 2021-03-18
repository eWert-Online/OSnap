open OSnap_CDP_Types;

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

  let parse = response =>
    try(response |> Yojson.Safe.from_string |> response_of_yojson) {
    | _ as exn =>
      print_endline("Error parsing SetLifecycleEventsEnabled!");
      raise(exn);
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
    format: option(string),
    [@yojson.option]
    quality: option(int),
    [@yojson.option]
    clip: option(Page.Viewport.t),
    [@yojson.option]
    fromSurface: option(bool),
    [@yojson.option]
    captureBeyondViewport: option(bool),
  };

  [@deriving yojson]
  type request = Request.t(params);

  [@deriving yojson]
  type result = {data: string};

  [@deriving yojson]
  type response = Response.t(result);

  let parse = response =>
    try(response |> Yojson.Safe.from_string |> response_of_yojson) {
    | _ as exn =>
      print_endline("Error parsing CaptureScreenshot! Response was:");
      print_endline(response);
      raise(exn);
    };

  let make =
      (
        ~sessionId=?,
        ~format=?,
        ~quality=?,
        ~clip=?,
        ~captureBeyondViewport=?,
        ~fromSurface=?,
        (),
      ) => {
    Request.make(
      "Page.captureScreenshot",
      ~params={format, quality, clip, captureBeyondViewport, fromSurface},
      ~sessionId?,
    )
    |> yojson_of_request
    |> Yojson.Safe.to_string;
  };
};

module GetLayoutMetrics = {
  [@deriving yojson]
  type params;

  [@deriving yojson]
  type request = Request.t(params);

  [@deriving yojson]
  type result = {
    layoutViewport: Page.LayoutViewport.t,
    visualViewport: Page.VisualViewport.t,
    contentSize: DOM.Rect.t,
  };

  [@deriving yojson]
  type response = Response.t(result);

  let parse = response =>
    try(response |> Yojson.Safe.from_string |> response_of_yojson) {
    | _ as exn =>
      print_endline("Error parsing GetLayoutMetrics!");
      raise(exn);
    };

  let make = (~sessionId=?, ()) => {
    Request.make("Page.getLayoutMetrics", ~sessionId?)
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

  let parse = response =>
    try(response |> Yojson.Safe.from_string |> response_of_yojson) {
    | _ as exn =>
      print_endline("Error parsing Navigate!");
      raise(exn);
    };

  let make = (~sessionId=?, ~referrer=?, url) => {
    Request.make("Page.navigate", ~params={url, referrer}, ~sessionId?)
    |> yojson_of_request
    |> Yojson.Safe.to_string;
  };
};
