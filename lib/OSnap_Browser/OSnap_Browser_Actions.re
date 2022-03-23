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
         switch (response.Response.error, response.Response.result) {
         | (_, Some({nodeId: `Int(0)}) | Some({nodeId: `Float(0.)})) =>
           Result.error(
             OSnap_Response.CDP_Protocol_Error(
               Printf.sprintf(
                 "A node with the selector %S could not be found.",
                 selector,
               ),
             ),
           )
         | (None, None) =>
           Result.error(OSnap_Response.CDP_Protocol_Error(""))
         | (Some({message, _}), None) =>
           Result.error(OSnap_Response.CDP_Protocol_Error(message))
         | (Some(_), Some(result))
         | (None, Some(result)) => Result.ok(result)
         }
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
                       def.keyCode
                       |> Option.map(i => `Int(i))
                       |> Option.value(~default=`Int(0)),
                     ~key=def.key,
                     ~code=def.code,
                     ~text=def.text,
                     ~unmodifiedText=def.text,
                     ~location=`Int(def.location),
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
                   ~location=`Int(def.location),
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

  let* {nodeId} = select_element(~selector, ~sessionId);

  let* result =
    GetContentQuads.(
      Request.make(~sessionId, ~params=Params.make(~nodeId, ()))
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

  let to_float =
    fun
    | `Float(f) => f
    | `Int(i) => float_of_int(i);

  switch (result.quads) {
  | [[x1, y1, x2, _y2, _x3, y2, _x4, _y4, ..._], ..._] =>
    Lwt_result.return((
      (to_float(x1), to_float(y1)),
      (to_float(x2), to_float(y2)),
    ))
  | _ => Lwt_result.fail(OSnap_Response.CDP_Protocol_Error(""))
  };
};

let click = (~selector, target) => {
  open Commands.Input;
  open Lwt_result.Syntax;

  let sessionId = target.sessionId;

  let* ((x1, y1), (x2, y2)) = get_quads(~selector, target);

  let x = `Float(x1 +. (x2 -. x1) /. 2.0);
  let y = `Float(y1 +. (y2 -. y1) /. 2.0);

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
            ~buttons=`Int(1),
            ~clickCount=`Int(1),
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
            ~buttons=`Int(1),
            ~clickCount=`Int(1),
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

let scroll = (~selector, ~px, target) => {
  open Lwt_result.Syntax;

  let sessionId = target.sessionId;

  switch (px, selector) {
  | (None, None) => assert(false)
  | (Some(_), Some(_)) => assert(false)
  | (None, Some(selector)) =>
    let* {nodeId} = select_element(~selector, ~sessionId);
    Commands.DOM.ScrollIntoViewIfNeeded.(
      Request.make(~sessionId, ~params=Params.make(~nodeId, ()))
      |> OSnap_Websocket.send
      |> Lwt.map(Response.parse)
      |> Lwt.map(response => {
           switch (response.Response.error) {
           | None => Result.ok()
           | Some({message, _}) =>
             Result.error(OSnap_Response.CDP_Protocol_Error(message))
           }
         })
    );
  | (Some(px), None) =>
    let expression =
      Printf.sprintf(
        {|
          window.scrollTo({
            top: %i,
            left: 0,
            behavior: 'smooth'
          });
        |},
        px,
      );

    Commands.Runtime.Evaluate.(
      Request.make(~sessionId, ~params=Params.make(~expression, ()))
      |> OSnap_Websocket.send
      |> Lwt.map(Response.parse)
      |> Lwt.bind(_, response => {
           switch (response.Response.error) {
           | None =>
             let timeout = float_of_int(px / 200);
             Lwt_unix.sleep(timeout) |> Lwt_result.ok;
           | Some({message, _}) =>
             Lwt_result.fail(OSnap_Response.CDP_Protocol_Error(message))
           }
         })
    );
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

  let sessionId = target.sessionId;

  let* _ =
    SetDeviceMetricsOverride.(
      Request.make(
        ~sessionId,
        ~params=
          Params.make(
            ~width,
            ~height,
            ~deviceScaleFactor=`Int(1),
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

let clear_cookies = target => {
  open Commands.Storage;
  open Lwt_result.Syntax;

  let sessionId = target.sessionId;

  let* _ =
    ClearCookies.(
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

  Lwt_result.return();
};
