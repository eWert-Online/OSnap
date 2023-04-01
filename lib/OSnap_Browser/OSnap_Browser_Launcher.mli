val make
  :  unit
  -> ( OSnap_Browser_Types.t
     , [> `OSnap_Chromium_Download_Failed
       | `OSnap_CDP_Connection_Failed
       | `OSnap_CDP_Protocol_Error of string
       ] )
     Lwt_result.t

val shutdown : OSnap_Browser_Types.t -> unit
