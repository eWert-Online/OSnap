type t;

module Launcher: {
  let make: unit => Lwt_result.t(t, OSnap_Response.t);

  let shutdown: t => unit;
};

module Target: {
  type target = {
    targetId: Cdp.Types.Target.TargetID.t,
    sessionId: Cdp.Types.Target.SessionID.t,
  };

  let make: t => Lwt_result.t(target, OSnap_Response.t);
};

module Actions: {
  let get_quads:
    (~selector: string, Target.target) =>
    Lwt_result.t(((float, float), (float, float)), OSnap_Response.t);

  let scroll:
    (~selector: option(string), ~px: option(int), Target.target) =>
    Lwt_result.t(unit, OSnap_Response.t);

  let click:
    (~selector: string, Target.target) => Lwt_result.t(unit, OSnap_Response.t);

  let type_text:
    (~selector: string, ~text: string, Target.target) =>
    Lwt_result.t(unit, OSnap_Response.t);

  let wait_for:
    (~timeout: float=?, ~look_behind: bool=?, ~event: string, Target.target) =>
    Lwt.t([> | `Data(string) | `Timeout]);

  let wait_for_network_idle:
    (Target.target, ~loaderId: Cdp.Types.Network.LoaderId.t) => Lwt.t(unit);

  let go_to:
    (~url: string, Target.target) => Lwt_result.t(string, OSnap_Response.t);

  let get_content_size:
    Target.target =>
    Lwt_result.t((Cdp.Types.number, Cdp.Types.number), OSnap_Response.t);

  let set_size:
    (~width: Cdp.Types.number, ~height: Cdp.Types.number, Target.target) =>
    Lwt_result.t(unit, OSnap_Response.t);

  let screenshot:
    (~full_size: bool=?, Target.target) =>
    Lwt_result.t(string, OSnap_Response.t);

  let clear_cookies: Target.target => Lwt_result.t(unit, OSnap_Response.t);
};

module Download: {
  let get_uri: (string, OSnap_Utils.platform) => Uri.t;

  let download: unit => Lwt_result.t(unit, unit);
};
