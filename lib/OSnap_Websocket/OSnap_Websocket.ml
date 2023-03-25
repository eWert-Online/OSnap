open OSnap_Utils

let id = ref 0

let id () =
  incr id;
  !id
;;

let close_requests = Queue.create ()
let pending_requests = Queue.create ()
let sent_requests = Hashtbl.create 10
let listeners = Hashtbl.create 10
let events = Hashtbl.create 1000

let call_event_handlers key message =
  List.iter (fun handler ->
    handler message (fun () ->
      Hashtbl.remove listeners key;
      Hashtbl.remove events key))
;;

let websocket_handler recv send =
  let close () = Websocket.Frame.close 1002 |> send in
  let send_payload payload =
    let frame = Websocket.Frame.create ~content:payload () in
    send frame
  in
  let rec input_loop () =
    let () = Eio.Fiber.yield () in
    if not (Queue.is_empty close_requests)
    then (
      close_requests |> Queue.iter (fun resolver -> Eio.Promise.resolve resolver ());
      close_requests |> Queue.clear;
      close ())
    else if not (Queue.is_empty pending_requests)
    then (
      let key, message, resolver = Queue.take pending_requests in
      let () = send_payload message in
      Hashtbl.add sent_requests key resolver;
      input_loop ())
    else input_loop ()
  in
  let react (frame : Websocket.Frame.t) =
    match frame.opcode with
    | Close | Continuation | Ctrl _ | Nonctrl _ -> close ()
    | Ping -> Websocket.Frame.create ~opcode:Pong () |> send
    | Pong -> ()
    | Text | Binary ->
      let response = frame.Websocket.Frame.content in
      let id =
        response
        |> Yojson.Safe.from_string
        |> Yojson.Safe.Util.member "id"
        |> Yojson.Safe.Util.to_int_option
      in
      let method_ =
        response
        |> Yojson.Safe.from_string
        |> Yojson.Safe.Util.member "method"
        |> Yojson.Safe.Util.to_string_option
      in
      let sessionId =
        response
        |> Yojson.Safe.from_string
        |> Yojson.Safe.Util.member "sessionId"
        |> Yojson.Safe.Util.to_string_option
      in
      (match method_, sessionId with
       | None, None -> ()
       | None, _ -> ()
       | Some method_, None ->
         let key = method_ in
         Hashtbl.add events key response;
         Hashtbl.find_opt listeners key |> Option.iter (call_event_handlers key response)
       | Some method_, Some sessionId ->
         let key = method_ ^ sessionId in
         Hashtbl.add events key response;
         Hashtbl.find_opt listeners key |> Option.iter (call_event_handlers key response));
      (match id with
       | None -> ()
       | Some key ->
         Hashtbl.find_opt sent_requests key
         |> Option.iter (fun resolver -> Eio.Promise.resolve resolver response);
         Hashtbl.remove sent_requests key;
         ())
  in
  let rec react_forever () =
    let frame = recv () in
    let () = react frame in
    react_forever ()
  in
  Eio.Fiber.first input_loop react_forever
;;

let connect url =
  let orig_uri = Uri.of_string url in
  let uri = Uri.with_scheme orig_uri (Some "http") in
  let endpoint =
    Lwt_eio.run_lwt (fun () -> Resolver_lwt.resolve_uri ~uri Resolver_lwt_unix.system)
  in
  let default_context = Lazy.force Conduit_lwt_unix.default_ctx in
  let client =
    Lwt_eio.run_lwt (fun () ->
      endpoint |> Conduit_lwt_unix.endp_to_client ~ctx:default_context)
  in
  let conn =
    Lwt_eio.run_lwt (fun () -> Websocket_lwt_unix.connect ~ctx:default_context client uri)
  in
  let recv () = Lwt_eio.run_lwt (fun () -> Websocket_lwt_unix.read conn) in
  let send frame = Lwt_eio.run_lwt (fun () -> Websocket_lwt_unix.write conn frame) in
  websocket_handler recv send
;;

let send message =
  let key = id () in
  let message = message key in
  let p, resolver = Eio.Promise.create () in
  pending_requests |> Queue.add (key, message, resolver);
  p
;;

let listen ?(look_behind = true) ~event ~sessionId handler =
  let key = event ^ sessionId in
  let stored_listeners = Hashtbl.find_opt listeners key in
  (match stored_listeners with
   | None -> Hashtbl.add listeners key [ handler ]
   | Some stored -> Hashtbl.replace listeners key (handler :: stored));
  if look_behind
  then
    Hashtbl.find_all events key
    |> List.iter (fun event ->
         handler event (fun () ->
           Hashtbl.remove listeners key;
           Hashtbl.remove events key))
;;

let close () =
  let p, resolver = Eio.Promise.create () in
  close_requests |> Queue.add resolver;
  p
;;
