let id = ref(0);

let id = () => {
  incr(id);
  id^;
};

let close_requests = Queue.create();
let pending_requests = Queue.create();
let sent_requests = Hashtbl.create(10);
let listeners = Hashtbl.create(10);

let events = Hashtbl.create(1000);

let call_event_handlers = (key, message) => {
  List.iter(handler =>
    handler(
      message,
      () => {
        Hashtbl.remove(listeners, key);
        Hashtbl.remove(events, key);
      },
    )
  );
};

let debug_send = OSnap_Logger.debug(~header="Websocket >>>");
let debug_recieve = OSnap_Logger.debug(~header="Websocket <<<");

let websocket_handler = (recv, send) => {
  let close = () => {
    Websocket.Frame.close(1002) |> send;
  };

  let send_payload = payload => {
    debug_send(payload);
    Websocket.Frame.create(~content=payload, ()) |> send;
  };

  let rec input_loop = () => {
    let%lwt () = Lwt.pause();
    if (!Queue.is_empty(close_requests)) {
      close_requests |> Queue.iter(resolver => Lwt.wakeup_later(resolver, ()));
      close_requests |> Queue.clear;
      close();
    } else if (!Queue.is_empty(pending_requests)) {
      let (key, message, resolver) = Queue.take(pending_requests);
      let%lwt () = send_payload(message);
      Hashtbl.add(sent_requests, key, resolver);
      input_loop();
    } else {
      input_loop();
    };
  };

  let react = (frame: Websocket.Frame.t) => {
    switch (frame.opcode) {
    | Close
    | Continuation
    | Ctrl(_)
    | Nonctrl(_) => close()
    | Ping => Websocket.Frame.create(~opcode=Pong, ()) |> send
    | Pong => Lwt.return()
    | Text
    | Binary =>
      let response = frame.Websocket.Frame.content;
      debug_recieve(
        String.sub(response, 0, min(String.length(response), 800)),
      );
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

      let sessionId =
        response
        |> Yojson.Safe.from_string
        |> Yojson.Safe.Util.member("sessionId")
        |> Yojson.Safe.Util.to_string_option;

      switch (method, sessionId) {
      | (None, None) => ()
      | (None, _) => ()
      | (Some(method), None) =>
        let key = method;
        Hashtbl.add(events, key, response);
        Hashtbl.find_opt(listeners, key)
        |> Option.iter(call_event_handlers(key, response));
      | (Some(method), Some(sessionId)) =>
        let key = method ++ sessionId;
        Hashtbl.add(events, key, response);
        Hashtbl.find_opt(listeners, key)
        |> Option.iter(call_event_handlers(key, response));
      };

      switch (id) {
      | None => Lwt.return()
      | Some(key) =>
        Hashtbl.find_opt(sent_requests, key)
        |> Option.iter(resolver => {Lwt.wakeup_later(resolver, response)});
        Hashtbl.remove(sent_requests, key);
        Lwt.return();
      };
    };
  };

  let rec react_forever = () => {
    let%lwt frame = recv();
    let%lwt () = react(frame);
    react_forever();
  };

  Lwt.pick([input_loop(), react_forever()]);
};

let connect = url => {
  let orig_uri = Uri.of_string(url);
  let uri = Uri.with_scheme(orig_uri, Some("http"));

  let%lwt endpoint = Resolver_lwt.resolve_uri(~uri, Resolver_lwt_unix.system);

  let%lwt client =
    endpoint
    |> Conduit_lwt_unix.endp_to_client(~ctx=Conduit_lwt_unix.default_ctx);

  let%lwt (recv, send) =
    Websocket_lwt_unix.with_connection(
      ~ctx=Conduit_lwt_unix.default_ctx,
      client,
      uri,
    );

  websocket_handler(recv, send);
};

let send = message => {
  let key = id();
  let message = message(key);
  let (p, resolver) = Lwt.wait();
  pending_requests |> Queue.add((key, message, resolver));
  p;
};

let listen = (~look_behind=true, ~event, ~sessionId, handler) => {
  let key = event ++ sessionId;
  let stored_listeners = Hashtbl.find_opt(listeners, key);
  switch (stored_listeners) {
  | None => Hashtbl.add(listeners, key, [handler])
  | Some(stored) => Hashtbl.replace(listeners, key, [handler, ...stored])
  };

  if (look_behind) {
    Hashtbl.find_all(events, key)
    |> List.iter(event => {
         handler(
           event,
           () => {
             Hashtbl.remove(listeners, key);
             Hashtbl.remove(events, key);
           },
         )
       });
  };
};

let close = () => {
  let (p, resolver) = Lwt.wait();
  close_requests |> Queue.add(resolver);
  p;
};
