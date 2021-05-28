type t = {
  config: OSnap_Config.Global.t,
  browser: OSnap_Browser.t,
  tests: list(OSnap_Config.Test.t),
  snapshot_dir: string,
  updated_dir: string,
  diff_dir: string,
  start_time: float,
};

let setup: (~config_path: string) => Lwt_result.t(t, 'a);

let run:
  (~noCreate: bool, ~noOnly: bool, ~noSkip: bool, t) =>
  Lwt_result.t(unit, unit);

let teardown: t => unit;
