open OSnap_Browser_Types
module Websocket = OSnap_Websocket

type target =
  { targetId : Cdp.Types.Target.TargetID.t
  ; sessionId : Cdp.Types.Target.SessionID.t
  }

let enable_events t =
  let open Cdp.Commands in
  let open Lwt_result.Syntax in
  let sessionId = t.sessionId in
  let* _ =
    let open Page.Enable in
    Request.make ~sessionId
    |> Websocket.send
    |> Lwt.map Response.parse
    |> Lwt.map (fun response ->
         let error =
           response.Response.error
           |> Option.map (fun (error : Response.error) ->
                OSnap_Response.CDP_Protocol_Error error.message)
           |> Option.value ~default:(OSnap_Response.CDP_Protocol_Error "")
         in
         Option.to_result response.Response.result ~none:error)
  in
  let* _ =
    let open DOM.Enable in
    Request.make ~sessionId ~params:(Params.make ())
    |> Websocket.send
    |> Lwt.map Response.parse
    |> Lwt.map (fun response ->
         let error =
           response.Response.error
           |> Option.map (fun (error : Response.error) ->
                OSnap_Response.CDP_Protocol_Error error.message)
           |> Option.value ~default:(OSnap_Response.CDP_Protocol_Error "")
         in
         Option.to_result response.Response.result ~none:error)
  in
  let* _ =
    let open Page.SetLifecycleEventsEnabled in
    Request.make ~sessionId ~params:(Params.make ~enabled:true ())
    |> Websocket.send
    |> Lwt.map Response.parse
    |> Lwt.map (fun response ->
         let error =
           response.Response.error
           |> Option.map (fun (error : Response.error) ->
                OSnap_Response.CDP_Protocol_Error error.message)
           |> Option.value ~default:(OSnap_Response.CDP_Protocol_Error "")
         in
         Option.to_result response.Response.result ~none:error)
  in
  Lwt_result.return ()
;;

let make browser =
  let open Lwt_result.Syntax in
  let* { targetId } =
    let open Cdp.Commands.Target.CreateTarget in
    Request.make
      ?sessionId:None
      ~params:
        (Params.make
           ~url:"about:blank"
           ~browserContextId:browser.browserContextId
           ~newWindow:true
           ())
    |> Websocket.send
    |> Lwt.map Response.parse
    |> Lwt.map (fun response ->
         let error =
           response.Response.error
           |> Option.map (fun (error : Response.error) ->
                OSnap_Response.CDP_Protocol_Error error.message)
           |> Option.value ~default:(OSnap_Response.CDP_Protocol_Error "")
         in
         Option.to_result response.Response.result ~none:error)
  in
  let* { sessionId } =
    let open Cdp.Commands.Target.AttachToTarget in
    Request.make ?sessionId:None ~params:(Params.make ~targetId ~flatten:true ())
    |> Websocket.send
    |> Lwt.map Response.parse
    |> Lwt.map (fun response ->
         let error =
           response.Response.error
           |> Option.map (fun (error : Response.error) ->
                OSnap_Response.CDP_Protocol_Error error.message)
           |> Option.value ~default:(OSnap_Response.CDP_Protocol_Error "")
         in
         Option.to_result response.Response.result ~none:error)
  in
  let t = { targetId; sessionId } in
  let* () = enable_events t in
  Lwt_result.return t
;;
