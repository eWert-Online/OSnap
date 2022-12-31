open Cdp
open OSnap_Browser_Target
open Lwt_result.Syntax

let wait_for ?timeout ?look_behind ~event target =
  let sessionId = target.sessionId in
  let p, resolver = Lwt.wait () in
  let callback data remove =
    remove ();
    Lwt.wakeup_later resolver (`Data data)
  in
  OSnap_Websocket.listen ~event ?look_behind ~sessionId callback;
  match timeout with
  | None -> p
  | Some t ->
    let timeout = Lwt_unix.sleep (t /. 1000.) |> Lwt.map (fun () -> `Timeout) in
    Lwt.pick [ timeout; p ]
;;

let get_document target =
  let sessionId = target.sessionId in
  let open Commands.DOM.GetDocument in
  Request.make ~sessionId ~params:(Params.make ())
  |> OSnap_Websocket.send
  |> Lwt.map Response.parse
  |> Lwt.map (fun response ->
       let error =
         response.Response.error
         |> Option.map (fun (error : Response.error) ->
              OSnap_Response.CDP_Protocol_Error error.message)
         |> Option.value ~default:(OSnap_Response.CDP_Protocol_Error "")
       in
       Option.to_result response.Response.result ~none:error)
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
  |> Lwt.map Response.parse
  |> Lwt.map (fun response ->
       match response.Response.error, response.Response.result with
       | _, Some { nodeIds = [] } ->
         Result.error
           (OSnap_Response.CDP_Protocol_Error
              (Printf.sprintf "No node with the selector %S could not be found." selector))
       | None, None -> Result.error (OSnap_Response.CDP_Protocol_Error "")
       | Some { message; _ }, None ->
         Result.error (OSnap_Response.CDP_Protocol_Error message)
       | Some _, Some result | None, Some result -> Result.ok result)
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
  |> Lwt.map Response.parse
  |> Lwt.map (fun response ->
       match response.Response.error, response.Response.result with
       | _, (Some { nodeId = `Int 0 } | Some { nodeId = `Float 0. }) ->
         Result.error
           (OSnap_Response.CDP_Protocol_Error
              (Printf.sprintf "A node with the selector %S could not be found." selector))
       | None, None -> Result.error (OSnap_Response.CDP_Protocol_Error "")
       | Some { message; _ }, None ->
         Result.error (OSnap_Response.CDP_Protocol_Error message)
       | Some _, Some result | None, Some result -> Result.ok result)
;;

let wait_for_network_idle target ~loaderId =
  let open Events.Page in
  let sessionId = target.sessionId in
  let p, resolver = Lwt.wait () in
  OSnap_Websocket.listen ~event:LifecycleEvent.name ~sessionId (fun response remove ->
    let eventData = LifecycleEvent.parse response in
    if eventData.params.name = "networkIdle" && loaderId = eventData.params.loaderId
    then (
      remove ();
      Lwt.wakeup_later resolver ()));
  p
;;

let go_to ~url target =
  let open Commands.Page in
  let sessionId = target.sessionId in
  let debug = OSnap_Logger.debug ~header:"Browser.go_to" in
  debug (Printf.sprintf "session %S navigationg to %S" sessionId url);
  let params = Navigate.Params.make ~url () in
  let* result =
    let open Navigate in
    Request.make ~sessionId ~params
    |> OSnap_Websocket.send
    |> Lwt.map Navigate.Response.parse
    |> Lwt.map (fun response ->
         let error =
           response.Response.error
           |> Option.map (fun (error : Response.error) ->
                OSnap_Response.CDP_Protocol_Error error.message)
           |> Option.value ~default:(OSnap_Response.CDP_Protocol_Error "")
         in
         Option.to_result response.Response.result ~none:error)
  in
  match result.errorText, result.loaderId with
  | Some error, _ -> OSnap_Response.CDP_Protocol_Error error |> Lwt_result.fail
  | None, None ->
    Lwt_result.fail (OSnap_Response.CDP_Protocol_Error "CDP responded with no loader id")
  | None, Some loaderId -> loaderId |> Lwt_result.return
;;

let type_text ~document ~selector ~text target =
  let open Commands.DOM in
  let sessionId = target.sessionId in
  let* node = select_element ~document ~selector ~sessionId in
  let* () =
    let open Focus in
    Request.make ~sessionId ~params:(Params.make ~nodeId:node.nodeId ())
    |> OSnap_Websocket.send
    |> Lwt.map Response.parse
    |> Lwt.map (fun response ->
         let error =
           response.Response.error
           |> Option.map (fun (error : Response.error) ->
                OSnap_Response.CDP_Protocol_Error error.message)
           |> Option.value ~default:(OSnap_Response.CDP_Protocol_Error "")
         in
         Option.to_result response.Response.result ~none:error)
    |> Lwt_result.map ignore
  in
  let* () =
    List.init (String.length text) (String.get text)
    |> Lwt_list.iter_s (fun char ->
         let definition =
           (OSnap_Browser_KeyDefinition.make char : OSnap_Browser_KeyDefinition.t option)
         in
         match definition with
         | Some def ->
           [%lwt
             let () =
               let open Commands.Input.DispatchKeyEvent in
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
               |> Lwt.map ignore
             in
             let open Commands.Input.DispatchKeyEvent in
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
             |> Lwt.map ignore]
         | None -> Lwt.return ())
    |> Lwt_result.ok
  in
  let* wait_result =
    wait_for ~event:"Page.frameNavigated" ~look_behind:false ~timeout:1000. target
    |> Lwt_result.ok
  in
  match wait_result with
  | `Timeout -> Lwt_result.return ()
  | `Data data ->
    let event_data = Cdp.Events.Page.FrameNavigated.parse data in
    let loaderId = event_data.params.frame.loaderId in
    wait_for_network_idle target ~loaderId |> Lwt_result.ok
;;

let get_quads_all ~document ~selector target =
  let open Commands.DOM in
  let sessionId = target.sessionId in
  let to_float = function
    | `Float f -> f
    | `Int i -> float_of_int i
  in
  let* { nodeIds } = select_element_all ~document ~selector ~sessionId in
  nodeIds
  |> Lwt_list.fold_left_s
       (fun acc nodeId ->
         let open GetContentQuads in
         Request.make ~sessionId ~params:(Params.make ~nodeId ())
         |> OSnap_Websocket.send
         |> Lwt.map Response.parse
         |> Lwt.map (fun response ->
              match response.Response.error, response.Response.result with
              | ( (None | Some _)
                , Some
                    { quads = (x1 :: y1 :: x2 :: _y2 :: _x3 :: y2 :: _x4 :: _y4 :: _) :: _
                    } ) -> ((to_float x1, to_float y1), (to_float x2, to_float y2)) :: acc
              | _ -> acc))
       []
  |> Lwt_result.ok
;;

let get_quads ~document ~selector target =
  let open Commands.DOM in
  let sessionId = target.sessionId in
  let* { nodeId } = select_element ~document ~selector ~sessionId in
  let* result =
    let open GetContentQuads in
    Request.make ~sessionId ~params:(Params.make ~nodeId ())
    |> OSnap_Websocket.send
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
  let to_float = function
    | `Float f -> f
    | `Int i -> float_of_int i
  in
  match result.quads with
  | (x1 :: y1 :: x2 :: _y2 :: _x3 :: y2 :: _x4 :: _y4 :: _) :: _ ->
    Lwt_result.return ((to_float x1, to_float y1), (to_float x2, to_float y2))
  | _ -> Lwt_result.fail (OSnap_Response.CDP_Protocol_Error "no content quads returned")
;;

let mousemove ~document ~to_ target =
  let open Commands.Input in
  let sessionId = target.sessionId in
  let* x, y =
    match to_ with
    | `Selector selector ->
      let* (x1, y1), (x2, y2) = get_quads ~document ~selector target in
      let x = `Float (x1 +. ((x2 -. x1) /. 2.0)) in
      let y = `Float (y1 +. ((y2 -. y1) /. 2.0)) in
      Lwt_result.return (x, y)
    | `Coordinates (x, y) -> Lwt_result.return (x, y)
  in
  let open DispatchMouseEvent in
  Request.make ~sessionId ~params:(Params.make ~x ~y ~type_:`mouseMoved ())
  |> OSnap_Websocket.send
  |> Lwt.map ignore
  |> Lwt_result.ok
;;

let click ~document ~selector target =
  let open Commands.Input in
  let sessionId = target.sessionId in
  let* (x1, y1), (x2, y2) = get_quads ~document ~selector target in
  let x = `Float (x1 +. ((x2 -. x1) /. 2.0)) in
  let y = `Float (y1 +. ((y2 -. y1) /. 2.0)) in
  let* () = mousemove ~document ~to_:(`Coordinates (x, y)) target in
  let* _ =
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
    |> Lwt.map ignore
    |> Lwt_result.ok
  in
  let* () =
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
    |> Lwt.map ignore
    |> Lwt_result.ok
  in
  let* wait_result =
    wait_for ~event:"Page.frameNavigated" ~look_behind:false ~timeout:1000. target
    |> Lwt_result.ok
  in
  match wait_result with
  | `Timeout -> Lwt_result.return ()
  | `Data data ->
    let event_data = Cdp.Events.Page.FrameNavigated.parse data in
    let loaderId = event_data.params.frame.loaderId in
    wait_for_network_idle target ~loaderId |> Lwt_result.ok
;;

let scroll ~document ~selector ~px target =
  let sessionId = target.sessionId in
  match px, selector with
  | None, None -> assert false
  | Some _, Some _ -> assert false
  | None, Some selector ->
    let* { nodeId } = select_element ~document ~selector ~sessionId in
    let open Commands.DOM.ScrollIntoViewIfNeeded in
    Request.make ~sessionId ~params:(Params.make ~nodeId ())
    |> OSnap_Websocket.send
    |> Lwt.map Response.parse
    |> Lwt.map (fun response ->
         match response.Response.error with
         | None -> Result.ok ()
         | Some { message; _ } -> Result.error (OSnap_Response.CDP_Protocol_Error message))
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
    Request.make ~sessionId ~params:(Params.make ~expression ())
    |> OSnap_Websocket.send
    |> Lwt.map Response.parse
    |> fun __x ->
    Lwt.bind __x (fun response ->
      match response.Response.error with
      | None ->
        let timeout = float_of_int (px / 200) in
        Lwt_unix.sleep timeout |> Lwt_result.ok
      | Some { message; _ } -> Lwt_result.fail (OSnap_Response.CDP_Protocol_Error message))
;;

let get_content_size target =
  let open Commands.Page in
  let sessionId = target.sessionId in
  let* metrics =
    let open GetLayoutMetrics in
    Request.make ~sessionId
    |> OSnap_Websocket.send
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
  Lwt_result.return (metrics.cssContentSize.width, metrics.cssContentSize.height)
;;

let set_size ~width ~height target =
  let open Commands.Emulation in
  let sessionId = target.sessionId in
  let* _ =
    let open SetDeviceMetricsOverride in
    Request.make
      ~sessionId
      ~params:(Params.make ~width ~height ~deviceScaleFactor:(`Int 1) ~mobile:false ())
    |> OSnap_Websocket.send
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

let screenshot ?(full_size = false) target =
  let open Commands.Page in
  let sessionId = target.sessionId in
  let* () =
    if full_size
    then
      let* width, height = get_content_size target in
      set_size ~width ~height target
    else Lwt_result.return ()
  in
  let* result =
    let open CaptureScreenshot in
    Request.make
      ~sessionId
      ~params:(Params.make ~format:`png ~captureBeyondViewport:false ~fromSurface:true ())
    |> OSnap_Websocket.send
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
  Lwt_result.return result.data
;;

let clear_cookies target =
  let open Commands.Storage in
  let sessionId = target.sessionId in
  let* _ =
    let open ClearCookies in
    Request.make ~sessionId ~params:(Params.make ())
    |> OSnap_Websocket.send
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
