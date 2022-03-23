open OSnap_Browser_Types;

module Websocket = OSnap_Websocket;

type target = {
  targetId: Cdp.Types.Target.TargetID.t,
  sessionId: Cdp.Types.Target.SessionID.t,
};

let enable_events = t => {
  open Cdp.Commands;
  open Lwt_result.Syntax;

  let sessionId = t.sessionId;

  let* _ =
    Page.Enable.(
      Request.make(~sessionId)
      |> Websocket.send
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

  let* _ =
    DOM.Enable.(
      Request.make(~sessionId, ~params=Params.make())
      |> Websocket.send
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

  let* _ =
    Page.SetLifecycleEventsEnabled.(
      Request.make(~sessionId, ~params=Params.make(~enabled=true, ()))
      |> Websocket.send
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

let make = browser => {
  open Lwt_result.Syntax;

  let* {targetId} =
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

  let* {sessionId} =
    Cdp.Commands.Target.AttachToTarget.(
      Request.make(
        ~sessionId=?None,
        ~params=Params.make(~targetId, ~flatten=true, ()),
      )
      |> Websocket.send
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

  let t = {targetId, sessionId};
  let* () = enable_events(t);
  Lwt_result.return(t);
};
