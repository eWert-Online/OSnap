open OSnap_CDP_Types;

module DispatchMouseEvent = {
  [@deriving yojson]
  type params = {
    [@key "type"]
    type_: string, // mousePressed, mouseReleased, mouseMoved, mouseWheel
    x: float,
    y: float,
    [@yojson.option]
    modifiers: option(int),
    [@yojson.option]
    timestamp: option(Input.TimeSinceEpoch.t),
    [@yojson.option]
    button: option(Input.MouseButton.t),
    [@yojson.option]
    buttons: option(int),
    [@yojson.option]
    clickCount: option(int),
    [@yojson.option]
    force: option(float),
    [@yojson.option]
    tangentialPressure: option(float),
    [@yojson.option]
    tiltX: option(int),
    [@yojson.option]
    tiltY: option(int),
    [@yojson.option]
    twist: option(int),
    [@yojson.option]
    deltaX: option(int),
    [@yojson.option]
    deltaY: option(int),
    [@yojson.option]
    pointerType: option(string),
  };

  [@deriving yojson]
  type request = Request.t(params);

  [@deriving yojson]
  type result;

  [@deriving yojson]
  type response = Response.t(result);

  let make =
      (
        ~sessionId=?,
        ~modifiers=?,
        ~timestamp=?,
        ~button=?,
        ~buttons=?,
        ~clickCount=?,
        ~force=?,
        ~tangentialPressure=?,
        ~tiltX=?,
        ~tiltY=?,
        ~twist=?,
        ~deltaX=?,
        ~deltaY=?,
        ~pointerType=?,
        ~type_: [
           | `mousePressed
           | `mouseReleased
           | `mouseMoved
           | `mouseWheel
         ],
        ~x: float,
        ~y: float,
        (),
      ) => {
    Request.make(
      "Input.dispatchMouseEvent",
      ~params={
        type_:
          switch (type_) {
          | `mouseMoved => "mouseMoved"
          | `mousePressed => "mousePressed"
          | `mouseReleased => "mouseReleased"
          | `mouseWheel => "mouseWheel"
          },
        x,
        y,
        modifiers,
        timestamp,
        button,
        buttons,
        clickCount,
        force,
        tangentialPressure,
        tiltX,
        tiltY,
        twist,
        deltaX,
        deltaY,
        pointerType,
      },
      ~sessionId?,
    )
    |> yojson_of_request
    |> Yojson.Safe.to_string;
  };
};
