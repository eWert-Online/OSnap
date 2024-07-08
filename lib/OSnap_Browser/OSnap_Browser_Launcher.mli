val make
  :  sw:Eio.Switch.t
  -> env:Eio_unix.Stdenv.base
  -> unit
  -> ( OSnap_Browser_Types.t
       , [> `OSnap_CDP_Connection_Failed
         | `OSnap_CDP_Protocol_Error of string
         | `OSnap_Chromium_Download_Failed
         ] )
       result

val shutdown : OSnap_Browser_Types.t -> unit
