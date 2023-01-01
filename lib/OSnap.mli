module Utils = OSnap_Utils

type t

val setup
  :  noCreate:bool
  -> noOnly:bool
  -> noSkip:bool
  -> parallelism:int option
  -> config_path:string
  -> (t, OSnap_Response.t) Lwt_result.t

val teardown : t -> unit
val cleanup : config_path:string -> (unit, OSnap_Response.t) Result.t
val run : t -> (unit, OSnap_Response.t) Lwt_result.t
val download_chromium : unit -> (unit, unit) Lwt_result.t
