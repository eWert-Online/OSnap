open OSnap_Browser_Types;

module CDP = OSnap_CDP;
module Websocket = OSnap_Websocket;

type target = {
  targetId: OSnap_CDP.Types.Target.TargetId.t,
  sessionId: OSnap_CDP.Types.Target.SessionId.t,
};

let enable_events = t => {
  let sessionId = t.sessionId;

  let%lwt _ = CDP.Page.Enable.make(~sessionId, ()) |> Websocket.send;
  let%lwt _ =
    CDP.Page.SetLifecycleEventsEnabled.make(~sessionId, ~enabled=true)
    |> Websocket.send;
  // let%lwt _ = Performance.Enable.make(~sessionId, ()) |> Websocket.send;
  // let%lwt _ = Log.Enable.make(~sessionId, ()) |> Websocket.send;
  // let%lwt _ = Runtime.Enable.make(~sessionId, ()) |> Websocket.send;
  // let%lwt _ = CDP.Network.Enable.make(~sessionId, ()) |> Websocket.send;
  Lwt.return();
};

let make = browser => {
  let%lwt targetId =
    CDP.Target.CreateTarget.make(
      ~browserContextId=browser.browserContextId,
      ~newWindow=true,
      "about:blank",
    )
    |> Websocket.send
    |> Lwt.map(CDP.Target.CreateTarget.parse)
    |> Lwt.map(response =>
         response.CDP.Types.Response.result.CDP.Target.CreateTarget.targetId
       );

  let%lwt sessionId =
    CDP.Target.AttachToTarget.make(targetId, ~flatten=true)
    |> Websocket.send
    |> Lwt.map(CDP.Target.AttachToTarget.parse)
    |> Lwt.map(response =>
         response.CDP.Types.Response.result.CDP.Target.AttachToTarget.sessionId
       );

  let t = {targetId, sessionId};

  let%lwt () = enable_events(t);

  Lwt.return(t);
};
