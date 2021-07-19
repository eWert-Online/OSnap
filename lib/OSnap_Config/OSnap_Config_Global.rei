exception Parse_Error(string);
exception No_Config_Found;

let parse: string => OSnap_Config_Types.global;

let find: (~config_path: string) => string;
