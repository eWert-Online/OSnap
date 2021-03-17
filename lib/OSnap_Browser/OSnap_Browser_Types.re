type t = {
  ws: string,
  browserContextId: OSnap_CDP.Types.Browser.BrowserContextID.t,
  targets:
    list(
      (OSnap_CDP.Types.Target.TargetID.t, OSnap_CDP.Types.Target.SessionID.t),
    ),
  process: Lwt_process.process_full,
};
