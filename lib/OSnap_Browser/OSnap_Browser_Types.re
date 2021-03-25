type t = {
  ws: string,
  browserContextId: OSnap_CDP.Types.Browser.BrowserContextID.t,
  process: Lwt_process.process_full,
};
