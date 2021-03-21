type t;

module Launcher: {
  let make: unit => Lwt.t(t);

  let create_targets: (int, t) => Lwt.t(t);

  let get_targets:
    t =>
    list(
      (OSnap_CDP.Types.Target.TargetId.t, OSnap_CDP.Types.Target.SessionId.t),
    );

  let shutdown: t => unit;
};

module Actions: {
  let wait_for:
    (
      ~event: string,
      (OSnap_CDP.Types.Target.TargetId.t, OSnap_CDP.Types.Target.SessionId.t)
    ) =>
    Lwt.t(unit);

  let wait_for_network_idle:
    (
      (OSnap_CDP.Types.Target.TargetId.t, OSnap_CDP.Types.Target.SessionId.t),
      ~loaderId: OSnap_CDP.Types.Network.LoaderId.t
    ) =>
    Lwt.t(unit);

  let go_to:
    (
      ~url: string,
      (OSnap_CDP.Types.Target.TargetId.t, OSnap_CDP.Types.Target.SessionId.t)
    ) =>
    Lwt.t(string);

  let set_size:
    (
      ~width: int,
      ~height: int,
      (OSnap_CDP.Types.Target.TargetId.t, OSnap_CDP.Types.Target.SessionId.t)
    ) =>
    Lwt.t(unit);

  let screenshot:
    (
      ~full_size: bool=?,
      (OSnap_CDP.Types.Target.TargetId.t, OSnap_CDP.Types.Target.SessionId.t)
    ) =>
    Lwt.t(string);
};
