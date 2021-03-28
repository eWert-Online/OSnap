open OSnap_CDP_Types;

module Enable = {
  [@deriving yojson]
  type params;

  [@deriving yojson]
  type request = Request.t(params);

  let make = (~sessionId=?, ()) => {
    Request.make("DOM.enable", ~sessionId?)
    |> yojson_of_request
    |> Yojson.Safe.to_string;
  };
};

module GetDocument = {
  [@deriving yojson]
  type params = {
    [@yojson.option]
    depth: option(int), // The maximum depth at which children should be retrieved, defaults to 1. Use -1 for the entire subtree or provide an integer larger than 0.
    [@yojson.option]
    pierce: option(bool) // Whether or not iframes and shadow roots should be traversed when returning the subtree (default is false).
  };

  [@deriving yojson]
  type request = Request.t(params);

  [@deriving yojson]
  type result = {root: DOM.Node.t};

  [@deriving yojson]
  type response = Response.t(result);

  let parse = response =>
    try(response |> Yojson.Safe.from_string |> response_of_yojson) {
    | _ as exn =>
      print_endline("Error parsing GetDocument! Response was:");
      print_endline(response);
      raise(exn);
    };

  let make = (~sessionId=?, ~depth=?, ~pierce=?, ()) => {
    Request.make("DOM.getDocument", ~params={depth, pierce}, ~sessionId?)
    |> yojson_of_request
    |> Yojson.Safe.to_string;
  };
};

module QuerySelector = {
  [@deriving yojson]
  type params = {
    nodeId: DOM.NodeId.t,
    selector: string,
  };

  [@deriving yojson]
  type request = Request.t(params);

  [@deriving yojson]
  type result = {nodeId: DOM.NodeId.t};

  [@deriving yojson]
  type response = Response.t(result);

  let parse = response =>
    try(response |> Yojson.Safe.from_string |> response_of_yojson) {
    | _ as exn =>
      print_endline("Error parsing QuerySelector! Response was:");
      print_endline(response);
      raise(exn);
    };

  let make = (~sessionId=?, ~nodeId, ~selector) => {
    Request.make(
      "DOM.querySelector",
      ~params={nodeId, selector},
      ~sessionId?,
    )
    |> yojson_of_request
    |> Yojson.Safe.to_string;
  };
};

module GetContentQuads = {
  [@deriving yojson]
  type params = {
    [@yojson.option]
    nodeId: option(DOM.NodeId.t),
    [@yojson.option]
    backendNodeId: option(DOM.BackendNodeId.t),
  };

  [@deriving yojson]
  type request = Request.t(params);

  [@deriving yojson]
  type result = {quads: list(DOM.Quad.t)};

  [@deriving yojson]
  type response = Response.t(result);

  let parse = response =>
    try(response |> Yojson.Safe.from_string |> response_of_yojson) {
    | _ as exn =>
      print_endline("Error parsing GetContentQuads! Response was:");
      print_endline(response);
      raise(exn);
    };

  let make = (~sessionId=?, ~nodeId=?, ~backendNodeId=?, ()) => {
    Request.make(
      "DOM.getContentQuads",
      ~params={nodeId, backendNodeId},
      ~sessionId?,
    )
    |> yojson_of_request
    |> Yojson.Safe.to_string;
  };
};
