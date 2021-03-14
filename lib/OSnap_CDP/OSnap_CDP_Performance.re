open OSnap_CDP_Types;

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
