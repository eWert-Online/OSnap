open Cdp;
open OSnap_Browser_Target;

let wait_for = (~timeout=?, ~look_behind=?, ~event, target) => {
  let sessionId = target.sessionId;
  let (p, resolver) = Lwt.wait();
  let callback = (data, remove) => {
    remove();
    Lwt.wakeup_later(resolver, `Data(data));
  };
  OSnap_Websocket.listen(~event, ~look_behind?, ~sessionId, callback);
  switch (timeout) {
  | None => p
  | Some(t) =>
    let timeout = Lwt_unix.sleep(t /. 1000.) |> Lwt.map(() => `Timeout);
    Lwt.pick([timeout, p]);
  };
};

let wait_for_network_idle = (target, ~loaderId) => {
  open Events.Page;

  let sessionId = target.sessionId;
  let (p, resolver) = Lwt.wait();

  OSnap_Websocket.listen(
    ~event=LifecycleEvent.name,
    ~sessionId,
    (response, remove) => {
      let eventData = LifecycleEvent.parse(response);
      if (eventData.params.name == "networkIdle"
          && loaderId == eventData.params.loaderId) {
        remove();
        Lwt.wakeup_later(resolver, ());
      };
    },
  );

  p;
};

let go_to = (~url, target) => {
  open Commands.Page;

  let sessionId = target.sessionId;

  let debug = OSnap_Logger.debug(~header="Browser.go_to");
  debug(Printf.sprintf("session %S \n\t navigationg to %S", sessionId, url));

  let params = Navigate.Params.make(~url, ());

  let%lwt payload =
    Navigate.Request.make(~sessionId, ~params)
    |> OSnap_Websocket.send
    |> Lwt.map(Navigate.Response.parse);

  switch (payload.result.errorText, payload.result.loaderId) {
  | (Some(error), _) => error |> Lwt_result.fail
  | (None, None) => Lwt_result.fail("CDP responded with no loader id")
  | (None, Some(loaderId)) => loaderId |> Lwt_result.return
  };
};

let type_text = (~selector, ~text, target) => {
  open Commands.DOM;

  let sessionId = target.sessionId;

  let%lwt document =
    GetDocument.(
      Request.make(~sessionId, ~params=Params.make())
      |> OSnap_Websocket.send
      |> Lwt.map(Response.parse)
      |> Lwt.map(response => response.Response.result)
    );

  let%lwt node =
    QuerySelector.(
      Request.make(
        ~sessionId,
        ~params=Params.make(~nodeId=document.root.nodeId, ~selector, ()),
      )
      |> OSnap_Websocket.send
      |> Lwt.map(Response.parse)
      |> Lwt.map(response => response.Response.result)
    );

  let%lwt () =
    Focus.(
      Request.make(~sessionId, ~params=Params.make(~nodeId=node.nodeId, ()))
      |> OSnap_Websocket.send
      |> Lwt.map(ignore)
    );

  let%lwt () =
    List.init(String.length(text), String.get(text))
    |> Lwt_list.iter_s(char => {
         let definition: option(OSnap_Browser_KeyDefinition.t) =
           OSnap_Browser_KeyDefinition.make(char);
         switch (definition) {
         | Some(def) =>
           let%lwt () =
             Commands.Input.DispatchKeyEvent.(
               Request.make(
                 ~sessionId,
                 ~params=
                   Params.make(
                     ~type_=`keyDown,
                     ~windowsVirtualKeyCode=
                       Float.of_int(Option.value(def.keyCode, ~default=0)),
                     ~key=def.key,
                     ~code=def.code,
                     ~text=def.text,
                     ~unmodifiedText=def.text,
                     ~location=Float.of_int(def.location),
                     ~isKeypad=def.location == 3,
                     (),
                   ),
               )
               |> OSnap_Websocket.send
               |> Lwt.map(ignore)
             );

           Commands.Input.DispatchKeyEvent.(
             Request.make(
               ~sessionId,
               ~params=
                 Params.make(
                   ~type_=`keyUp,
                   ~key=def.key,
                   ~code=def.code,
                   ~location=Float.of_int(def.location),
                   (),
                 ),
             )
             |> OSnap_Websocket.send
             |> Lwt.map(ignore)
           );
         | None => Lwt.return()
         };
       });

  let%lwt wait_result =
    wait_for(
      ~event="Page.frameNavigated",
      ~look_behind=false,
      ~timeout=1000.,
      target,
    );

  switch (wait_result) {
  | `Timeout => Lwt.return()
  | `Data(data) =>
    let event_data = Cdp.Events.Page.FrameNavigated.parse(data);
    let loaderId = event_data.params.frame.loaderId;
    wait_for_network_idle(target, ~loaderId);
  };
};

let get_quads = (~selector, target) => {
  open Commands.DOM;

  let sessionId = target.sessionId;
  let%lwt document =
    GetDocument.(
      Request.make(~sessionId, ~params=Params.make())
      |> OSnap_Websocket.send
      |> Lwt.map(Response.parse)
      |> Lwt.map(response => response.Response.result)
    );

  let%lwt node =
    QuerySelector.(
      Request.make(
        ~sessionId,
        ~params=Params.make(~nodeId=document.root.nodeId, ~selector, ()),
      )
      |> OSnap_Websocket.send
      |> Lwt.map(Response.parse)
      |> Lwt.map(response => response.Response.result)
    );

  let%lwt quads =
    GetContentQuads.(
      Request.make(~sessionId, ~params=Params.make(~nodeId=node.nodeId, ()))
      |> OSnap_Websocket.send
      |> Lwt.map(Response.parse)
      |> Lwt.map(response => response.Response.result.quads)
    );

  switch (quads) {
  | [[x1, y1, x2, _y2, _x3, y2, _x4, _y4, ..._], ..._] =>
    Lwt.return(((x1, y1), (x2, y2)))
  | _ => Lwt.return(((0., 0.), (0., 0.)))
  };
};

let click = (~selector, target) => {
  open Commands.Input;
  let sessionId = target.sessionId;

  let%lwt ((x1, y1), (x2, y2)) = get_quads(~selector, target);

  let x = x1 +. (x2 -. x1) /. 2.0;
  let y = y1 +. (y2 -. y1) /. 2.0;

  let%lwt () =
    DispatchMouseEvent.(
      Request.make(
        ~sessionId,
        ~params=Params.make(~x, ~y, ~type_=`mouseMoved, ()),
      )
      |> OSnap_Websocket.send
      |> Lwt.map(ignore)
    );

  let%lwt _ =
    DispatchMouseEvent.(
      Request.make(
        ~sessionId,
        ~params=
          Params.make(
            ~type_=`mousePressed,
            ~button=`left,
            ~buttons=1.,
            ~clickCount=1.,
            ~x,
            ~y,
            (),
          ),
      )
      |> OSnap_Websocket.send
      |> Lwt.map(ignore)
    );

  let%lwt _ =
    DispatchMouseEvent.(
      Request.make(
        ~sessionId,
        ~params=
          Params.make(
            ~type_=`mouseReleased,
            ~button=`left,
            ~buttons=1.,
            ~clickCount=1.,
            ~x,
            ~y,
            (),
          ),
      )
      |> OSnap_Websocket.send
      |> Lwt.map(ignore)
    );

  let%lwt wait_result =
    wait_for(
      ~event="Page.frameNavigated",
      ~look_behind=false,
      ~timeout=1000.,
      target,
    );

  switch (wait_result) {
  | `Timeout => Lwt.return()
  | `Data(data) =>
    let event_data = Cdp.Events.Page.FrameNavigated.parse(data);
    let loaderId = event_data.params.frame.loaderId;
    wait_for_network_idle(target, ~loaderId);
  };
};

let get_content_size = target => {
  open Commands.Page;

  let sessionId = target.sessionId;

  let%lwt metrics =
    GetLayoutMetrics.(
      Request.make(~sessionId)
      |> OSnap_Websocket.send
      |> Lwt.map(Response.parse)
      |> Lwt.map(response => response.Response.result)
    );

  Lwt.return((metrics.cssContentSize.width, metrics.cssContentSize.height));
};

let set_size = (~width, ~height, target) => {
  open Commands.Emulation;
  let debug = OSnap_Logger.debug(~header="Browser.set_size");
  let sessionId = target.sessionId;

  debug(
    Printf.sprintf(
      "session %S \n\t setting size to %fx%f",
      sessionId,
      width,
      height,
    ),
  );

  let%lwt _ =
    SetDeviceMetricsOverride.(
      Request.make(
        ~sessionId,
        ~params=
          Params.make(
            ~width,
            ~height,
            ~deviceScaleFactor=1.,
            ~mobile=false,
            (),
          ),
      )
      |> OSnap_Websocket.send
    );

  Lwt.return();
};

let screenshot = (~full_size=false, target) => {
  open Commands.Page;

  let sessionId = target.sessionId;

  let%lwt () =
    if (full_size) {
      let%lwt (width, height) = get_content_size(target);
      set_size(~width, ~height, target);
    } else {
      Lwt.return();
    };

  let%lwt response =
    CaptureScreenshot.(
      Request.make(
        ~sessionId,
        ~params=
          Params.make(
            ~format=`png,
            ~captureBeyondViewport=false,
            ~fromSurface=true,
            (),
          ),
      )
      |> OSnap_Websocket.send
      |> Lwt.map(Response.parse)
    );

  Lwt.return(response.result.data);
};
