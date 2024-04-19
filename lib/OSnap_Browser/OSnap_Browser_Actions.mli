val get_document
  :  OSnap_Browser_Target.target
  -> ( Cdp.Commands.DOM.GetDocument.Response.result
       , [> `OSnap_CDP_Protocol_Error of string ] )
       result
       Lwt.t

val wait_for_network_idle : OSnap_Browser_Target.target -> loaderId:string -> unit Lwt.t

val go_to
  :  url:string
  -> OSnap_Browser_Target.target
  -> (string, [> `OSnap_CDP_Protocol_Error of string ]) Lwt_result.t

val type_text
  :  document:Cdp.Commands.DOM.GetDocument.Response.result
  -> selector:string
  -> text:string
  -> OSnap_Browser_Target.target
  -> ( unit
       , [> `OSnap_CDP_Protocol_Error of string
         | `OSnap_Selector_Not_Found of string
         | `OSnap_Selector_Not_Visible of string
         ] )
       Lwt_result.t

val get_quads_all
  :  document:Cdp.Commands.DOM.GetDocument.Response.result
  -> selector:string
  -> OSnap_Browser_Target.target
  -> ( ((float * float) * (float * float)) list
       , [> `OSnap_CDP_Protocol_Error of string
         | `OSnap_Selector_Not_Found of string
         | `OSnap_Selector_Not_Visible of string
         ] )
       Lwt_result.t

val get_quads
  :  document:Cdp.Commands.DOM.GetDocument.Response.result
  -> selector:string
  -> OSnap_Browser_Target.target
  -> ( (float * float) * (float * float)
       , [> `OSnap_CDP_Protocol_Error of string
         | `OSnap_Selector_Not_Found of string
         | `OSnap_Selector_Not_Visible of string
         ] )
       Lwt_result.t

val mousemove
  :  document:Cdp.Commands.DOM.GetDocument.Response.result
  -> to_:[< `Coordinates of Cdp.Types.number * Cdp.Types.number | `Selector of string ]
  -> OSnap_Browser_Target.target
  -> ( unit
       , [> `OSnap_CDP_Protocol_Error of string
         | `OSnap_Selector_Not_Found of string
         | `OSnap_Selector_Not_Visible of string
         ] )
       Lwt_result.t

val click
  :  document:Cdp.Commands.DOM.GetDocument.Response.result
  -> selector:string
  -> OSnap_Browser_Target.target
  -> ( unit
       , [> `OSnap_CDP_Protocol_Error of string
         | `OSnap_Selector_Not_Found of string
         | `OSnap_Selector_Not_Visible of string
         ] )
       Lwt_result.t

val scroll
  :  document:Cdp.Commands.DOM.GetDocument.Response.result
  -> selector:string option
  -> px:int option
  -> OSnap_Browser_Target.target
  -> ( unit
       , [> `OSnap_CDP_Protocol_Error of string
         | `OSnap_Selector_Not_Found of string
         | `OSnap_Selector_Not_Visible of string
         ] )
       Lwt_result.t

val set_size
  :  width:Cdp.Types.number
  -> height:Cdp.Types.number
  -> OSnap_Browser_Target.target
  -> (unit, [> `OSnap_CDP_Protocol_Error of string ]) Lwt_result.t

val screenshot
  :  ?full_size:bool
  -> OSnap_Browser_Target.target
  -> (string, [> `OSnap_CDP_Protocol_Error of string ]) Lwt_result.t

val clear_cookies
  :  OSnap_Browser_Target.target
  -> (unit, [> `OSnap_CDP_Protocol_Error of string ]) Lwt_result.t
