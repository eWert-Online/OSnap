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

  let make = (~sessionId=?, ~enabled, ()) => {
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
    loaderId: string,
    [@yojson.option]
    errorText: option(string),
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

module Events = {
  module LifecycleEvent = {
    let name = "Page.lifecycleEvent";

    [@deriving yojson]
    [@yojson.allow_extra_fields]
    type result = {
      frameId: Page.FrameId.t,
      loaderId: Network.LoaderId.t,
      name: string,
      timestamp: Network.MonotonicTime.t,
    };

    [@deriving yojson]
    type event = Event.t(result);

    let parse = event =>
      try(event |> Yojson.Safe.from_string |> event_of_yojson) {
      | Ppx_yojson_conv_lib.Yojson_conv.Of_yojson_error(a, b) =>
        print_endline("Error parsing " ++ name);
        print_endline("Event data was:");
        print_endline(event);
        Printexc.to_string(a) |> print_endline;
        raise(Ppx_yojson_conv_lib.Yojson_conv.Of_yojson_error(a, b));
      | _ as exn =>
        print_endline("Error parsing " ++ name);
        print_endline("Event data was:");
        print_endline(event);
        raise(exn);
      };
  };
};
