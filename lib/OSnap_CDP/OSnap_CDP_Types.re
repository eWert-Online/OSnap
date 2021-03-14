module Browser = {
  module BrowserContextID = {
    [@deriving yojson]
    type t = string;
  };
};

module DOM = {
  module Rect = {
    [@deriving yojson]
    type t = {
      x: int,
      y: int,
      width: int,
      height: int,
    };
  };
};

module Page = {
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
    sessionId: option(Target.SessionID.t),
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
    sessionId: option(Target.SessionID.t),
  };
};
