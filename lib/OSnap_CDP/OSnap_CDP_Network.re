open OSnap_CDP_Types;

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

module Events = {
  module RequestWillBeSent = {
    let name = "Network.requestWillBeSent";

    [@deriving yojson]
    [@yojson.allow_extra_fields]
    type result = {
      requestId: Network.RequestId.t, // Request identifier.
      loaderId: Network.LoaderId.t, // Loader identifier. Empty string if the request is fetched from worker.
      documentURL: string, // URL of the document this request is loaded for.
      request: Network.Request.t, // Request data.
      [@key "type"]
      type_: option(Network.ResourceType.t), // Type of this resource.
      frameId: option(Page.FrameId.t) // Frame identifier.
    };

    [@deriving yojson]
    type event = Event.t(result);

    let parse = event =>
      try(event |> Yojson.Safe.from_string |> event_of_yojson) {
      | Ppx_yojson_conv_lib.Yojson_conv.Of_yojson_error(a, b) =>
        print_endline("Error parsing Network.requestWillBeSent!");
        print_endline("Event data was:");
        print_endline(event);
        Printexc.to_string(a) |> print_endline;
        raise(Ppx_yojson_conv_lib.Yojson_conv.Of_yojson_error(a, b));
      | _ as exn =>
        print_endline("Error parsing Network.requestWillBeSent!");
        print_endline("Event data was:");
        print_endline(event);
        raise(exn);
      };
  };

  /**
   * Fired when HTTP request has finished loading.
   */
  module LoadingFinished = {
    let name = "Network.loadingFinished";

    [@deriving yojson]
    [@yojson.allow_extra_fields]
    type result = {
      requestId: Network.RequestId.t, // Request identifier.
      timestamp: Network.MonotonicTime.t, // Timestamp.
      encodedDataLength: int // Total number of bytes received for this request.
    };

    [@deriving yojson]
    type event = Event.t(result);

    let parse = event =>
      try(event |> Yojson.Safe.from_string |> event_of_yojson) {
      | Ppx_yojson_conv_lib.Yojson_conv.Of_yojson_error(a, b) =>
        print_endline("Error parsing Network.loadingFinished!");
        print_endline("Event data was:");
        print_endline(event);
        Printexc.to_string(a) |> print_endline;
        raise(Ppx_yojson_conv_lib.Yojson_conv.Of_yojson_error(a, b));
      | _ as exn =>
        print_endline("Error parsing Network.loadingFinished!");
        print_endline("Event data was:");
        print_endline(event);
        raise(exn);
      };
  };

  /**
   * Fired when HTTP request has finished loading.
   */
  module LoadingFailed = {
    let name = "Network.loadingFailed";

    [@deriving yojson]
    [@yojson.allow_extra_fields]
    type result = {
      requestId: Network.RequestId.t, // Request identifier.
      timestamp: Network.MonotonicTime.t, // Timestamp.
      [@key "type"]
      type_: Network.ResourceType.t, // Type of this resource.
      [@yojson.option]
      errorText: option(string), // User friendly error message.
      [@yojson.option]
      canceled: option(bool), // True if loading was canceled.
      [@yojson.option]
      blockedReason: option(Network.BlockedReason.t),
      [@yojson.option]
      corsErrorStatus: option(Network.CorsErrorStatus.t),
    };

    [@deriving yojson]
    type event = Event.t(result);

    let parse = event =>
      try(event |> Yojson.Safe.from_string |> event_of_yojson) {
      | Ppx_yojson_conv_lib.Yojson_conv.Of_yojson_error(a, b) =>
        print_endline("Error parsing Network.loadingFailed!");
        print_endline("Event data was:");
        print_endline(event);
        Printexc.to_string(a) |> print_endline;
        raise(Ppx_yojson_conv_lib.Yojson_conv.Of_yojson_error(a, b));
      | _ as exn =>
        print_endline("Error parsing Network.loadingFailed!");
        print_endline("Event data was:");
        print_endline(event);
        raise(exn);
      };
  };
};
