module Config = OSnap_Config;
module Browser = OSnap_Browser;

type action =
  | Click(string)
  | Type(string, string);

type test = {
  name: string,
  url: string,
  sizes: list(OSnap_Config.size),
  actions: option(list(action)),
};

type browser;

let init: unit => browser;

let test: (test, browser) => Lwt_result.t(unit, string);

let close: browser => unit;
