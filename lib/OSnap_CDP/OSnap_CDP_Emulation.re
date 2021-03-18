open OSnap_CDP_Types;

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

  let parse = response =>
    try(response |> Yojson.Safe.from_string |> response_of_yojson) {
    | _ as exn =>
      print_endline("Error parsing SetDeviceMetricsOverride!");
      raise(exn);
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
