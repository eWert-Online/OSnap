type t;

module Launcher: {
  let make: unit => Lwt.t(t);

  let shutdown: t => unit;
};

module Target: {
  type target = {
    targetId: OSnap_CDP.Types.Target.TargetId.t,
    sessionId: OSnap_CDP.Types.Target.SessionId.t,
  };

  let make: t => Lwt.t(target);
};

module Actions: {
  let click: (~selector: string, Target.target) => Lwt.t(unit);

  let type_text:
    (~selector: string, ~text: string, Target.target) => Lwt.t(unit);

  let wait_for: (~event: string, Target.target) => Lwt.t(unit);

  let wait_for_network_idle:
    (Target.target, ~loaderId: OSnap_CDP.Types.Network.LoaderId.t) =>
    Lwt.t(unit);

  let go_to: (~url: string, Target.target) => Lwt.t(string);

  let set_size: (~width: int, ~height: int, Target.target) => Lwt.t(unit);

  let screenshot: (~full_size: bool=?, Target.target) => Lwt.t(string);
};

module Path: {
  let get_revision: unit => string;

  let get_folder_name: unit => string;

  let get_chromium_path: unit => string;
};
