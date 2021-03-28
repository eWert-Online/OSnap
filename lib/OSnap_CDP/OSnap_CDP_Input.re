open OSnap_CDP_Types;

module DispatchKeyEvent = {
  [@deriving yojson]
  type params = {
    [@key "type"]
    type_: string, // keyDown, keyUp, rawKeyDown, char
    [@yojson.option]
    modifiers: option(int), // Bit field representing pressed modifier keys. Alt=1, Ctrl=2, Meta/Command=4, Shift=8 (default: 0).
    [@yojson.option]
    timestamp: option(Input.TimeSinceEpoch.t), // Time at which the event occurred.
    [@yojson.option]
    text: option(string), // Text as generated by processing a virtual key code with a keyboard layout. Not needed for for keyUp and rawKeyDown events (default: "")
    [@yojson.option]
    unmodifiedText: option(string), // Text that would have been generated by the keyboard if no modifiers were pressed (except for shift). Useful for shortcut (accelerator) key handling (default: "").
    [@yojson.option]
    keyIdentifier: option(string), // Unique key identifier (e.g., 'U+0041') (default: "").
    [@yojson.option]
    code: option(string), // Unique DOM defined string value for each physical key (e.g., 'KeyA') (default: "").
    [@yojson.option]
    key: option(string), // Unique DOM defined string value describing the meaning of the key in the context of active modifiers, keyboard layout, etc (e.g., 'AltGr') (default: "").
    [@yojson.option]
    windowsVirtualKeyCode: option(int), // Windows virtual key code (default: 0).
    [@yojson.option]
    nativeVirtualKeyCode: option(int), // Native virtual key code (default: 0).
    [@yojson.option]
    autoRepeat: option(bool), // Whether the event was generated from auto repeat (default: false).
    [@yojson.option]
    isKeypad: option(bool), // Whether the event was generated from the keypad (default: false).
    [@yojson.option]
    isSystemKey: option(bool), // Whether the event was a system key event (default: false).
    [@yojson.option]
    location: option(int) // Whether the event was from the left or right side of the keyboard. 1=Left, 2=Right (default: 0).
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
        ~text=?,
        ~unmodifiedText=?,
        ~keyIdentifier=?,
        ~code=?,
        ~key=?,
        ~windowsVirtualKeyCode=?,
        ~nativeVirtualKeyCode=?,
        ~autoRepeat=?,
        ~isKeypad=?,
        ~isSystemKey=?,
        ~location=?,
        ~type_: [ | `keyDown | `keyUp | `rawKeyDown | `char],
        (),
      ) => {
    Request.make(
      "Input.dispatchKeyEvent",
      ~params={
        type_:
          switch (type_) {
          | `keyDown => "keyDown"
          | `keyUp => "keyUp"
          | `rawKeyDown => "rawKeyDown"
          | `char => "char"
          },
        modifiers,
        timestamp,
        text,
        unmodifiedText,
        keyIdentifier,
        code,
        key,
        windowsVirtualKeyCode,
        nativeVirtualKeyCode,
        autoRepeat,
        isKeypad,
        isSystemKey,
        location,
      },
      ~sessionId?,
    )
    |> yojson_of_request
    |> Yojson.Safe.to_string;
  };
};

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
