module Io = OSnap_Io;
module Config = OSnap_Config;
module Browser = OSnap_Browser;
module Websocket = OSnap_Websocket;

type action =
  | Click(string)
  | Type(string, string);

type test = {
  name: string,
  url: string,
  sizes: list((int, int)),
  actions: option(list(action)),
};

type browser;

let init: unit => browser = () => Obj.magic("");

let test: (test, browser) => Lwt_result.t(unit, string) =
  (_test, _browser) => {
    Lwt_result.return();
  };

let close: browser => unit = _browser => ();
