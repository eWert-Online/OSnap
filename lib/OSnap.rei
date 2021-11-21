module Logger = OSnap_Logger;
module Utils = OSnap_Utils;

type t;

exception Failed_Test(int);

let setup:
  (~noCreate: bool, ~noOnly: bool, ~noSkip: bool, ~config_path: string) =>
  Lwt_result.t(t, exn);

let teardown: t => unit;

let cleanup: (~config_path: string) => Result.t(unit, unit);

let run: t => Lwt_result.t(unit, exn);

let download_chromium: unit => Lwt_result.t(unit, unit);
