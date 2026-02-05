type target =
  { targetId : Cdp.Types.Target.TargetID.t
  ; sessionId : Cdp.Types.Target.SessionID.t
  }

val make
  :  config:OSnap_Config.Types.global
  -> OSnap_Browser_Types.t
  -> (target, [> `OSnap_CDP_Protocol_Error of string ]) Result.t
