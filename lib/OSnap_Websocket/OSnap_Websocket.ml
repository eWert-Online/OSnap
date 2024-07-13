open OSnap_Utils

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

let websocket_handler ~sw u wsd =
  let send_payload payload =
    let payload = Bytes.of_string payload in
    let len = Bytes.length payload in
    Httpun_ws.Wsd.send_bytes wsd ~kind:`Text payload ~off:0 ~len
  in
  let rec input_loop () =
    let () = Eio.Fiber.yield () in
    if not (Queue.is_empty pending_requests)
    then (
      let key, message, resolver = Queue.take pending_requests in
      let () = send_payload message in
      Hashtbl.add sent_requests key resolver;
      input_loop ())
    else input_loop ()
  in
  let frame ~(opcode : Httpun_ws.Websocket.Opcode.t) ~is_fin:_ ~len payload =
    match opcode with
    | `Connection_close | `Continuation | `Other _ -> ()
    | `Ping -> Httpun_ws.Wsd.send_pong wsd
    | `Pong -> ()
    | `Text | `Binary ->
      let buf = Eio.Buf_write.create len in
      let on_eof () =
        let response = Eio.Buf_write.serialize_to_string buf in
        let json = response |> Yojson.Basic.from_string in
        let id =
          json |> Yojson.Basic.Util.member "id" |> Yojson.Basic.Util.to_int_option
        in
        let method_ =
          json |> Yojson.Basic.Util.member "method" |> Yojson.Basic.Util.to_string_option
        in
        let sessionId =
          json
          |> Yojson.Basic.Util.member "sessionId"
          |> Yojson.Basic.Util.to_string_option
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
        match id with
        | None -> ()
        | Some key ->
          Hashtbl.find_opt sent_requests key
          |> Option.iter (fun resolver -> Eio.Promise.resolve resolver response);
          Hashtbl.remove sent_requests key;
          ()
      in
      let rec on_read bs ~off ~len:read_len =
        let response = Bigstringaf.substring ~off ~len:read_len bs in
        Eio.Buf_write.string buf response;
        Eio.Fiber.yield ();
        if len > read_len
        then Httpun_ws.Payload.schedule_read payload ~on_read ~on_eof
        else on_eof ()
      in
      Httpun_ws.Payload.schedule_read payload ~on_read ~on_eof
  in
  Eio.Fiber.fork_daemon ~sw input_loop;
  let eof () = Eio.Promise.resolve u () in
  Httpun_ws.Websocket_connection.{ frame; eof }
;;

let error_handler = function
  | `Handshake_failure (rsp, _body) ->
    Format.eprintf "Handshake failure: %a\n%!" Httpun.Response.pp_hum rsp
  | _ -> assert false
;;

let connect ~sw ~env url =
  let net = Eio.Stdenv.net env in
  Eio.Fiber.fork_daemon ~sw (fun () ->
    let uri = Uri.of_string url in
    let host = Uri.host_with_default uri ~default:"localhost" in
    let port = Uri.port uri |> Option.value ~default:80 in
    let addresses =
      Eio.Net.getaddrinfo net ~service:"http" host
      |> List.filter_map
         @@ function
         | `Tcp (addr, _port) ->
           let addr = Eio.Net.Ipaddr.fold ~v4:Option.some ~v6:(Fun.const None) addr in
           Option.map (fun addr -> `Tcp (addr, port)) addr
         | _ -> None
    in
    let socket = Eio.Net.connect ~sw net (List.hd addresses) in
    let p, u = Eio.Promise.create () in
    let nonce = "0123456789ABCDEF" in
    let resource = Uri.path_and_query uri in
    let _client =
      Httpun_ws_eio.Client.connect
        socket
        ~sw
        ~nonce
        ~host
        ~port
        ~resource
        ~error_handler
        ~websocket_handler:(websocket_handler ~sw u)
    in
    Eio.Promise.await p;
    `Stop_daemon)
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
