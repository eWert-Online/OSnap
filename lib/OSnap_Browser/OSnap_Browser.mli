type t

module Launcher : sig
  val make : unit -> (t, OSnap_Response.t) Lwt_result.t
  val shutdown : t -> unit
end

module Target : sig
  type target =
    { targetId : Cdp.Types.Target.TargetID.t
    ; sessionId : Cdp.Types.Target.SessionID.t
    }

  val make : t -> (target, OSnap_Response.t) Lwt_result.t
end

module Actions : sig
  val get_document
    :  Target.target
    -> (Cdp.Commands.DOM.GetDocument.Response.result, OSnap_Response.t) Lwt_result.t

  val get_quads
    :  document:Cdp.Commands.DOM.GetDocument.Response.result
    -> selector:string
    -> Target.target
    -> ((float * float) * (float * float), OSnap_Response.t) Lwt_result.t

  val get_quads_all
    :  document:Cdp.Commands.DOM.GetDocument.Response.result
    -> selector:string
    -> Target.target
    -> (((float * float) * (float * float)) list, OSnap_Response.t) Lwt_result.t

  val scroll
    :  document:Cdp.Commands.DOM.GetDocument.Response.result
    -> selector:string option
    -> px:int option
    -> Target.target
    -> (unit, OSnap_Response.t) Lwt_result.t

  val mousemove
    :  document:Cdp.Commands.DOM.GetDocument.Response.result
    -> to_:[ `Selector of string | `Coordinates of Cdp.Types.number * Cdp.Types.number ]
    -> Target.target
    -> (unit, OSnap_Response.t) Lwt_result.t

  val click
    :  document:Cdp.Commands.DOM.GetDocument.Response.result
    -> selector:string
    -> Target.target
    -> (unit, OSnap_Response.t) Lwt_result.t

  val type_text
    :  document:Cdp.Commands.DOM.GetDocument.Response.result
    -> selector:string
    -> text:string
    -> Target.target
    -> (unit, OSnap_Response.t) Lwt_result.t

  val wait_for
    :  ?timeout:float
    -> ?look_behind:bool
    -> event:string
    -> Target.target
    -> [> `Data of string | `Timeout ] Lwt.t

  val wait_for_network_idle
    :  Target.target
    -> loaderId:Cdp.Types.Network.LoaderId.t
    -> unit Lwt.t

  val go_to : url:string -> Target.target -> (string, OSnap_Response.t) Lwt_result.t

  val get_content_size
    :  Target.target
    -> (Cdp.Types.number * Cdp.Types.number, OSnap_Response.t) Lwt_result.t

  val set_size
    :  width:Cdp.Types.number
    -> height:Cdp.Types.number
    -> Target.target
    -> (unit, OSnap_Response.t) Lwt_result.t

  val screenshot
    :  ?full_size:bool
    -> Target.target
    -> (string, OSnap_Response.t) Lwt_result.t

  val clear_cookies : Target.target -> (unit, OSnap_Response.t) Lwt_result.t
end

module Download : sig
  val get_uri : string -> OSnap_Utils.platform -> Uri.t
  val download : unit -> (unit, unit) Lwt_result.t
end
