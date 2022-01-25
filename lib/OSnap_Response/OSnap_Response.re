type t =
  | Config_Parse_Error(string, option(string))
  | Config_Global_Not_Found
  | Config_Unsupported_Format(string)
  | Config_Invalid(string, option(string))
  | Config_Duplicate_Tests(list(string))
  | Config_Duplicate_Size_Names(list(string))
  | CDP_Protocol_Error(string)
  | CDP_Connection_Failed
  | Invalid_Run(string)
  | FS_Error(string)
  | Test_Failure
  | Unknown_Error(exn);
