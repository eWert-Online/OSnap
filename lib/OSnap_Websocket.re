let close_requests = Queue.create();
let pending_requests = Queue.create();
let sent_requests = Hashtbl.create(10);
let listeners = Hashtbl.create(10);

let websocket_handler = (u, wsd) => {
  let rec input_loop = wsd => {
    let%lwt () = Lwt.pause();
    if (!Queue.is_empty(close_requests)) {
      Websocketaf.Wsd.close(wsd);
      close_requests |> Queue.iter(resolver => Lwt.wakeup_later(resolver, ()));
      close_requests |> Queue.clear;
      Lwt.return_unit;
    } else if (!Queue.is_empty(pending_requests)) {
      let (key, message, resolver) = Queue.take(pending_requests);
      let payload = Bytes.of_string(message);
      Websocketaf.Wsd.send_bytes(
        wsd,
        ~kind=`Text,
        payload,
        ~off=0,
        ~len=Bytes.length(payload),
      );
      Hashtbl.add(sent_requests, key, resolver);
      input_loop(wsd);
    } else {
      input_loop(wsd);
    };
  };
  Lwt.async(() => input_loop(wsd));

  let frame = (~opcode as _, ~is_fin as _, bs, ~off, ~len) => {
    let payload = Bytes.create(len);
    Lwt_bytes.blit_to_bytes(bs, off, payload, 0, len);
    let response = Bytes.to_string(payload);
    print_endline("[SOCKET] < \t" ++ response);
    let id =
      response
      |> Yojson.Safe.from_string
      |> Yojson.Safe.Util.member("id")
      |> Yojson.Safe.Util.to_int_option;

    let method =
      response
      |> Yojson.Safe.from_string
      |> Yojson.Safe.Util.member("method")
      |> Yojson.Safe.Util.to_string_option;

    switch (method) {
    | None => ()
    | Some(method) =>
      Hashtbl.find_opt(listeners, method)
      |> Option.iter(List.iter(handler => handler()));
      Hashtbl.remove(listeners, method);
    };

    switch (id) {
    | None => ()
    | Some(key) =>
      Hashtbl.find_opt(sent_requests, key)
      |> Option.iter(resolver => {Lwt.wakeup_later(resolver, response)});
      Hashtbl.remove(sent_requests, key);
    };
  };

  let eof = () => {
    print_endline("[CLOSING SOCKET]");
    Lwt.wakeup_later(u, ());
  };

  Websocketaf.Client_connection.{frame, eof};
};

let error_handler = error => {
  print_endline("[SOCKET] GOT ERROR");
  switch (error) {
  | `Exn(_exn) => ()
  | `Handshake_failure(_resp, _body) => ()
  | `Invalid_response_body_length(_resp) => ()
  | `Malformed_response(_string) => ()
  | _ => assert(false)
  };
};

let connect = url => {
  let uri = Uri.of_string(url);
  let port = uri |> Uri.port |> Option.value(~default=0);
  let host = uri |> Uri.host_with_default(~default="");
  let resource = uri |> Uri.path;

  let%lwt addresses =
    Lwt_unix.getaddrinfo(
      host,
      string_of_int(port),
      [Unix.(AI_FAMILY(PF_INET))],
    );
  let socket = Lwt_unix.socket(Unix.PF_INET, Unix.SOCK_STREAM, 0);
  let%lwt () = Lwt_unix.connect(socket, List.hd(addresses).Unix.ai_addr);

  let (p, u) = Lwt.wait();

  let _ =
    socket
    |> Websocketaf_lwt_unix.Client.connect(
         ~nonce="0123456789ABCDEF",
         ~host,
         ~port,
         ~resource,
         ~error_handler,
         ~websocket_handler=websocket_handler(u),
       );
  p;
};

let send = message => {
  print_endline("[SOCKET] > \t" ++ message);
  let key =
    message
    |> Yojson.Safe.from_string
    |> Yojson.Safe.Util.member("id")
    |> Yojson.Safe.Util.to_int;

  let (p, resolver) = Lwt.wait();
  pending_requests |> Queue.add((key, message, resolver));
  p;
};

let listen = (event, handler) => {
  let stored_listeners = Hashtbl.find_opt(listeners, event);
  switch (stored_listeners) {
  | None => Hashtbl.add(listeners, event, [handler])
  | Some(stored) => Hashtbl.replace(listeners, event, [handler, ...stored])
  };
};

let close = () => {
  let (p, resolver) = Lwt.wait();
  close_requests |> Queue.add(resolver);
  p;
};
