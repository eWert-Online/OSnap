type t =
  { ws : string
  ; browserContextId : Cdp.Types.Browser.BrowserContextID.t
  ; process : [ `Generic | `Unix ] Eio.Process.ty Eio.Resource.t
  }
