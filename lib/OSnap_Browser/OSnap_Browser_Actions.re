open OSnap_CDP;
open OSnap_Browser_Target;

let wait_for = (~event, target) => {
  let sessionId = target.sessionId;
  let (p, resolver) = Lwt.wait();
  let callback = (_, remove) => {
    remove();
    Lwt.wakeup_later(resolver, ());
  };
  OSnap_Websocket.listen(~event, ~sessionId, callback);
  p;
};

let wait_for_network_idle = (target, ~loaderId) => {
  let sessionId = target.sessionId;
  let (p, resolver) = Lwt.wait();

  OSnap_Websocket.listen(
    ~event=Page.Events.LifecycleEvent.name,
    ~sessionId,
    (response, remove) => {
      open Page.Events;
      let eventData = LifecycleEvent.parse(response);
      let name = eventData.Types.Event.params.LifecycleEvent.name;
      let returned_loaderId =
        eventData.Types.Event.params.LifecycleEvent.loaderId;
      if (name == "networkIdle" && loaderId == returned_loaderId) {
        remove();
        Lwt.wakeup_later(resolver, ());
      };
    },
  );

  p;
};

let go_to = (~url, target) => {
  let debug = OSnap_Logger.debug(~header="Browser.go_to");
  let sessionId = target.sessionId;
  debug(Printf.sprintf("session %S \n\t navigationg to %S", sessionId, url));
  let%lwt payload =
    Page.Navigate.make(url, ~sessionId)
    |> OSnap_Websocket.send
    |> Lwt.map(Page.Navigate.parse);

  switch (payload.Types.Response.result.Page.Navigate.errorText) {
  | Some(error) => error |> Lwt_result.fail
  | None =>
    payload.Types.Response.result.Page.Navigate.loaderId |> Lwt_result.return
  };
};

let type_text = (~selector, ~text, target) => {
  let sessionId = target.sessionId;

  let%lwt document =
    Dom.GetDocument.make(~sessionId, ())
    |> OSnap_Websocket.send
    |> Lwt.map(Dom.GetDocument.parse)
    |> Lwt.map(response => response.Types.Response.result);

  let%lwt node =
    Dom.QuerySelector.make(
      ~sessionId,
      ~nodeId=document.root.nodeId,
      ~selector,
      (),
    )
    |> OSnap_Websocket.send
    |> Lwt.map(Dom.QuerySelector.parse)
    |> Lwt.map(response => response.Types.Response.result);

  let%lwt () =
    Dom.Focus.make(~sessionId, ~nodeId=node.nodeId, ())
    |> OSnap_Websocket.send
    |> Lwt.map(ignore);

  List.init(String.length(text), String.get(text))
  |> Lwt_list.iter_s(char => {
       let definition: option(OSnap_Browser_KeyDefinition.t) =
         OSnap_Browser_KeyDefinition.make(char);
       switch (definition) {
       | Some(def) =>
         let%lwt () =
           Input.DispatchKeyEvent.make(
             ~sessionId,
             ~type_=`keyDown,
             ~windowsVirtualKeyCode=Option.value(def.keyCode, ~default=0),
             ~key=def.key,
             ~code=def.code,
             ~text=def.text,
             ~unmodifiedText=def.text,
             ~location=def.location,
             ~isKeypad=def.location == 3,
             (),
           )
           |> OSnap_Websocket.send
           |> Lwt.map(ignore);

         Input.DispatchKeyEvent.make(
           ~sessionId,
           ~type_=`keyUp,
           ~key=def.key,
           ~code=def.code,
           ~location=def.location,
           (),
         )
         |> OSnap_Websocket.send
         |> Lwt.map(ignore);
       | None => Lwt.return()
       };
     });
};

let get_quads = (~selector, target) => {
  let sessionId = target.sessionId;
  let%lwt document =
    Dom.GetDocument.make(~sessionId, ())
    |> OSnap_Websocket.send
    |> Lwt.map(Dom.GetDocument.parse)
    |> Lwt.map(response => response.Types.Response.result);

  let%lwt node =
    Dom.QuerySelector.make(
      ~sessionId,
      ~nodeId=document.root.nodeId,
      ~selector,
      (),
    )
    |> OSnap_Websocket.send
    |> Lwt.map(Dom.QuerySelector.parse)
    |> Lwt.map(response => response.Types.Response.result);

  let%lwt quads =
    Dom.GetContentQuads.make(~sessionId, ~nodeId=node.nodeId, ())
    |> OSnap_Websocket.send
    |> Lwt.map(Dom.GetContentQuads.parse)
    |> Lwt.map(response =>
         response.Types.Response.result.Dom.GetContentQuads.quads
       );

  switch (quads) {
  | [[x1, y1, x2, _y2, _x3, y2, _x4, _y4, ..._], ..._] =>
    Lwt.return(((x1, y1), (x2, y2)))
  | _ => Lwt.return(((0., 0.), (0., 0.)))
  };
};

let click = (~selector, target) => {
  let sessionId = target.sessionId;

  let%lwt ((x1, y1), (x2, y2)) = get_quads(~selector, target);

  let center_x = x1 +. (x2 -. x1) /. 2.0;
  let center_y = y1 +. (y2 -. y1) /. 2.0;

  let%lwt () =
    Input.DispatchMouseEvent.make(
      ~sessionId,
      ~x=center_x,
      ~y=center_y,
      ~type_=`mouseMoved,
      (),
    )
    |> OSnap_Websocket.send
    |> Lwt.map(ignore);

  let%lwt _ =
    Input.DispatchMouseEvent.make(
      ~type_=`mousePressed,
      ~button="left",
      ~buttons=1,
      ~clickCount=1,
      ~sessionId,
      ~x=center_x,
      ~y=center_y,
      (),
    )
    |> OSnap_Websocket.send
    |> Lwt.map(ignore);

  let%lwt _ =
    Input.DispatchMouseEvent.make(
      ~type_=`mouseReleased,
      ~button="left",
      ~buttons=1,
      ~clickCount=1,
      ~sessionId,
      ~x=center_x,
      ~y=center_y,
      (),
    )
    |> OSnap_Websocket.send
    |> Lwt.map(ignore);

  Lwt.return();
};

let set_size = (~width, ~height, target) => {
  let debug = OSnap_Logger.debug(~header="Browser.set_size");
  let sessionId = target.sessionId;

  debug(
    Printf.sprintf(
      "session %S \n\t setting size to %ix%i",
      sessionId,
      width,
      height,
    ),
  );
  let%lwt _ =
    Emulation.SetDeviceMetricsOverride.make(
      ~sessionId,
      ~width,
      ~height,
      ~deviceScaleFactor=1,
      ~mobile=false,
      (),
    )
    |> OSnap_Websocket.send;

  Lwt.return();
};

let screenshot = (~full_size=false, target) => {
  let sessionId = target.sessionId;

  let%lwt clip =
    if (full_size) {
      let%lwt metrics =
        Page.GetLayoutMetrics.make(~sessionId, ())
        |> OSnap_Websocket.send
        |> Lwt.map(Page.GetLayoutMetrics.parse)
        |> Lwt.map(response => response.Types.Response.result);
      let width = metrics.cssContentSize.width;
      let height = metrics.cssContentSize.height;

      Lwt.return(
        Some(Types.Page.Viewport.{x: 0, y: 0, width, height, scale: 1}),
      );
    } else {
      Lwt.return(None);
    };

  let%lwt response =
    Page.CaptureScreenshot.make(
      ~format="png",
      ~sessionId,
      ~clip?,
      ~captureBeyondViewport=true,
      ~fromSurface=true,
      (),
    )
    |> OSnap_Websocket.send
    |> Lwt.map(Page.CaptureScreenshot.parse);

  response.Types.Response.result.Page.CaptureScreenshot.data |> Lwt.return;
};
