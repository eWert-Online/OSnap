type t =
  { ws : string
  ; browserContextId : Cdp.Types.Browser.BrowserContextID.t
  ; process : Lwt_process.process_full
  }
