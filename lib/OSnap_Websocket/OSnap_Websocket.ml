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

let _websocket_handler ~sw wsd =
  let close () = Httpun_ws.Wsd.close wsd in
  let send_payload payload =
    let payload = Bytes.of_string payload in
    let len = Bytes.length payload in
    Httpun_ws.Wsd.send_bytes wsd ~kind:`Text payload ~off:0 ~len
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
  let frame ~opcode:_ ~is_fin:_ ~len:_ _payload = () in
  Eio.Fiber.fork ~sw input_loop;
  let eof () = () in
  Httpun_ws.Websocket_connection.{ frame; eof }
;;

let _error_handler = function
  | `Handshake_failure (rsp, _body) ->
    Format.eprintf "Handshake failure: %a\n%!" Httpun.Response.pp_hum rsp
  | _ -> assert false
;;

let connect ~sw ~env url =
  let uri = Uri.of_string url in
  let resource = Uri.path uri in
  let*? client =
    Piaf.Client.create env ~sw (Uri.with_scheme uri (Some "http"))
    |> Result.map_error (fun _ -> `OSnap_CDP_Connection_Failed)
  in
  let*? wsd =
    Piaf.Client.ws_upgrade client resource
    |> Result.map_error (fun _ -> `OSnap_CDP_Connection_Failed)
  in
  let close () = Piaf.Ws.Descriptor.close wsd in
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
      Piaf.Ws.Descriptor.send_string wsd message;
      Hashtbl.add sent_requests key resolver;
      input_loop ())
    else input_loop ()
  in
  Result.ok
  @@ Eio.Fiber.fork ~sw
  @@ fun () ->
  Eio.Fiber.both
    (fun () ->
      input_loop ();
      Piaf.Client.shutdown client)
    (fun () ->
      wsd
      |> Piaf.Ws.Descriptor.messages
      |> Piaf.Stream.iter ~f:(fun (opcode, { Piaf.IOVec.buffer; off; len }) ->
        match opcode with
        | `Connection_close | `Continuation | `Other _ -> close ()
        | `Ping -> Piaf.Ws.Descriptor.send_pong wsd
        | `Pong -> ()
        | `Text | `Binary ->
          let response = Bigstringaf.substring ~off ~len buffer in
          let json = response |> Yojson.Safe.from_string in
          let id =
            json |> Yojson.Safe.Util.member "id" |> Yojson.Safe.Util.to_int_option
          in
          let method_ =
            json |> Yojson.Safe.Util.member "method" |> Yojson.Safe.Util.to_string_option
          in
          let sessionId =
            json
            |> Yojson.Safe.Util.member "sessionId"
            |> Yojson.Safe.Util.to_string_option
          in
          (match method_, sessionId with
           | None, None -> ()
           | None, _ -> ()
           | Some method_, None ->
             let key = method_ in
             Hashtbl.add events key response;
             Hashtbl.find_opt listeners key
             |> Option.iter (call_event_handlers key response)
           | Some method_, Some sessionId ->
             let key = method_ ^ sessionId in
             Hashtbl.add events key response;
             Hashtbl.find_opt listeners key
             |> Option.iter (call_event_handlers key response));
          (match id with
           | None -> ()
           | Some key ->
             Hashtbl.find_opt sent_requests key
             |> Option.iter (fun resolver -> Eio.Promise.resolve resolver response);
             Hashtbl.remove sent_requests key;
             ())))
;;

let send message =
  let key = id () in
  let message = message key in
  let p, resolver = Eio.Promise.create () in
  pending_requests |> Queue.add (key, message, resolver);
  Eio.Promise.await p
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
  Eio.Promise.await p
;;
