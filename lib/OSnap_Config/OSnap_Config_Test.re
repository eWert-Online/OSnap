open OSnap_Config_Types;

exception Invalid_format;
exception Duplicate_Tests(list(string));

module JSON = {
  let parse_action = a => {
    let debug = OSnap_Logger.debug(~header="Config.Test.parse_action");
    let action =
      a |> Yojson.Basic.Util.member("action") |> Yojson.Basic.Util.to_string;

    let size_restriction =
      a
      |> Yojson.Basic.Util.member("@")
      |> Yojson.Basic.Util.to_option(Yojson.Basic.Util.to_list)
      |> Option.map(List.map(Yojson.Basic.Util.to_string));

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
        Click(selector, size_restriction);
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
        Type(selector, text, size_restriction);
      };
    | "wait" =>
      debug("found wait action");
      switch (timeout) {
      | None =>
        debug("no timeout provided");
        raise(Invalid_format);
      | Some(timeout) =>
        debug(Printf.sprintf("timeout for wait action is %i", timeout));
        Wait(timeout, size_restriction);
      };
    | action =>
      debug(Printf.sprintf("found unknown action %S", action));
      raise(Invalid_format);
    };
  };

  let parse_ignore = r => {
    let debug = OSnap_Logger.debug(~header="Config.Test.parse_ignore");

    let size_restriction =
      r
      |> Yojson.Basic.Util.member("@")
      |> Yojson.Basic.Util.to_option(Yojson.Basic.Util.to_list)
      |> Option.map(List.map(Yojson.Basic.Util.to_string));

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
      Selector(selector, size_restriction);
    | (None, Some(x1), Some(y1), Some(x2), Some(y2)) =>
      debug(
        Printf.sprintf("using coordinates (%i,%i),(%i,%i)", x1, y1, x2, y2),
      );
      Coordinates((x1, y1), (x2, y2), size_restriction);
    | _ => raise(Invalid_format)
    };
  };

  let parse_single_test = (global_config: OSnap_Config_Types.global, test) => {
    let debug = OSnap_Logger.debug(~header="Config.Test.parse");
    try({
      let name =
        test
        |> Yojson.Basic.Util.member("name")
        |> Yojson.Basic.Util.to_string;

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
        |> Option.value(~default=global_config.threshold);

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
              Some(List.map(OSnap_Config_Utils.JSON.parse_size, list));
            }
          | _ => {
              debug("no sizes present. using default sizes");
              None;
            }
        )
        |> Option.value(~default=global_config.default_sizes);

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

      debug("looking for duplicate names in defined sizes");
      let duplicates =
        sizes
        |> List.filter((s: OSnap_Config_Types.size) =>
             Option.is_some(s.name)
           )
        |> OSnap_Utils.find_duplicates((s: OSnap_Config_Types.size) =>
             s.name
           )
        |> List.map((s: OSnap_Config_Types.size) => {
             let name = Option.value(s.name, ~default="");
             debug(
               Printf.sprintf("found size with duplicate name %S", name),
             );
             name;
           });

      if (List.length(duplicates) != 0) {
        raise(Duplicate_Size_Names(duplicates));
      } else {
        debug("did not find duplicates");
      };

      Result.ok({only, skip, threshold, name, url, sizes, actions, ignore});
    }) {
    | Duplicate_Size_Names(s) => raise(Duplicate_Size_Names(s))
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
};

module YAML = {
  let parse_action = a => {
    let debug = OSnap_Logger.debug(~header="Config.Test.YAML.parse_action");
    let action =
      a
      |> Yaml.Util.find_exn("action")
      |> Option.map(Yaml.Util.to_string_exn)
      |> (
        fun
        | Some(s) => s
        | None =>
          raise(Parse_Error("action type is required but not provided"))
      );

    let size_restriction =
      a
      |> Yaml.Util.find_exn("@")
      |> Option.map(
           fun
           | `A(lst) => lst |> List.map(Yaml.Util.to_string_exn)
           | _ => raise(Parse_Error("@ has to be an array of strings")),
         );

    let selector =
      a
      |> Yaml.Util.find_exn("selector")
      |> Option.map(Yaml.Util.to_string_exn);

    let text =
      a |> Yaml.Util.find_exn("text") |> Option.map(Yaml.Util.to_string_exn);

    let timeout =
      a
      |> Yaml.Util.find_exn("timeout")
      |> Option.map(Yaml.Util.to_float_exn)
      |> Option.map(Float.to_int);

    switch (action) {
    | "click" =>
      debug("found click action");
      switch (selector) {
      | None =>
        debug("no selector for click action provided");
        raise(Invalid_format);
      | Some(selector) =>
        debug(Printf.sprintf("click selector is %S", selector));
        Click(selector, size_restriction);
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
        Type(selector, text, size_restriction);
      };
    | "wait" =>
      debug("found wait action");
      switch (timeout) {
      | None =>
        debug("no timeout provided");
        raise(Invalid_format);
      | Some(timeout) =>
        debug(Printf.sprintf("timeout for wait action is %i", timeout));
        Wait(timeout, size_restriction);
      };
    | action =>
      debug(Printf.sprintf("found unknown action %S", action));
      raise(Invalid_format);
    };
  };

  let parse_ignore = r => {
    let debug = OSnap_Logger.debug(~header="Config.Test.YAML.parse_ignore");

    let size_restriction =
      r
      |> Yaml.Util.find_exn("@")
      |> Option.map(
           fun
           | `A(lst) => lst |> List.map(Yaml.Util.to_string_exn)
           | _ => raise(Parse_Error("@ has to be an array of strings")),
         );

    let x1 =
      r
      |> Yaml.Util.find_exn("x1")
      |> Option.map(Yaml.Util.to_float_exn)
      |> Option.map(Float.to_int);
    let y1 =
      r
      |> Yaml.Util.find_exn("y1")
      |> Option.map(Yaml.Util.to_float_exn)
      |> Option.map(Float.to_int);
    let x2 =
      r
      |> Yaml.Util.find_exn("x2")
      |> Option.map(Yaml.Util.to_float_exn)
      |> Option.map(Float.to_int);
    let y2 =
      r
      |> Yaml.Util.find_exn("y2")
      |> Option.map(Yaml.Util.to_float_exn)
      |> Option.map(Float.to_int);

    let selector =
      r
      |> Yaml.Util.find_exn("selector")
      |> Option.map(Yaml.Util.to_string_exn);

    switch (selector, x1, y1, x2, y2) {
    | (Some(selector), None, None, None, None) =>
      debug(Printf.sprintf("using selector %S", selector));
      Selector(selector, size_restriction);
    | (None, Some(x1), Some(y1), Some(x2), Some(y2)) =>
      debug(
        Printf.sprintf("using coordinates (%i,%i),(%i,%i)", x1, y1, x2, y2),
      );
      Coordinates((x1, y1), (x2, y2), size_restriction);
    | _ => raise(Invalid_format)
    };
  };

  let parse_single_test = (global_config: OSnap_Config_Types.global, test) => {
    let debug = OSnap_Logger.debug(~header="Config.Test.YAML.parse");
    try({
      let name =
        test
        |> Yaml.Util.find_exn("name")
        |> Option.map(Yaml.Util.to_string_exn)
        |> (
          fun
          | Some(s) => s
          | None => raise(Parse_Error("name is required but not provided"))
        );

      debug(Printf.sprintf("name: %S", name));

      let url =
        test
        |> Yaml.Util.find_exn("url")
        |> Option.map(Yaml.Util.to_string_exn)
        |> (
          fun
          | Some(s) => s
          | None => raise(Parse_Error("url is required but not provided"))
        );

      debug(Printf.sprintf("url: %s", url));

      let only =
        test
        |> Yaml.Util.find_exn("only")
        |> Option.map(Yaml.Util.to_bool_exn)
        |> Option.value(~default=false);

      debug(Printf.sprintf("only: %b", only));

      let skip =
        test
        |> Yaml.Util.find_exn("skip")
        |> Option.map(Yaml.Util.to_bool_exn)
        |> Option.value(~default=false);

      debug(Printf.sprintf("skip: %b", only));

      let threshold =
        test
        |> Yaml.Util.find_exn("threshold")
        |> Option.map(Yaml.Util.to_float_exn)
        |> Option.fold(~none=global_config.threshold, ~some=Float.to_int);

      debug(Printf.sprintf("threshold: %i", threshold));

      let sizes =
        test
        |> Yaml.Util.find_exn("sizes")
        |> Option.bind(
             _,
             fun
             | `A(list) => {
                 debug("parsing sizes");
                 Some(List.map(OSnap_Config_Utils.YAML.parse_size, list));
               }
             | _ => {
                 debug("no sizes present. using default sizes");
                 None;
               },
           )
        |> Option.value(~default=global_config.default_sizes);

      let actions =
        test
        |> Yaml.Util.find_exn("actions")
        |> Option.map(
             fun
             | `A(list) => {
                 debug("parsing actions");
                 List.map(parse_action, list);
               }
             | _ => [],
           )
        |> Option.value(~default=[]);

      let ignore =
        test
        |> Yaml.Util.find_exn("ignore")
        |> Option.map(
             fun
             | `A(list) => {
                 debug("parsing ignore regions");
                 List.map(parse_ignore, list);
               }
             | _ => [],
           )
        |> Option.value(~default=[]);

      debug("looking for duplicate names in defined sizes");
      let duplicates =
        sizes
        |> List.filter((s: OSnap_Config_Types.size) =>
             Option.is_some(s.name)
           )
        |> OSnap_Utils.find_duplicates((s: OSnap_Config_Types.size) =>
             s.name
           )
        |> List.map((s: OSnap_Config_Types.size) => {
             let name = Option.value(s.name, ~default="");
             debug(
               Printf.sprintf("found size with duplicate name %S", name),
             );
             name;
           });

      if (List.length(duplicates) != 0) {
        raise(Duplicate_Size_Names(duplicates));
      } else {
        debug("did not find duplicates");
      };

      Result.ok({only, skip, threshold, name, url, sizes, actions, ignore});
    }) {
    | Duplicate_Size_Names(s) => raise(Duplicate_Size_Names(s))
    | _ => raise(Invalid_format)
    };
  };

  let parse = (global_config, path) => {
    let debug = OSnap_Logger.debug(~header="Config.Test.YAML.parse");
    let config = OSnap_Utils.get_file_contents(path);
    debug(Printf.sprintf("parsing test file %S", path));

    try({
      let yaml = config |> Yaml.of_string_exn;

      let (tests, failed) =
        yaml
        |> (
          fun
          | `A(lst) => lst
          | _ => raise(Parse_Error("ignorePatterns has to be an array"))
        )
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
  )
  |> List.map(path => {
       let format = OSnap_Config_Utils.get_format(path);
       (path, format);
     });
};

let init = config => {
  let debug = OSnap_Logger.debug(~header="Config.Test.init");

  debug("looking for test files");
  let tests =
    find(
      ~root_path=config.root_path,
      ~pattern=config.test_pattern,
      ~ignore_patterns=config.ignore_patterns,
      (),
    )
    |> List.map(((path, test_format)) =>
         switch (test_format) {
         | OSnap_Config_Types.JSON => JSON.parse(config, path)
         | OSnap_Config_Types.YAML => YAML.parse(config, path)
         }
       )
    |> List.flatten;

  debug("looking for duplicate names in test files");
  let duplicates =
    tests
    |> OSnap_Utils.find_duplicates((t: OSnap_Config_Types.test) => t.name)
    |> List.map((t: OSnap_Config_Types.test) => {
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
