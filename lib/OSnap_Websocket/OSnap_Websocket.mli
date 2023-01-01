val listen
  :  ?look_behind:bool
  -> event:string
  -> sessionId:string
  -> (string -> (unit -> unit) -> unit)
  -> unit

val close : unit -> unit Lwt.t
val send : (int -> string) -> string Lwt.t
val connect : string -> unit Lwt.t
