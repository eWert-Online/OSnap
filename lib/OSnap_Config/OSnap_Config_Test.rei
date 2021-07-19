exception Invalid_format;
exception Duplicate_Tests(list(string));

let init: OSnap_Config_Types.global => list(OSnap_Config_Types.test);
