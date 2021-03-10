open Lwt.Syntax;

Lwt_main.run(
  {
    let* t = OSnap.setup();
    let* () = OSnap.run(t);
    let () = OSnap.teardown(t);

    Lwt.return();
  },
);
