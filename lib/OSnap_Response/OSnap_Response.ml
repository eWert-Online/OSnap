type t =
  | Config_Parse_Error of string * string option
  | Config_Global_Not_Found
  | Config_Unsupported_Format of string
  | Config_Invalid of string * string option
  | Config_Duplicate_Tests of string list
  | Config_Duplicate_Size_Names of string list
  | CDP_Protocol_Error of string
  | CDP_Connection_Failed
  | Invalid_Run of string
  | FS_Error of string
  | Test_Failure
  | Unknown_Error of exn
