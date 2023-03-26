module Printer = OSnap_Test_Printer
module Types = OSnap_Test_Types

val get_filename : ?diff:bool -> string -> int -> int -> string

val run
  :  fs:#Eio.Fs.dir Eio.Path.t
  -> clock:#Eio.Time.clock
  -> OSnap_Config.Types.global
  -> OSnap_Browser.Target.target
  -> Types.t
  -> ( Types.t
     , [> `OSnap_CDP_Protocol_Error of string | `OSnap_FS_Error of string ] )
     Result.t
