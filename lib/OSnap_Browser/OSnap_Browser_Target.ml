open OSnap_Browser_Types
module Websocket = OSnap_Websocket

type target =
  { targetId : Cdp.Types.Target.TargetID.t
  ; sessionId : Cdp.Types.Target.SessionID.t
  }

let enable_events t =
  let ( let*? ) = Result.bind in
  let open Cdp.Commands in
  let sessionId = t.sessionId in
  let*? _ =
    let open Page.Enable in
    let response = Request.make ~sessionId |> Websocket.send |> Response.parse in
    let error =
      response.Response.error
      |> Option.map (fun (error : Response.error) ->
        `OSnap_CDP_Protocol_Error error.message)
      |> Option.value ~default:(`OSnap_CDP_Protocol_Error "")
    in
    Option.to_result response.Response.result ~none:error
  in
  let*? _ =
    let open DOM.Enable in
    let response =
      Request.make ~sessionId ~params:(Params.make ()) |> Websocket.send |> Response.parse
    in
    let error =
      response.Response.error
      |> Option.map (fun (error : Response.error) ->
        `OSnap_CDP_Protocol_Error error.message)
      |> Option.value ~default:(`OSnap_CDP_Protocol_Error "")
    in
    Option.to_result response.Response.result ~none:error
  in
  let*? _ =
    let open CSS.Enable in
    let response = Request.make ~sessionId |> Websocket.send |> Response.parse in
    let error =
      response.Response.error
      |> Option.map (fun (error : Response.error) ->
        `OSnap_CDP_Protocol_Error error.message)
      |> Option.value ~default:(`OSnap_CDP_Protocol_Error "")
    in
    Option.to_result response.Response.result ~none:error
  in
  let*? _ =
    let open Network.Enable in
    let params = Params.make () in
    let response = Request.make ~sessionId ~params |> Websocket.send |> Response.parse in
    let error =
      response.Response.error
      |> Option.map (fun (error : Response.error) ->
        `OSnap_CDP_Protocol_Error error.message)
      |> Option.value ~default:(`OSnap_CDP_Protocol_Error "")
    in
    Option.to_result response.Response.result ~none:error
  in
  let*? _ =
    let open Page.SetLifecycleEventsEnabled in
    let response =
      Request.make ~sessionId ~params:(Params.make ~enabled:true ())
      |> Websocket.send
      |> Response.parse
    in
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
  let ( let*? ) = Result.bind in
  let*? { targetId } =
    let open Cdp.Commands.Target.CreateTarget in
    let response =
      Request.make
        ?sessionId:None
        ~params:
          (Params.make
             ~url:"about:blank"
             ~browserContextId:browser.browserContextId
             ~newWindow:true
             ())
      |> Websocket.send
      |> Response.parse
    in
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
    let response =
      Request.make ?sessionId:None ~params:(Params.make ~targetId ~flatten:true ())
      |> Websocket.send
      |> Response.parse
    in
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
