val init
  :  env:Eio_unix.Stdenv.base
  -> config_path:string
  -> ( OSnap_Config_Types.global
       , [> `OSnap_Config_Duplicate_Size_Names of string list
         | `OSnap_Config_Global_Not_Found
         | `OSnap_Config_Invalid of string * Eio.Fs.dir_ty Eio.Path.t
         | `OSnap_Config_Parse_Error of string * Eio.Fs.dir_ty Eio.Path.t
         | `OSnap_Config_Undefined_Function of string * Eio.Fs.dir_ty Eio.Path.t
         | `OSnap_Config_Unsupported_Format of Eio.Fs.dir_ty Eio.Path.t
         ] )
       result
