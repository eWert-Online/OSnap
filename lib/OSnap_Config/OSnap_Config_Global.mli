val init
  :  config_path:string
  -> ( OSnap_Config_Types.global
     , [> `OSnap_Config_Duplicate_Size_Names of string list
       | `OSnap_Config_Global_Not_Found
       | `OSnap_Config_Invalid of string * string
       | `OSnap_Config_Parse_Error of string * string
       | `OSnap_Config_Unsupported_Format of string
       | `OSnap_Config_Undefined_Function of string * string
       ] )
     result
