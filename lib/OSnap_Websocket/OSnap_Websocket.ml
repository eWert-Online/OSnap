open OSnap_Utils
open Lwt.Syntax

let id = ref 0

let id () =
  incr id;
  !id
;;

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
  let send_payload payload = Websocket.Frame.create ~content:payload () |> send in
  let rec input_loop () =
    let* () = Lwt.pause () in
    if not (Queue.is_empty pending_requests)
    then (
      let key, message, resolver = Queue.take pending_requests in
      let* () = send_payload message in
      Hashtbl.add sent_requests key resolver;
      input_loop ())
    else input_loop ()
  in
  let react (frame : Websocket.Frame.t) =
    match frame.opcode with
    | Close | Continuation | Ctrl _ | Nonctrl _ -> close ()
    | Ping -> Websocket.Frame.create ~opcode:Pong () |> send
    | Pong -> Lwt.return ()
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
       | None -> Lwt.return ()
       | Some key ->
         Hashtbl.find_opt sent_requests key
         |> Option.iter (fun resolver -> Lwt.wakeup_later resolver response);
         Hashtbl.remove sent_requests key;
         Lwt.return ())
  in
  let rec react_forever () =
    let* frame = recv () in
    let* () = react frame in
    react_forever ()
  in
  Lwt.pick [ input_loop (); react_forever () ]
;;

let connect ~sw ~env:_ url =
  Result.ok
  @@ Eio.Fiber.fork_daemon ~sw
  @@ fun () ->
  let () =
    Lwt_eio.run_lwt
    @@ fun () ->
    let orig_uri = Uri.of_string url in
    let uri = Uri.with_scheme orig_uri (Some "http") in
    let* endpoint = Resolver_lwt.resolve_uri ~uri Resolver_lwt_unix.system in
    let default_context = Lazy.force Conduit_lwt_unix.default_ctx in
    let* client = endpoint |> Conduit_lwt_unix.endp_to_client ~ctx:default_context in
    let* conn = Websocket_lwt_unix.connect ~ctx:default_context client uri in
    let recv () = Websocket_lwt_unix.read conn in
    let send = Websocket_lwt_unix.write conn in
    websocket_handler recv send
  in
  `Stop_daemon
;;

let send message =
  Lwt_eio.run_lwt
  @@ fun () ->
  let key = id () in
  let message = message key in
  let p, resolver = Lwt.wait () in
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
