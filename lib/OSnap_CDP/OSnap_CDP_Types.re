module Input = {
  module TimeSinceEpoch = {
    [@deriving yojson]
    type t = float;
  };

  module MouseButton = {
    // none, left, middle, right, back, forward
    [@deriving yojson]
    type t = string;
  };
};

module Browser = {
  module BrowserContextID = {
    [@deriving yojson]
    type t = string;
  };
};

module Security = {
  module MixedContentType = {
    [@deriving yojson]
    type t = string;
  };
};

module Network = {
  module RequestId = {
    [@deriving yojson]
    type t = string;
  };

  module LoaderId = {
    [@deriving yojson]
    type t = string;
  };

  module ResourcePriority = {
    [@deriving yojson]
    type t = string;
  };

  module ResourceType = {
    [@deriving yojson]
    type t = string;
  };

  module CorsError = {
    [@deriving yojson]
    type t = string;
  };

  module CorsErrorStatus = {
    [@deriving yojson]
    type t = {
      corsError: CorsError.t,
      failedParameter: string,
    };
  };

  module BlockedReason = {
    [@deriving yojson]
    type t = string;
  };

  module MonotonicTime = {
    [@deriving yojson]
    type t = float;
  };

  module TimeSinceEpoch = {
    [@deriving yojson]
    type t = float;
  };

  module Request = {
    [@deriving yojson]
    [@yojson.allow_extra_fields]
    type t = {
      url: string, // Request URL (without fragment).
      method: string // HTTP request method.
    };
  };
};

module Page = {
  module FrameId = {
    [@deriving yojson]
    type t = string;
  };

  module Viewport = {
    [@deriving yojson]
    type t = {
      x: int, // X offset in device independent pixels (dip).
      y: int, // Y offset in device independent pixels (dip).
      width: int, // Rectangle width in device independent pixels (dip).
      height: int, // Rectangle height in device independent pixels (dip).
      scale: int // Page scale factor.
    };
  };

  module LayoutViewport = {
    [@deriving yojson]
    type t = {
      pageX: int, // Horizontal offset relative to the document (CSS pixels).
      pageY: int, // Vertical offset relative to the document (CSS pixels).
      clientWidth: int, // Width (CSS pixels), excludes scrollbar if present.
      clientHeight: int // Height (CSS pixels), excludes scrollbar if present.
    };
  };

  module VisualViewport = {
    [@deriving yojson]
    type t = {
      offsetX: int, // Horizontal offset relative to the layout viewport (CSS pixels).
      offsetY: int, // Vertical offset relative to the layout viewport (CSS pixels).
      pageX: int, // Horizontal offset relative to the document (CSS pixels).
      pageY: int, // Vertical offset relative to the document (CSS pixels).
      clientWidth: int, // Width (CSS pixels), excludes scrollbar if present.
      clientHeight: int, // Height (CSS pixels), excludes scrollbar if present.
      scale: int, // Scale relative to the ideal viewport (size at width=device-width).
      [@yojson.option]
      zoom: option(int) // Page zoom factor (CSS to device independent pixels ratio).
    };
  };
};

module Target = {
  module TargetId = {
    [@deriving yojson]
    type t = string;
  };

  module SessionId = {
    [@deriving yojson]
    type t = string;
  };

  module TargetInfo = {
    [@deriving yojson]
    type t = {
      targetId: TargetId.t,
      [@key "type"]
      type_: string,
      title: string,
      url: string,
      attached: bool,
      canAccessOpener: bool,
      browserContextId: string,
    };
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
    sessionId: option(Target.SessionId.t),
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

module DOM = {
  module Quad = {
    type t = list(float);
    let t_of_yojson: Yojson.Safe.t => t =
      fun
      | `List(list) =>
        list
        |> List.map(
             fun
             | `Float(f) => f
             | `Int(i) => float_of_int(i)
             | _ => 0.0,
           )
      | _ => [];

    let yojson_of_t: t => Yojson.Safe.t =
      quads => {
        `List(quads |> List.map(quad => `Float(quad)));
      };
  };

  module NodeId = {
    [@deriving yojson]
    type t = int;
  };

  module Rect = {
    [@deriving yojson]
    type t = {
      x: int,
      y: int,
      width: int,
      height: int,
    };
  };

  module BackendNodeId = {
    [@deriving yojson]
    type t = int;
  };

  module BackendNode = {
    [@deriving yojson]
    type t = {
      nodeType: int, // Node's nodeType.
      nodeName: string, // Node's nodeName.
      backendNodeId: BackendNodeId.t,
    };
  };

  module PseudoType = {
    [@deriving yojson]
    type t = string;
  };

  module ShadowRootType = {
    [@deriving yojson]
    type t = string;
  };

  module Node = {
    [@deriving yojson]
    [@yojson.allow_extra_fields]
    type t = {
      nodeId: NodeId.t, // Node identifier that is passed into the rest of the DOM messages as the nodeId. Backend will only push node with given id once. It is aware of all requested nodes and will only fire DOM events for nodes known to the client.
      [@yojson.option]
      parentId: option(NodeId.t), // The id of the parent node if any.
      backendNodeId: BackendNodeId.t, // The BackendNodeId for this node.
      nodeType: int, // Node's nodeType.
      nodeName: string, // Node's nodeName.
      localName: string, // Node's localName.
      nodeValue: string, // Node's nodeValue.
      [@yojson.option]
      childNodeCount: option(int), // Child count for Container nodes.
      [@yojson.option]
      attributes: option(list(string)), // Attributes of the Element node in the form of flat list [name1, value1, name2, value2].
      [@yojson.option]
      documentURL: option(string), // Document URL that Document or FrameOwner node points to.
      [@yojson.option]
      baseURL: option(string), // Base URL that Document or FrameOwner node uses for URL completion.
      [@yojson.option]
      publicId: option(string), // DocumentType's publicId.
      [@yojson.option]
      systemId: option(string), // DocumentType's systemId.
      [@yojson.option]
      internalSubset: option(string), // DocumentType's internalSubset.
      [@yojson.option]
      xmlVersion: option(string), // Document's XML version in case of XML documents.
      [@yojson.option]
      name: option(string), // Attr's name.
      [@yojson.option]
      value: option(string), // Attr's value.
      [@yojson.option]
      pseudoType: option(PseudoType.t), // Pseudo element type for this node.
      [@yojson.option]
      shadowRootType: option(ShadowRootType.t), // Shadow root type.
      [@yojson.option]
      frameId: option(Page.FrameId.t), // Frame ID for frame owner elements.
      [@yojson.option]
      distributedNodes: option(list(BackendNode.t)), // Distributed nodes for given insertion point.
      [@yojson.option]
      isSVG: option(bool) // Whether the node is SVG.
    };
  };
};

module Response = {
  [@deriving yojson]
  type t('a) = {
    id: int,
    result: 'a,
    [@yojson.option]
    sessionId: option(Target.SessionId.t),
  };
};

module Event = {
  [@deriving yojson]
  type t('a) = {
    [@key "method"]
    method: string,
    [@key "params"]
    params: 'a,
    [@key "sessionId"]
    sessionId: Target.SessionId.t,
  };
};
