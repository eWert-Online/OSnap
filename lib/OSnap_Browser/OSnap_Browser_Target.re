open OSnap_Browser_Types;

module Websocket = OSnap_Websocket;

type target = {
  targetId: Cdp.Types.Target.TargetID.t,
  sessionId: Cdp.Types.Target.SessionID.t,
};

let enable_events = t => {
  open Cdp.Commands;
  let sessionId = t.sessionId;

  let%lwt _ = Page.Enable.Request.make(~sessionId) |> Websocket.send;
  let%lwt _ = DOM.Enable.Request.make(~sessionId) |> Websocket.send;
  let%lwt _ =
    Page.SetLifecycleEventsEnabled.(
      Request.make(~sessionId, ~params=Params.make(~enabled=true, ()))
      |> Websocket.send
    );
  Lwt.return();
};

let make = browser => {
  let%lwt targetId =
    Cdp.Commands.Target.CreateTarget.(
      Request.make(
        ~sessionId=?None,
        ~params=
          Params.make(
            ~url="about:blank",
            ~browserContextId=browser.browserContextId,
            ~newWindow=true,
            (),
          ),
      )
      |> Websocket.send
      |> Lwt.map(Response.parse)
      |> Lwt.map(response => response.Response.result.targetId)
    );

  let%lwt sessionId =
    Cdp.Commands.Target.AttachToTarget.(
      Request.make(
        ~sessionId=?None,
        ~params=Params.make(~targetId, ~flatten=true, ()),
      )
      |> Websocket.send
      |> Lwt.map(Response.parse)
      |> Lwt.map(response => response.Response.result.sessionId)
    );

  let t = {targetId, sessionId};

  let%lwt () = enable_events(t);

  Lwt.return(t);
};
