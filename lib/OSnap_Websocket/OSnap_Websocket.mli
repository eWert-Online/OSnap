val listen
  :  ?look_behind:bool
  -> event:string
  -> sessionId:string
  -> (string -> (unit -> unit) -> unit)
  -> unit

val close : unit -> unit
val send : (int -> string) -> string

val connect
  :  sw:Eio.Switch.t
  -> env:Eio_unix.Stdenv.base
  -> string
  -> (unit, [> `OSnap_CDP_Connection_Failed ]) result
