type size = (int, int);

exception Invalid_format;
exception Duplicate_Tests(list(string));

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

let parse_size = size => {
  let width =
    size |> Yojson.Basic.Util.member("width") |> Yojson.Basic.Util.to_int;

  let height =
    size |> Yojson.Basic.Util.member("height") |> Yojson.Basic.Util.to_int;

  (width, height);
};

let parse_action = a => {
  let action =
    a |> Yojson.Basic.Util.member("action") |> Yojson.Basic.Util.to_string;

  let selector =
    a
    |> Yojson.Basic.Util.member("selector")
    |> Yojson.Basic.Util.to_string_option;

  let text =
    a
    |> Yojson.Basic.Util.member("text")
    |> Yojson.Basic.Util.to_string_option;

  let timeout =
    a
    |> Yojson.Basic.Util.member("timeout")
    |> Yojson.Basic.Util.to_int_option;

  switch (action) {
  | "click" =>
    switch (selector) {
    | None => raise(Invalid_format)
    | Some(selector) => Click(selector)
    }
  | "type" =>
    switch (selector, text) {
    | (None, _) => raise(Invalid_format)
    | (_, None) => raise(Invalid_format)
    | (Some(selector), Some(text)) => Type(selector, text)
    }
  | "wait" =>
    switch (timeout) {
    | None => raise(Invalid_format)
    | Some(timeout) => Wait(timeout)
    }
  | _ => raise(Invalid_format)
  };
};

let parse_ignore = r => {
  let x1 =
    r |> Yojson.Basic.Util.member("x1") |> Yojson.Basic.Util.to_int_option;
  let y1 =
    r |> Yojson.Basic.Util.member("y1") |> Yojson.Basic.Util.to_int_option;
  let x2 =
    r |> Yojson.Basic.Util.member("x2") |> Yojson.Basic.Util.to_int_option;
  let y2 =
    r |> Yojson.Basic.Util.member("y2") |> Yojson.Basic.Util.to_int_option;

  let selector =
    r
    |> Yojson.Basic.Util.member("selector")
    |> Yojson.Basic.Util.to_string_option;

  switch (selector, x1, y1, x2, y2) {
  | (Some(selector), None, None, None, None) => Selector(selector)
  | (None, Some(x1), Some(y1), Some(x2), Some(y2)) =>
    Coordinates((x1, y1), (x2, y2))
  | _ => raise(Invalid_format)
  };
};

let parse_single_test = (global_config, test) =>
  try({
    let name =
      test |> Yojson.Basic.Util.member("name") |> Yojson.Basic.Util.to_string;

    let only =
      test
      |> Yojson.Basic.Util.member("only")
      |> Yojson.Basic.Util.to_bool_option
      |> Option.value(~default=false);

    let skip =
      test
      |> Yojson.Basic.Util.member("skip")
      |> Yojson.Basic.Util.to_bool_option
      |> Option.value(~default=false);

    let threshold =
      test
      |> Yojson.Basic.Util.member("threshold")
      |> Yojson.Basic.Util.to_int_option
      |> Option.value(~default=global_config.OSnap_Config_Global.threshold);

    let url =
      test |> Yojson.Basic.Util.member("url") |> Yojson.Basic.Util.to_string;

    let sizes =
      test
      |> Yojson.Basic.Util.member("sizes")
      |> (
        fun
        | `List(list) => Some(List.map(parse_size, list))
        | _ => None
      )
      |> Option.value(
           ~default=global_config.OSnap_Config_Global.default_sizes,
         );

    let actions =
      test
      |> Yojson.Basic.Util.member("actions")
      |> (
        fun
        | `List(list) => List.map(parse_action, list)
        | _ => []
      );

    let ignore =
      test
      |> Yojson.Basic.Util.member("ignore")
      |> (
        fun
        | `List(list) => List.map(parse_ignore, list)
        | _ => []
      );

    Result.ok({only, skip, threshold, name, url, sizes, actions, ignore});
  }) {
  | _ => raise(Invalid_format)
  };

let parse = (global_config, path) => {
  let ic = open_in(path);
  let file_length = in_channel_length(ic);
  let config = really_input_string(ic, file_length);
  close_in(ic);

  try({
    let json = config |> Yojson.Basic.from_string;

    let (tests, failed) =
      json
      |> Yojson.Basic.Util.to_list
      |> List.map(parse_single_test(global_config))
      |> List.partition(Result.is_ok);

    if (List.length(failed) > 0) {
      raise(Invalid_format);
    } else {
      tests |> List.map(Result.get_ok);
    };
  }) {
  | _ => raise(Invalid_format)
  };
};

let find =
    (~root_path="/", ~pattern="**/*.osnap.json", ~ignore_patterns=[], ()) => {
  let pattern = pattern |> Re.Glob.glob |> Re.compile;
  let ignore_patterns =
    ignore_patterns
    |> List.map(pattern => pattern |> Re.Glob.glob |> Re.compile);

  let is_ignored = path => {
    ignore_patterns |> List.exists(Re.execp(_, path));
  };

  FileUtil.find(
    Custom(path => !is_ignored(path) && Re.execp(pattern, path)),
    root_path,
    (acc, curr) => [curr, ...acc],
    [],
  );
};

let init = config => {
  let tests =
    find(
      ~root_path=config.OSnap_Config_Global.root_path,
      ~pattern=config.OSnap_Config_Global.test_pattern,
      ~ignore_patterns=config.OSnap_Config_Global.ignore_patterns,
      (),
    )
    |> List.map(parse(config))
    |> List.flatten;

  let duplicates =
    tests |> Utils.find_duplicates(t => t.name) |> List.map(t => t.name);

  if (List.length(duplicates) != 0) {
    raise(Duplicate_Tests(duplicates));
  };

  tests;
};
