open Cdp
open OSnap_Browser_Target

let ( let*? ) = Result.bind

let wait_for ~clock ?timeout ?look_behind ~event target =
  let sessionId = target.sessionId in
  let p, resolver = Eio.Promise.create () in
  let callback data remove =
    remove ();
    Eio.Promise.resolve resolver (`Data data)
  in
  OSnap_Websocket.listen ~event ?look_behind ~sessionId callback;
  (match timeout with
   | None -> ()
   | Some t ->
     Eio.Switch.run
     @@ fun sw ->
     Eio.Fiber.fork ~sw
     @@ fun () ->
     Eio.Time.sleep clock (t /. 1000.);
     Eio.Promise.resolve resolver `Timeout);
  p
;;

let get_document target =
  let sessionId = target.sessionId in
  let open Commands.DOM.GetDocument in
  Request.make ~sessionId ~params:(Params.make ())
  |> OSnap_Websocket.send
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
;;

let select_element_all ~document ~selector ~sessionId =
  let open Commands.DOM.QuerySelectorAll in
  Request.make
    ~sessionId
    ~params:
      (Params.make
         ~nodeId:document.Commands.DOM.GetDocument.Response.root.nodeId
         ~selector
         ())
  |> OSnap_Websocket.send
  |> Eio.Promise.await
  |> Response.parse
  |> fun response ->
  match response.Response.error, response.Response.result with
  | _, Some { nodeIds = [] } ->
    Result.error
      (`OSnap_CDP_Protocol_Error
        (Printf.sprintf "No node with the selector %S could not be found." selector))
  | None, None -> Result.error (`OSnap_CDP_Protocol_Error "")
  | Some { message; _ }, None -> Result.error (`OSnap_CDP_Protocol_Error message)
  | Some _, Some result | None, Some result -> Result.ok result
;;

let select_element ~document ~selector ~sessionId =
  let open Commands.DOM.QuerySelector in
  Request.make
    ~sessionId
    ~params:
      (Params.make
         ~nodeId:document.Commands.DOM.GetDocument.Response.root.nodeId
         ~selector
         ())
  |> OSnap_Websocket.send
  |> Eio.Promise.await
  |> Response.parse
  |> fun response ->
  match response.Response.error, response.Response.result with
  | _, (Some { nodeId = `Int 0 } | Some { nodeId = `Float 0. }) ->
    Result.error (`OSnap_Selector_Not_Found selector)
  | None, None -> Result.error (`OSnap_CDP_Protocol_Error "")
  | Some { message; _ }, None -> Result.error (`OSnap_CDP_Protocol_Error message)
  | Some _, Some result | None, Some result -> Result.ok result
;;

let wait_for_network_idle target ~loaderId =
  let open Events.Page in
  let sessionId = target.sessionId in
  let p, resolver = Eio.Promise.create () in
  OSnap_Websocket.listen ~event:LifecycleEvent.name ~sessionId (fun response remove ->
    let eventData = LifecycleEvent.parse response in
    if eventData.params.name = "networkIdle" && loaderId = eventData.params.loaderId
    then (
      remove ();
      Eio.Promise.resolve resolver ()));
  Eio.Promise.await p
;;

let go_to ~url target =
  let open Commands.Page in
  let sessionId = target.sessionId in
  let params = Navigate.Params.make ~url () in
  let*? result =
    let open Navigate in
    Request.make ~sessionId ~params
    |> OSnap_Websocket.send
    |> Eio.Promise.await
    |> Navigate.Response.parse
    |> fun response ->
    let error =
      response.Response.error
      |> Option.map (fun (error : Response.error) ->
           `OSnap_CDP_Protocol_Error error.message)
      |> Option.value ~default:(`OSnap_CDP_Protocol_Error "")
    in
    Option.to_result response.Response.result ~none:error
  in
  match result.errorText, result.loaderId with
  | Some error, _ -> `OSnap_CDP_Protocol_Error error |> Result.error
  | None, None ->
    Result.error (`OSnap_CDP_Protocol_Error "CDP responded with no loader id")
  | None, Some loaderId -> loaderId |> Result.ok
;;

let type_text ~clock ~document ~selector ~text target =
  let open Commands.DOM in
  let sessionId = target.sessionId in
  let*? node = select_element ~document ~selector ~sessionId in
  let*? () =
    let open Focus in
    Request.make ~sessionId ~params:(Params.make ~nodeId:node.nodeId ())
    |> OSnap_Websocket.send
    |> Eio.Promise.await
    |> Response.parse
    |> (fun response ->
         let error =
           response.Response.error
           |> Option.map (fun (error : Response.error) ->
                `OSnap_CDP_Protocol_Error error.message)
           |> Option.value ~default:(`OSnap_CDP_Protocol_Error "")
         in
         Option.to_result response.Response.result ~none:error)
    |> Result.map ignore
  in
  let () =
    List.init (String.length text) (String.get text)
    |> List.iter (fun char ->
         let definition =
           (OSnap_Browser_KeyDefinition.make char : OSnap_Browser_KeyDefinition.t option)
         in
         match definition with
         | Some def ->
           let open Commands.Input.DispatchKeyEvent in
           let () =
             Request.make
               ~sessionId
               ~params:
                 (Params.make
                    ~type_:`keyDown
                    ~windowsVirtualKeyCode:
                      (def.keyCode
                       |> Option.map (fun i -> `Int i)
                       |> Option.value ~default:(`Int 0))
                    ~key:def.key
                    ~code:def.code
                    ~text:def.text
                    ~unmodifiedText:def.text
                    ~location:(`Int def.location)
                    ~isKeypad:(def.location = 3)
                    ())
             |> OSnap_Websocket.send
             |> Eio.Promise.await
             |> ignore
           in
           Request.make
             ~sessionId
             ~params:
               (Params.make
                  ~type_:`keyUp
                  ~key:def.key
                  ~code:def.code
                  ~location:(`Int def.location)
                  ())
           |> OSnap_Websocket.send
           |> Eio.Promise.await
           |> ignore
         | None -> ())
  in
  let wait_result =
    wait_for ~clock ~event:"Page.frameNavigated" ~look_behind:false ~timeout:1000. target
    |> Eio.Promise.await
  in
  match wait_result with
  | `Timeout -> Result.ok ()
  | `Data data ->
    let event_data = Cdp.Events.Page.FrameNavigated.parse data in
    let loaderId = event_data.params.frame.loaderId in
    wait_for_network_idle target ~loaderId |> Result.ok
;;

let get_quads_all ~document ~selector target =
  let open Commands.DOM in
  let sessionId = target.sessionId in
  let to_float = function
    | `Float f -> f
    | `Int i -> float_of_int i
  in
  let*? { nodeIds } = select_element_all ~document ~selector ~sessionId in
  nodeIds
  |> List.fold_left
       (fun acc nodeId ->
         let open GetContentQuads in
         Request.make ~sessionId ~params:(Params.make ~nodeId ())
         |> OSnap_Websocket.send
         |> Eio.Promise.await
         |> Response.parse
         |> fun response ->
         match response.Response.error, response.Response.result with
         | ( (None | Some _)
           , Some { quads = (x1 :: y1 :: x2 :: _y2 :: _x3 :: y2 :: _x4 :: _y4 :: _) :: _ }
           ) -> ((to_float x1, to_float y1), (to_float x2, to_float y2)) :: acc
         | _ -> acc)
       []
  |> Result.ok
;;

let get_quads ~document ~selector target =
  let open Commands.DOM in
  let sessionId = target.sessionId in
  let*? { nodeId } = select_element ~document ~selector ~sessionId in
  let*? result =
    let open GetContentQuads in
    Request.make ~sessionId ~params:(Params.make ~nodeId ())
    |> OSnap_Websocket.send
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
  let to_float = function
    | `Float f -> f
    | `Int i -> float_of_int i
  in
  match result.quads with
  | (x1 :: y1 :: x2 :: _y2 :: _x3 :: y2 :: _x4 :: _y4 :: _) :: _ ->
    Result.ok ((to_float x1, to_float y1), (to_float x2, to_float y2))
  | _ -> Result.error (`OSnap_Selector_Not_Visible selector)
;;

let mousemove ~document ~to_ target =
  let open Commands.Input in
  let sessionId = target.sessionId in
  let*? x, y =
    match to_ with
    | `Selector selector ->
      let*? (x1, y1), (x2, y2) = get_quads ~document ~selector target in
      let x = `Float (x1 +. ((x2 -. x1) /. 2.0)) in
      let y = `Float (y1 +. ((y2 -. y1) /. 2.0)) in
      Result.ok (x, y)
    | `Coordinates (x, y) -> Result.ok (x, y)
  in
  let open DispatchMouseEvent in
  Request.make ~sessionId ~params:(Params.make ~x ~y ~type_:`mouseMoved ())
  |> OSnap_Websocket.send
  |> Eio.Promise.await
  |> ignore
  |> Result.ok
;;

let click ~clock ~document ~selector target =
  let open Commands.Input in
  let sessionId = target.sessionId in
  let*? (x1, y1), (x2, y2) = get_quads ~document ~selector target in
  let x = `Float (x1 +. ((x2 -. x1) /. 2.0)) in
  let y = `Float (y1 +. ((y2 -. y1) /. 2.0)) in
  let*? () = mousemove ~document ~to_:(`Coordinates (x, y)) target in
  let _ =
    let open DispatchMouseEvent in
    Request.make
      ~sessionId
      ~params:
        (Params.make
           ~type_:`mousePressed
           ~button:`left
           ~buttons:(`Int 1)
           ~clickCount:(`Int 1)
           ~x
           ~y
           ())
    |> OSnap_Websocket.send
    |> Eio.Promise.await
    |> ignore
  in
  let () =
    let open DispatchMouseEvent in
    Request.make
      ~sessionId
      ~params:
        (Params.make
           ~type_:`mouseReleased
           ~button:`left
           ~buttons:(`Int 1)
           ~clickCount:(`Int 1)
           ~x
           ~y
           ())
    |> OSnap_Websocket.send
    |> Eio.Promise.await
    |> ignore
  in
  let wait_result =
    wait_for ~clock ~event:"Page.frameNavigated" ~look_behind:false ~timeout:1000. target
    |> Eio.Promise.await
  in
  match wait_result with
  | `Timeout -> Result.ok ()
  | `Data data ->
    let event_data = Cdp.Events.Page.FrameNavigated.parse data in
    let loaderId = event_data.params.frame.loaderId in
    wait_for_network_idle target ~loaderId |> Result.ok
;;

let scroll ~clock ~document ~selector ~px target =
  let sessionId = target.sessionId in
  match px, selector with
  | None, None -> assert false
  | Some _, Some _ -> assert false
  | None, Some selector ->
    let*? { nodeId } = select_element ~document ~selector ~sessionId in
    let open Commands.DOM.ScrollIntoViewIfNeeded in
    Request.make ~sessionId ~params:(Params.make ~nodeId ())
    |> OSnap_Websocket.send
    |> Eio.Promise.await
    |> Response.parse
    |> fun response ->
    (match response.Response.error with
     | None -> Result.ok ()
     | Some { message; _ } -> Result.error (`OSnap_CDP_Protocol_Error message))
  | Some px, None ->
    let expression =
      Printf.sprintf
        {|
          window.scrollTo({
            top: %i,
            left: 0,
            behavior: 'smooth'
          });
        |}
        px
    in
    let open Commands.Runtime.Evaluate in
    let response =
      Request.make ~sessionId ~params:(Params.make ~expression ())
      |> OSnap_Websocket.send
      |> Eio.Promise.await
      |> Response.parse
    in
    (match response.Response.error with
     | None ->
       let timeout = float_of_int (px / 200) in
       Eio.Time.sleep clock timeout |> Result.ok
     | Some { message; _ } -> Result.error (`OSnap_CDP_Protocol_Error message))
;;

let get_content_size target =
  let open Commands.Page in
  let sessionId = target.sessionId in
  let*? metrics =
    let open GetLayoutMetrics in
    Request.make ~sessionId
    |> OSnap_Websocket.send
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
  Result.ok (metrics.cssContentSize.width, metrics.cssContentSize.height)
;;

let set_size ~width ~height target =
  let open Commands.Emulation in
  let sessionId = target.sessionId in
  let*? _ =
    let open SetDeviceMetricsOverride in
    Request.make
      ~sessionId
      ~params:(Params.make ~width ~height ~deviceScaleFactor:(`Int 1) ~mobile:false ())
    |> OSnap_Websocket.send
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

let screenshot ?(full_size = false) target =
  let open Commands.Page in
  let sessionId = target.sessionId in
  let*? () =
    if full_size
    then
      let*? width, height = get_content_size target in
      set_size ~width ~height target
    else Result.ok ()
  in
  let*? result =
    let open CaptureScreenshot in
    Request.make
      ~sessionId
      ~params:(Params.make ~format:`png ~captureBeyondViewport:false ~fromSurface:true ())
    |> OSnap_Websocket.send
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
  Result.ok result.data
;;

let clear_cookies target =
  let open Commands.Storage in
  let sessionId = target.sessionId in
  let*? _ =
    let open ClearCookies in
    Request.make ~sessionId ~params:(Params.make ())
    |> OSnap_Websocket.send
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
