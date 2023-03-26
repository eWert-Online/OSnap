open OSnap_Browser_Types
module Websocket = OSnap_Websocket

let ( let*? ) = Result.bind

type target =
  { targetId : Cdp.Types.Target.TargetID.t
  ; sessionId : Cdp.Types.Target.SessionID.t
  }

let enable_events t =
  let open Cdp.Commands in
  let sessionId = t.sessionId in
  let _ =
    let open Page.Enable in
    Request.make ~sessionId
    |> Websocket.send
    |> Eio.Promise.await
    |> Response.parse
    |> fun response ->
    let error =
      response.Response.error
      |> Option.map (fun (error : Response.error) ->
           `OSnap_CDP_Protocol_Error error.message)
      |> Option.value ~default:(`OSnap_CDP_Protocol_Error "")
    in
    Option.to_result response.Response.result ~none:error
  in
  let _ =
    let open DOM.Enable in
    Request.make ~sessionId ~params:(Params.make ())
    |> Websocket.send
    |> Eio.Promise.await
    |> Response.parse
    |> fun response ->
    let error =
      response.Response.error
      |> Option.map (fun (error : Response.error) ->
           `OSnap_CDP_Protocol_Error error.message)
      |> Option.value ~default:(`OSnap_CDP_Protocol_Error "")
    in
    Option.to_result response.Response.result ~none:error
  in
  let _ =
    let open Page.SetLifecycleEventsEnabled in
    Request.make ~sessionId ~params:(Params.make ~enabled:true ())
    |> Websocket.send
    |> Eio.Promise.await
    |> Response.parse
    |> fun response ->
    let error =
      response.Response.error
      |> Option.map (fun (error : Response.error) ->
           `OSnap_CDP_Protocol_Error error.message)
      |> Option.value ~default:(`OSnap_CDP_Protocol_Error "")
    in
    Option.to_result response.Response.result ~none:error
  in
  Result.ok ()
;;

let make browser =
  let*? { targetId } =
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
    |> Eio.Promise.await
    |> Response.parse
    |> fun response ->
    let error =
      response.Response.error
      |> Option.map (fun (error : Response.error) ->
           `OSnap_CDP_Protocol_Error error.message)
      |> Option.value ~default:(`OSnap_CDP_Protocol_Error "")
    in
    Option.to_result response.Response.result ~none:error
  in
  let*? { sessionId } =
    let open Cdp.Commands.Target.AttachToTarget in
    Request.make ?sessionId:None ~params:(Params.make ~targetId ~flatten:true ())
    |> Websocket.send
    |> Eio.Promise.await
    |> Response.parse
    |> fun response ->
    let error =
      response.Response.error
      |> Option.map (fun (error : Response.error) ->
           `OSnap_CDP_Protocol_Error error.message)
      |> Option.value ~default:(`OSnap_CDP_Protocol_Error "")
    in
    Option.to_result response.Response.result ~none:error
  in
  let t = { targetId; sessionId } in
  let*? () = enable_events t in
  Result.ok t
;;
