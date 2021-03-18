type t;

module Launcher: {
  let make: unit => Lwt.t(t);

  let create_targets: (int, t) => Lwt.t(t);

  let get_targets:
    t =>
    list(
      (OSnap_CDP.Types.Target.TargetID.t, OSnap_CDP.Types.Target.SessionID.t),
    );

  let shutdown: t => unit;
};

module Actions: {
  let wait_for: (~event: string, (string, string)) => Lwt.t(unit);

  let go_to:
    (
      ~url: string,
      (OSnap_CDP.Types.Target.TargetID.t, OSnap_CDP.Types.Target.SessionID.t)
    ) =>
    Lwt.t(string);

  let set_size:
    (
      ~width: int,
      ~height: int,
      (OSnap_CDP.Types.Target.TargetID.t, OSnap_CDP.Types.Target.SessionID.t)
    ) =>
    Lwt.t(unit);

  let screenshot:
    (
      ~full_size: bool=?,
      (OSnap_CDP.Types.Target.TargetID.t, OSnap_CDP.Types.Target.SessionID.t)
    ) =>
    Lwt.t(string);
};
