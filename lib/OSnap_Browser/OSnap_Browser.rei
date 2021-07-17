type t;

module Launcher: {
  let make: unit => Lwt.t(t);

  let shutdown: t => unit;
};

module Target: {
  type target = {
    targetId: Cdp.Types.Target.TargetID.t,
    sessionId: Cdp.Types.Target.SessionID.t,
  };

  let make: t => Lwt.t(target);
};

module Actions: {
  let get_quads:
    (~selector: string, Target.target) =>
    Lwt.t(((float, float), (float, float)));

  let click: (~selector: string, Target.target) => Lwt.t(unit);

  let type_text:
    (~selector: string, ~text: string, Target.target) => Lwt.t(unit);

  let wait_for: (~event: string, Target.target) => Lwt.t(unit);

  let wait_for_network_idle:
    (Target.target, ~loaderId: Cdp.Types.Network.LoaderId.t) => Lwt.t(unit);

  let go_to: (~url: string, Target.target) => Lwt_result.t(string, string);

  let set_size: (~width: int, ~height: int, Target.target) => Lwt.t(unit);

  let screenshot: (~full_size: bool=?, Target.target) => Lwt.t(string);
};

module Download: {let download: unit => Lwt_result.t(unit, unit);};
