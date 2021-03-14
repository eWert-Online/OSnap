open OSnap_Browser_Types;

let wait_for: string => Lwt.t(unit);

let go_to: (string, t) => Lwt.t(string);

let set_size: (~width: int, ~height: int, t) => Lwt.t(unit);

let screenshot: (~full_size: bool=?, t) => Lwt.t(string);
