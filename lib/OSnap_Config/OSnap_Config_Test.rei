exception Invalid_format;
exception Duplicate_Tests(list(string));

type size = (int, int);

type action =
  | Click(string)
  | Type(string, string)
  | Wait(int);

type ignoreType =
  | Coordinates((int, int), (int, int))
  | Selector(string);

type t = {
  only: bool,
  skip: bool,
  threshold: int,
  name: string,
  url: string,
  sizes: list(size),
  actions: list(action),
  ignore: list(ignoreType),
};

let init: OSnap_Config_Global.t => list(t);
