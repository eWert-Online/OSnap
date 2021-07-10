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
  let debug = OSnap_Logger.debug(~header="Config.Test.parse_size");
  let width =
    size |> Yojson.Basic.Util.member("width") |> Yojson.Basic.Util.to_int;

  let height =
    size |> Yojson.Basic.Util.member("height") |> Yojson.Basic.Util.to_int;

  debug(Printf.sprintf("size is set to %ix%i", width, height));

  (width, height);
};

let parse_action = a => {
  let debug = OSnap_Logger.debug(~header="Config.Test.parse_action");
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
    debug("found click action");
    switch (selector) {
    | None =>
      debug("no selector for click action provided");
      raise(Invalid_format);
    | Some(selector) =>
      debug(Printf.sprintf("click selector is %S", selector));
      Click(selector);
    };
  | "type" =>
    debug("found type action");
    switch (selector, text) {
    | (None, _) =>
      debug("no selector for type action provided");
      raise(Invalid_format);
    | (_, None) =>
      debug("no text for type action provided");
      raise(Invalid_format);
    | (Some(selector), Some(text)) =>
      debug(
        Printf.sprintf(
          "type action selector is %S with text %S",
          selector,
          text,
        ),
      );
      Type(selector, text);
    };
  | "wait" =>
    debug("found wait action");
    switch (timeout) {
    | None =>
      debug("no timeout provided");
      raise(Invalid_format);
    | Some(timeout) =>
      debug(Printf.sprintf("timeout for wait action is %i", timeout));
      Wait(timeout);
    };
  | action =>
    debug(Printf.sprintf("found unknown action %S", action));
    raise(Invalid_format);
  };
};

let parse_ignore = r => {
  let debug = OSnap_Logger.debug(~header="Config.Test.parse_ignore");

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
  | (Some(selector), None, None, None, None) =>
    debug(Printf.sprintf("using selector %S", selector));
    Selector(selector);
  | (None, Some(x1), Some(y1), Some(x2), Some(y2)) =>
    debug(
      Printf.sprintf("using coordinates (%i,%i),(%i,%i)", x1, y1, x2, y2),
    );
    Coordinates((x1, y1), (x2, y2));
  | _ => raise(Invalid_format)
  };
};

let parse_single_test = (global_config, test) => {
  let debug = OSnap_Logger.debug(~header="Config.Test.parse");
  try({
    let name =
      test |> Yojson.Basic.Util.member("name") |> Yojson.Basic.Util.to_string;

    debug(Printf.sprintf("name: %S", name));

    let only =
      test
      |> Yojson.Basic.Util.member("only")
      |> Yojson.Basic.Util.to_bool_option
      |> Option.value(~default=false);

    debug(Printf.sprintf("only: %b", only));

    let skip =
      test
      |> Yojson.Basic.Util.member("skip")
      |> Yojson.Basic.Util.to_bool_option
      |> Option.value(~default=false);

    debug(Printf.sprintf("skip: %b", only));

    let threshold =
      test
      |> Yojson.Basic.Util.member("threshold")
      |> Yojson.Basic.Util.to_int_option
      |> Option.value(~default=global_config.OSnap_Config_Global.threshold);

    debug(Printf.sprintf("threshold: %i", threshold));

    let url =
      test |> Yojson.Basic.Util.member("url") |> Yojson.Basic.Util.to_string;

    debug(Printf.sprintf("url: %s", url));

    let sizes =
      test
      |> Yojson.Basic.Util.member("sizes")
      |> (
        fun
        | `List(list) => {
            debug("parsing sizes");
            Some(List.map(parse_size, list));
          }
        | _ => {
            debug("no sizes present. using default sizes");
            None;
          }
      )
      |> Option.value(
           ~default=global_config.OSnap_Config_Global.default_sizes,
         );

    let actions =
      test
      |> Yojson.Basic.Util.member("actions")
      |> (
        fun
        | `List(list) => {
            debug("parsing actions");
            List.map(parse_action, list);
          }
        | _ => []
      );

    let ignore =
      test
      |> Yojson.Basic.Util.member("ignore")
      |> (
        fun
        | `List(list) => {
            debug("parsing ignore regions");
            List.map(parse_ignore, list);
          }
        | _ => []
      );

    Result.ok({only, skip, threshold, name, url, sizes, actions, ignore});
  }) {
  | _ => raise(Invalid_format)
  };
};

let parse = (global_config, path) => {
  let debug = OSnap_Logger.debug(~header="Config.Test.parse");
  let config = OSnap_Utils.get_file_contents(path);
  debug(Printf.sprintf("parsing test file %S", path));

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
  let debug = OSnap_Logger.debug(~header="Config.Test.find");

  debug(Printf.sprintf("looking for test files matching %S", pattern));

  let pattern = pattern |> Re.Glob.glob |> Re.compile;
  let ignore_patterns =
    ignore_patterns
    |> List.map(pattern => {
         debug(Printf.sprintf("adding %S to ignore patterns", pattern));
         pattern |> Re.Glob.glob |> Re.compile;
       });

  let is_ignored = path => {
    let ignored = ignore_patterns |> List.exists(Re.execp(_, path));
    if (ignored) {
      debug(Printf.sprintf("ignoring %S", path));
    };
    ignored;
  };

  FileUtil.find(
    Custom(
      path =>
        if (!is_ignored(path)) {
          let matches = Re.execp(pattern, path);
          debug(Printf.sprintf("checking: %S", path));
          if (matches) {
            debug(Printf.sprintf("matched:  %S", path));
          };
          matches;
        } else {
          false;
        },
    ),
    root_path,
    (acc, curr) => [curr, ...acc],
    [],
  );
};

let init = config => {
  let debug = OSnap_Logger.debug(~header="Config.Test.init");

  debug("looking for test files");
  let tests =
    find(
      ~root_path=config.OSnap_Config_Global.root_path,
      ~pattern=config.OSnap_Config_Global.test_pattern,
      ~ignore_patterns=config.OSnap_Config_Global.ignore_patterns,
      (),
    )
    |> List.map(parse(config))
    |> List.flatten;

  debug("looking for duplicate names in test files");
  let duplicates =
    tests
    |> OSnap_Utils.find_duplicates(t => t.name)
    |> List.map(t => {
         debug(Printf.sprintf("found test with duplicate name %S", t.name));
         t.name;
       });

  if (List.length(duplicates) != 0) {
    raise(Duplicate_Tests(duplicates));
  } else {
    debug("did not find duplicates");
  };

  tests;
};
