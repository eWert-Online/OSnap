open Lwt.Syntax;

Printexc.record_backtrace(true);

Lwt_main.run(
  try(
    {
      let* t = OSnap.setup();
      let* () = OSnap.run(t);
      let () = OSnap.teardown(t);

      Lwt.return();
    }
  ) {
  | e =>
    e |> Printexc.to_string |> print_endline;
    Lwt.return();
  },
);
