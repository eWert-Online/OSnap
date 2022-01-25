module Logger = OSnap_Logger;
module Utils = OSnap_Utils;

type t;

let setup:
  (
    ~noCreate: bool,
    ~noOnly: bool,
    ~noSkip: bool,
    ~parallelism: option(int),
    ~config_path: string
  ) =>
  Lwt_result.t(t, OSnap_Response.t);

let teardown: t => unit;

let cleanup: (~config_path: string) => Result.t(unit, OSnap_Response.t);

let run: t => Lwt_result.t(unit, OSnap_Response.t);

let download_chromium: unit => Lwt_result.t(unit, unit);
