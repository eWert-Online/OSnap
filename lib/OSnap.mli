module Config = OSnap_Config
module Browser = OSnap_Browser
module Printer = OSnap_Printer
module Server = OSnap_Server

val print_error : ('a, unit, string, unit) format4 -> 'a

type t =
  { config : Config.Types.global
  ; all_tests : (Config.Types.test * Config.Types.size * bool) list
  ; tests_to_run : (Config.Types.test * Config.Types.size * bool) list
  ; start_time : float
  ; browser : Browser.t
  }

val setup
  :  noCreate:bool
  -> noOnly:bool
  -> noSkip:bool
  -> parallelism:int option
  -> config_path:string
  -> ( t
     , [> `OSnap_CDP_Connection_Failed
       | `OSnap_CDP_Protocol_Error of string
       | `OSnap_Config_Duplicate_Size_Names of string list
       | `OSnap_Config_Duplicate_Tests of string list
       | `OSnap_Config_Global_Invalid of string
       | `OSnap_Config_Global_Not_Found
       | `OSnap_Config_Invalid of string * string
       | `OSnap_Config_Parse_Error of string * string
       | `OSnap_Config_Unsupported_Format of string
       | `OSnap_Invalid_Run of string
       ] )
     Lwt_result.t

val teardown : t -> unit

val run
  :  t
  -> ( unit
     , [> `OSnap_CDP_Protocol_Error of string
       | `OSnap_Config_Undefined_Function of string
       | `OSnap_FS_Error of string
       | `OSnap_Test_Failure
       ] )
     Lwt_result.t

val cleanup
  :  config_path:string
  -> ( unit
     , [> `OSnap_Config_Duplicate_Size_Names of string list
       | `OSnap_Config_Duplicate_Tests of string list
       | `OSnap_Config_Global_Invalid of string
       | `OSnap_Config_Global_Not_Found
       | `OSnap_Config_Invalid of string * string
       | `OSnap_Config_Parse_Error of string * string
       | `OSnap_Config_Unsupported_Format of string
       ] )
     result

val download_chromium : unit -> (unit, unit) Lwt_result.t
