val listen
  :  ?look_behind:bool
  -> event:string
  -> sessionId:string
  -> (string -> (unit -> unit) -> unit)
  -> unit

val close : unit -> unit Eio.Promise.t
val send : (int -> string) -> string Eio.Promise.t
val connect : string -> unit
