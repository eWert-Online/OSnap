module Server = Cohttp_lwt_unix.Server

let serve ?(port = 3000) docroot =
  let callback _ request _body =
    match Cohttp.Request.meth request with
    | `GET ->
      let uri = request |> Cohttp.Request.uri in
      let fname = Server.resolve_file ~docroot ~uri in
      Server.respond_file ~fname ()
    | _ -> Server.respond_not_found ()
  in
  Server.create ~mode:(`TCP (`Port port)) (Server.make ~callback ())
;;
