module Utils = OSnap_Utils;

type t;

let setup:
  (~noCreate: bool, ~noOnly: bool, ~noSkip: bool, ~config_path: string) =>
  Lwt_result.t(t, 'a);

let cleanup: (~config_path: string) => Result.t(unit, unit);

let run: t => Lwt_result.t(unit, unit);
