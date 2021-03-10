module Io = OSnap_Io;
module Test = OSnap_Test;
module Utils = OSnap_Utils;
module Config = OSnap_Config;
module Browser = OSnap_Browser;
module Websocket = OSnap_Websocket;

type t = {
  config: Config.t,
  browser: Browser.t,
  tests: list(Test.t),
  snapshot_dir: string,
  updated_dir: string,
  diff_dir: string,
};

let setup: unit => Lwt.t(t);

let run: t => Lwt.t(unit);

let teardown: t => unit;
