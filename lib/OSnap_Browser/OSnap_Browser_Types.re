type t = {
  ws: string,
  browserContextId: OSnap_CDP.Types.Browser.BrowserContextID.t,
  targets:
    list(
      (OSnap_CDP.Types.Target.TargetId.t, OSnap_CDP.Types.Target.SessionId.t),
    ),
  process: Lwt_process.process_full,
};
