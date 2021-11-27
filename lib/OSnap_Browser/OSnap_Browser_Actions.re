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

let select_element = (~selector, ~sessionId) => {
  open Commands.DOM;
  open Lwt_result.Syntax;

  let* document =
    GetDocument.(
      Request.make(~sessionId, ~params=Params.make())
      |> OSnap_Websocket.send
      |> Lwt.map(Response.parse)
      |> Lwt.map(response => {
           let error =
             response.Response.error
             |> Option.map((error: Response.error) =>
                  OSnap_Response.CDP_Protocol_Error(error.message)
                )
             |> Option.value(~default=OSnap_Response.CDP_Protocol_Error(""));

           Option.to_result(response.Response.result, ~none=error);
         })
    );

  QuerySelector.(
    Request.make(
      ~sessionId,
      ~params=Params.make(~nodeId=document.root.nodeId, ~selector, ()),
    )
    |> OSnap_Websocket.send
    |> Lwt.map(Response.parse)
    |> Lwt.map(response => {
         let error =
           response.Response.error
           |> Option.map((error: Response.error) =>
                OSnap_Response.CDP_Protocol_Error(error.message)
              )
           |> Option.value(~default=OSnap_Response.CDP_Protocol_Error(""));

         Option.to_result(response.Response.result, ~none=error);
       })
  );
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
  open Lwt_result.Syntax;

  let sessionId = target.sessionId;

  let debug = OSnap_Logger.debug(~header="Browser.go_to");
  debug(Printf.sprintf("session %S \n\t navigationg to %S", sessionId, url));

  let params = Navigate.Params.make(~url, ());

  let* result =
    Navigate.(
      Request.make(~sessionId, ~params)
      |> OSnap_Websocket.send
      |> Lwt.map(Navigate.Response.parse)
      |> Lwt.map(response => {
           let error =
             response.Response.error
             |> Option.map((error: Response.error) =>
                  OSnap_Response.CDP_Protocol_Error(error.message)
                )
             |> Option.value(~default=OSnap_Response.CDP_Protocol_Error(""));

           Option.to_result(response.Response.result, ~none=error);
         })
    );

  switch (result.errorText, result.loaderId) {
  | (Some(error), _) =>
    OSnap_Response.CDP_Protocol_Error(error) |> Lwt_result.fail
  | (None, None) =>
    Lwt_result.fail(
      OSnap_Response.CDP_Protocol_Error("CDP responded with no loader id"),
    )
  | (None, Some(loaderId)) => loaderId |> Lwt_result.return
  };
};

let type_text = (~selector, ~text, target) => {
  open Commands.DOM;
  open Lwt_result.Syntax;

  let sessionId = target.sessionId;

  let* node = select_element(~selector, ~sessionId);

  let* () =
    Focus.(
      Request.make(~sessionId, ~params=Params.make(~nodeId=node.nodeId, ()))
      |> OSnap_Websocket.send
      |> Lwt.map(Response.parse)
      |> Lwt.map(response => {
           let error =
             response.Response.error
             |> Option.map((error: Response.error) =>
                  OSnap_Response.CDP_Protocol_Error(error.message)
                )
             |> Option.value(~default=OSnap_Response.CDP_Protocol_Error(""));

           Option.to_result(response.Response.result, ~none=error);
         })
      |> Lwt_result.map(ignore)
    );

  let* () =
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
       })
    |> Lwt_result.ok;

  let* wait_result =
    wait_for(
      ~event="Page.frameNavigated",
      ~look_behind=false,
      ~timeout=1000.,
      target,
    )
    |> Lwt_result.ok;

  switch (wait_result) {
  | `Timeout => Lwt_result.return()
  | `Data(data) =>
    let event_data = Cdp.Events.Page.FrameNavigated.parse(data);
    let loaderId = event_data.params.frame.loaderId;
    wait_for_network_idle(target, ~loaderId) |> Lwt_result.ok;
  };
};

let get_quads = (~selector, target) => {
  open Commands.DOM;
  open Lwt_result.Syntax;

  let sessionId = target.sessionId;

  let* node = select_element(~selector, ~sessionId);

  let* result =
    GetContentQuads.(
      Request.make(~sessionId, ~params=Params.make(~nodeId=node.nodeId, ()))
      |> OSnap_Websocket.send
      |> Lwt.map(Response.parse)
      |> Lwt.map(response => {
           let error =
             response.Response.error
             |> Option.map((error: Response.error) =>
                  OSnap_Response.CDP_Protocol_Error(error.message)
                )
             |> Option.value(~default=OSnap_Response.CDP_Protocol_Error(""));

           Option.to_result(response.Response.result, ~none=error);
         })
    );

  switch (result.quads) {
  | [[x1, y1, x2, _y2, _x3, y2, _x4, _y4, ..._], ..._] =>
    Lwt_result.return(((x1, y1), (x2, y2)))
  | _ => Lwt_result.fail(OSnap_Response.CDP_Protocol_Error(""))
  };
};

let click = (~selector, target) => {
  open Commands.Input;
  open Lwt_result.Syntax;

  let sessionId = target.sessionId;

  let* ((x1, y1), (x2, y2)) = get_quads(~selector, target);

  let x = x1 +. (x2 -. x1) /. 2.0;
  let y = y1 +. (y2 -. y1) /. 2.0;

  let* () =
    DispatchMouseEvent.(
      Request.make(
        ~sessionId,
        ~params=Params.make(~x, ~y, ~type_=`mouseMoved, ()),
      )
      |> OSnap_Websocket.send
      |> Lwt.map(ignore)
      |> Lwt_result.ok
    );

  let* _ =
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
      |> Lwt_result.ok
    );

  let* () =
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
      |> Lwt_result.ok
    );

  let* wait_result =
    wait_for(
      ~event="Page.frameNavigated",
      ~look_behind=false,
      ~timeout=1000.,
      target,
    )
    |> Lwt_result.ok;

  switch (wait_result) {
  | `Timeout => Lwt_result.return()
  | `Data(data) =>
    let event_data = Cdp.Events.Page.FrameNavigated.parse(data);
    let loaderId = event_data.params.frame.loaderId;
    wait_for_network_idle(target, ~loaderId) |> Lwt_result.ok;
  };
};

let get_content_size = target => {
  open Commands.Page;
  open Lwt_result.Syntax;

  let sessionId = target.sessionId;

  let* metrics =
    GetLayoutMetrics.(
      Request.make(~sessionId)
      |> OSnap_Websocket.send
      |> Lwt.map(Response.parse)
      |> Lwt.map(response => {
           let error =
             response.Response.error
             |> Option.map((error: Response.error) =>
                  OSnap_Response.CDP_Protocol_Error(error.message)
                )
             |> Option.value(~default=OSnap_Response.CDP_Protocol_Error(""));

           Option.to_result(response.Response.result, ~none=error);
         })
    );

  Lwt_result.return((
    metrics.cssContentSize.width,
    metrics.cssContentSize.height,
  ));
};

let set_size = (~width, ~height, target) => {
  open Commands.Emulation;
  open Lwt_result.Syntax;

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

  let* _ =
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
      |> Lwt.map(Response.parse)
      |> Lwt.map(response => {
           let error =
             response.Response.error
             |> Option.map((error: Response.error) =>
                  OSnap_Response.CDP_Protocol_Error(error.message)
                )
             |> Option.value(~default=OSnap_Response.CDP_Protocol_Error(""));

           Option.to_result(response.Response.result, ~none=error);
         })
    );

  Lwt_result.return();
};

let screenshot = (~full_size=false, target) => {
  open Commands.Page;
  open Lwt_result.Syntax;

  let sessionId = target.sessionId;

  let* () =
    if (full_size) {
      let* (width, height) = get_content_size(target);
      set_size(~width, ~height, target);
    } else {
      Lwt_result.return();
    };

  let* result =
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
      |> Lwt.map(response => {
           let error =
             response.Response.error
             |> Option.map((error: Response.error) =>
                  OSnap_Response.CDP_Protocol_Error(error.message)
                )
             |> Option.value(~default=OSnap_Response.CDP_Protocol_Error(""));

           Option.to_result(response.Response.result, ~none=error);
         })
    );

  Lwt_result.return(result.data);
};
