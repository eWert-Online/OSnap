module Config = OSnap_Config
module Browser = OSnap_Browser

type t =
  { config : Config.Types.global
  ; tests_to_run : (Config.Types.test * Config.Types.size * bool) list
  ; start_time : float
  ; browser : Browser.t
  }

val setup
  :  sw:Eio.Switch.t
  -> env:Eio_unix.Stdenv.base
  -> noCreate:bool
  -> noOnly:bool
  -> noSkip:bool
  -> config_path:string
  -> ( t
       , [> `OSnap_CDP_Connection_Failed
         | `OSnap_CDP_Protocol_Error of string
         | `OSnap_Chromium_Download_Failed
         | `OSnap_Config_Duplicate_Size_Names of string list
         | `OSnap_Config_Duplicate_Tests of string list
         | `OSnap_Config_Global_Invalid of string
         | `OSnap_Config_Global_Not_Found
         | `OSnap_Config_Invalid of string * Eio.Fs.dir_ty Eio.Path.t
         | `OSnap_Config_Parse_Error of string * Eio.Fs.dir_ty Eio.Path.t
         | `OSnap_Config_Undefined_Function of string * Eio.Fs.dir_ty Eio.Path.t
         | `OSnap_Config_Unsupported_Format of Eio.Fs.dir_ty Eio.Path.t
         | `OSnap_Invalid_Run of string
         ] )
       result

val teardown : t -> unit

val run
  :  env:Eio_unix.Stdenv.base
  -> t
  -> ( unit
       , [> `OSnap_CDP_Protocol_Error of string
         | `OSnap_FS_Error of string
         | `OSnap_Test_Failure
         ] )
       result

val cleanup
  :  env:Eio_unix.Stdenv.base
  -> config_path:string
  -> ( unit
       , [> `OSnap_Config_Duplicate_Size_Names of string list
         | `OSnap_Config_Duplicate_Tests of string list
         | `OSnap_Config_Global_Invalid of string
         | `OSnap_Config_Global_Not_Found
         | `OSnap_Config_Invalid of string * Eio.Fs.dir_ty Eio.Path.t
         | `OSnap_Config_Parse_Error of string * Eio.Fs.dir_ty Eio.Path.t
         | `OSnap_Config_Undefined_Function of string * Eio.Fs.dir_ty Eio.Path.t
         | `OSnap_Config_Unsupported_Format of Eio.Fs.dir_ty Eio.Path.t
         ] )
       result
