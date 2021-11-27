open OSnap_Config_Types;

let ( let* ) = Result.bind;

module Common = {
  let collect_duplicates = (~debug, sizes) => {
    debug("looking for duplicate names in defined sizes");
    let duplicates =
      sizes
      |> List.filter((s: OSnap_Config_Types.size) => Option.is_some(s.name))
      |> OSnap_Utils.find_duplicates((s: OSnap_Config_Types.size) => s.name)
      |> List.map((s: OSnap_Config_Types.size) => {
           let name = Option.value(s.name, ~default="");
           debug(Printf.sprintf("found size with duplicate name %S", name));
           name;
         });

    if (List.length(duplicates) != 0) {
      Result.error(OSnap_Response.Config_Duplicate_Size_Names(duplicates));
    } else {
      debug("did not find duplicates");
      Result.ok();
    };
  };

  let collect_action =
      (~debug, ~selector, ~size_restriction, ~text, ~timeout, action) => {
    switch (action) {
    | "click" =>
      debug("found click action");
      switch (selector) {
      | None =>
        Result.error(
          OSnap_Response.Config_Invalid(
            "no selector for click action provided",
            None,
          ),
        )
      | Some(selector) =>
        debug(Printf.sprintf("click selector is %S", selector));
        Click(selector, size_restriction) |> Result.ok;
      };
    | "type" =>
      debug("found type action");
      switch (selector, text) {
      | (None, _) =>
        debug("");
        Result.error(
          OSnap_Response.Config_Invalid(
            "no selector for type action provided",
            None,
          ),
        );
      | (_, None) =>
        Result.error(
          OSnap_Response.Config_Invalid(
            "no text for type action provided",
            None,
          ),
        )
      | (Some(selector), Some(text)) =>
        debug(
          Printf.sprintf(
            "type action selector is %S with text %S",
            selector,
            text,
          ),
        );
        Type(selector, text, size_restriction) |> Result.ok;
      };
    | "wait" =>
      debug("found wait action");
      switch (timeout) {
      | None =>
        Result.error(
          OSnap_Response.Config_Invalid(
            "no timeout for wait action provided",
            None,
          ),
        )
      | Some(timeout) =>
        debug(Printf.sprintf("timeout for wait action is %i", timeout));
        Wait(timeout, size_restriction) |> Result.ok;
      };
    | action =>
      Result.error(
        OSnap_Response.Config_Invalid(
          Printf.sprintf("found unknown action %S", action),
          None,
        ),
      )
    };
  };

  let collect_ignore = (~debug, ~size_restriction, selector, x1, y1, x2, y2) => {
    switch (selector, x1, y1, x2, y2) {
    | (Some(selector), None, None, None, None) =>
      debug(Printf.sprintf("using selector %S", selector));
      Selector(selector, size_restriction) |> Result.ok;
    | (None, Some(x1), Some(y1), Some(x2), Some(y2)) =>
      debug(
        Printf.sprintf("using coordinates (%i,%i),(%i,%i)", x1, y1, x2, y2),
      );
      Coordinates((x1, y1), (x2, y2), size_restriction) |> Result.ok;
    | _ =>
      Result.error(
        OSnap_Response.Config_Invalid(
          "Did not find a complete configuration for an ignore region.",
          None,
        ),
      )
    };
  };
};

module JSON = {
  let parse_action = a => {
    let debug = OSnap_Logger.debug(~header="Config.Test.parse_action");

    let* action =
      try(
        a
        |> Yojson.Basic.Util.member("action")
        |> Yojson.Basic.Util.to_string
        |> Result.ok
      ) {
      | Yojson.Basic.Util.Type_error(message, _) =>
        Result.error(OSnap_Response.Config_Parse_Error(message, None))
      };

    let* size_restriction =
      try(
        a
        |> Yojson.Basic.Util.member("@")
        |> Yojson.Basic.Util.to_option(Yojson.Basic.Util.to_list)
        |> Option.map(List.map(Yojson.Basic.Util.to_string))
        |> Result.ok
      ) {
      | Yojson.Basic.Util.Type_error(message, _) =>
        Result.error(OSnap_Response.Config_Parse_Error(message, None))
      };

    let* selector =
      try(
        a
        |> Yojson.Basic.Util.member("selector")
        |> Yojson.Basic.Util.to_string_option
        |> Result.ok
      ) {
      | Yojson.Basic.Util.Type_error(message, _) =>
        Result.error(OSnap_Response.Config_Parse_Error(message, None))
      };

    let* text =
      try(
        a
        |> Yojson.Basic.Util.member("text")
        |> Yojson.Basic.Util.to_string_option
        |> Result.ok
      ) {
      | Yojson.Basic.Util.Type_error(message, _) =>
        Result.error(OSnap_Response.Config_Parse_Error(message, None))
      };

    let* timeout =
      try(
        a
        |> Yojson.Basic.Util.member("timeout")
        |> Yojson.Basic.Util.to_int_option
        |> Result.ok
      ) {
      | Yojson.Basic.Util.Type_error(message, _) =>
        Result.error(OSnap_Response.Config_Parse_Error(message, None))
      };

    Common.collect_action(
      ~debug,
      ~selector,
      ~size_restriction,
      ~text,
      ~timeout,
      action,
    );
  };

  let parse_ignore = r => {
    let debug = OSnap_Logger.debug(~header="Config.Test.parse_ignore");

    let* size_restriction =
      try(
        r
        |> Yojson.Basic.Util.member("@")
        |> Yojson.Basic.Util.to_option(Yojson.Basic.Util.to_list)
        |> Option.map(List.map(Yojson.Basic.Util.to_string))
        |> Result.ok
      ) {
      | Yojson.Basic.Util.Type_error(message, _) =>
        Result.error(OSnap_Response.Config_Parse_Error(message, None))
      };

    let* x1 =
      try(
        r
        |> Yojson.Basic.Util.member("x1")
        |> Yojson.Basic.Util.to_int_option
        |> Result.ok
      ) {
      | Yojson.Basic.Util.Type_error(message, _) =>
        Result.error(OSnap_Response.Config_Parse_Error(message, None))
      };

    let* y1 =
      try(
        r
        |> Yojson.Basic.Util.member("y1")
        |> Yojson.Basic.Util.to_int_option
        |> Result.ok
      ) {
      | Yojson.Basic.Util.Type_error(message, _) =>
        Result.error(OSnap_Response.Config_Parse_Error(message, None))
      };

    let* x2 =
      try(
        r
        |> Yojson.Basic.Util.member("x2")
        |> Yojson.Basic.Util.to_int_option
        |> Result.ok
      ) {
      | Yojson.Basic.Util.Type_error(message, _) =>
        Result.error(OSnap_Response.Config_Parse_Error(message, None))
      };

    let* y2 =
      try(
        r
        |> Yojson.Basic.Util.member("y2")
        |> Yojson.Basic.Util.to_int_option
        |> Result.ok
      ) {
      | Yojson.Basic.Util.Type_error(message, _) =>
        Result.error(OSnap_Response.Config_Parse_Error(message, None))
      };

    let* selector =
      try(
        r
        |> Yojson.Basic.Util.member("selector")
        |> Yojson.Basic.Util.to_string_option
        |> Result.ok
      ) {
      | Yojson.Basic.Util.Type_error(message, _) =>
        Result.error(OSnap_Response.Config_Parse_Error(message, None))
      };

    Common.collect_ignore(
      ~debug,
      ~size_restriction,
      selector,
      x1,
      y1,
      x2,
      y2,
    );
  };

  let parse_single_test = (global_config: OSnap_Config_Types.global, test) => {
    let debug = OSnap_Logger.debug(~header="Config.Test.parse");

    let* name =
      try(
        test
        |> Yojson.Basic.Util.member("name")
        |> Yojson.Basic.Util.to_string
        |> Result.ok
      ) {
      | Yojson.Basic.Util.Type_error(message, _) =>
        Result.error(OSnap_Response.Config_Parse_Error(message, None))
      };

    debug(Printf.sprintf("name: %S", name));

    let* only =
      try(
        test
        |> Yojson.Basic.Util.member("only")
        |> Yojson.Basic.Util.to_bool_option
        |> Option.value(~default=false)
        |> Result.ok
      ) {
      | Yojson.Basic.Util.Type_error(message, _) =>
        Result.error(OSnap_Response.Config_Parse_Error(message, None))
      };

    debug(Printf.sprintf("only: %b", only));

    let* skip =
      try(
        test
        |> Yojson.Basic.Util.member("skip")
        |> Yojson.Basic.Util.to_bool_option
        |> Option.value(~default=false)
        |> Result.ok
      ) {
      | Yojson.Basic.Util.Type_error(message, _) =>
        Result.error(OSnap_Response.Config_Parse_Error(message, None))
      };

    debug(Printf.sprintf("skip: %b", only));

    let* threshold =
      try(
        test
        |> Yojson.Basic.Util.member("threshold")
        |> Yojson.Basic.Util.to_int_option
        |> Option.value(~default=global_config.threshold)
        |> Result.ok
      ) {
      | Yojson.Basic.Util.Type_error(message, _) =>
        Result.error(OSnap_Response.Config_Parse_Error(message, None))
      };

    debug(Printf.sprintf("threshold: %i", threshold));

    let* url =
      try(
        test
        |> Yojson.Basic.Util.member("url")
        |> Yojson.Basic.Util.to_string
        |> Result.ok
      ) {
      | Yojson.Basic.Util.Type_error(message, _) =>
        Result.error(OSnap_Response.Config_Parse_Error(message, None))
      };

    debug(Printf.sprintf("url: %s", url));

    let* sizes =
      test
      |> Yojson.Basic.Util.member("sizes")
      |> (
        fun
        | `Null => {
            debug("no sizes present. using default sizes");
            Result.ok(global_config.default_sizes);
          }
        | `List(list) => {
            debug("parsing sizes");
            list
            |> OSnap_Utils.List.map_until_exception(
                 OSnap_Config_Utils.JSON.parse_size,
               );
          }
        | _ => {
            Result.error(
              OSnap_Response.Config_Invalid(
                "sizes has an invalid format.",
                None,
              ),
            );
          }
      );

    let* actions =
      test
      |> Yojson.Basic.Util.member("actions")
      |> (
        fun
        | `List(list) => {
            debug("parsing actions");
            OSnap_Utils.List.map_until_exception(parse_action, list);
          }
        | _ => Result.ok([])
      );

    let* ignore =
      test
      |> Yojson.Basic.Util.member("ignore")
      |> (
        fun
        | `List(list) => {
            debug("parsing ignore regions");
            OSnap_Utils.List.map_until_exception(parse_ignore, list);
          }
        | _ => Result.ok([])
      );

    let* () = Common.collect_duplicates(~debug, sizes);

    Result.ok({only, skip, threshold, name, url, sizes, actions, ignore});
  };

  let parse = (global_config, path) => {
    let debug = OSnap_Logger.debug(~header="Config.Test.parse");
    let config = OSnap_Utils.get_file_contents(path);
    debug(Printf.sprintf("parsing test file %S", path));

    let json = config |> Yojson.Basic.from_string(~fname=path);

    try(
      json
      |> Yojson.Basic.Util.to_list
      |> OSnap_Utils.List.map_until_exception(
           parse_single_test(global_config),
         )
    ) {
    | Yojson.Basic.Util.Type_error(message, _) =>
      Result.error(OSnap_Response.Config_Parse_Error(message, None))
    };
  };
};

module YAML = {
  let parse_action = a => {
    let debug = OSnap_Logger.debug(~header="Config.Test.YAML.parse_action");

    let* action =
      a
      |> Yaml.Util.find("action")
      |> Result.map(Option.map(Yaml.Util.to_string))
      |> Result.map(OSnap_Config_Utils.to_result_option)
      |> Result.join
      |> (
        fun
        | Ok(Some(s)) => Result.ok(s)
        | Ok(None) =>
          Result.error(
            OSnap_Response.Config_Parse_Error(
              "action has an invalid format. Key \"action\" is required but not provided!",
              None,
            ),
          )
        | Error(`Msg(message)) =>
          Result.error(OSnap_Response.Config_Parse_Error(message, None))
      );

    let* size_restriction =
      a
      |> Yaml.Util.find("@")
      |> Result.map(
           Option.map(v => {
             switch (v) {
             | `A(l) => Result.ok(l)
             | _ => Result.error(`Msg("@ is in an invalid format."))
             }
           }),
         )
      |> Result.map(OSnap_Config_Utils.to_result_option)
      |> Result.join
      |> Result.map(
           Option.map(
             OSnap_Utils.List.map_until_exception(Yaml.Util.to_string),
           ),
         )
      |> Result.map(OSnap_Config_Utils.to_result_option)
      |> Result.join
      |> Result.map_error(
           fun
           | `Msg(message) =>
             OSnap_Response.Config_Parse_Error(message, None),
         );

    let* selector =
      a
      |> Yaml.Util.find("selector")
      |> Result.map(Option.map(Yaml.Util.to_string))
      |> Result.map(OSnap_Config_Utils.to_result_option)
      |> Result.join
      |> Result.map_error(
           fun
           | `Msg(message) =>
             OSnap_Response.Config_Parse_Error(message, None),
         );

    let* text =
      a
      |> Yaml.Util.find("text")
      |> Result.map(Option.map(Yaml.Util.to_string))
      |> Result.map(OSnap_Config_Utils.to_result_option)
      |> Result.join
      |> Result.map_error(
           fun
           | `Msg(message) =>
             OSnap_Response.Config_Parse_Error(message, None),
         );

    let* timeout =
      a
      |> Yaml.Util.find("timeout")
      |> Result.map(Option.map(Yaml.Util.to_float))
      |> Result.map(OSnap_Config_Utils.to_result_option)
      |> Result.join
      |> Result.map(Option.map(Float.to_int))
      |> Result.map_error(
           fun
           | `Msg(message) =>
             OSnap_Response.Config_Parse_Error(message, None),
         );

    Common.collect_action(
      ~debug,
      ~selector,
      ~size_restriction,
      ~text,
      ~timeout,
      action,
    );
  };

  let parse_ignore = r => {
    let debug = OSnap_Logger.debug(~header="Config.Test.YAML.parse_ignore");

    let* size_restriction =
      r
      |> Yaml.Util.find("@")
      |> Result.map(
           Option.map(v => {
             switch (v) {
             | `A(l) => Result.ok(l)
             | _ => Result.error(`Msg("@ is in an invalid format."))
             }
           }),
         )
      |> Result.map(OSnap_Config_Utils.to_result_option)
      |> Result.join
      |> Result.map(
           Option.map(
             OSnap_Utils.List.map_until_exception(Yaml.Util.to_string),
           ),
         )
      |> Result.map(OSnap_Config_Utils.to_result_option)
      |> Result.join
      |> Result.map_error(
           fun
           | `Msg(message) =>
             OSnap_Response.Config_Parse_Error(message, None),
         );

    let* x1 =
      r
      |> Yaml.Util.find("x1")
      |> Result.map(Option.map(Yaml.Util.to_float))
      |> Result.map(OSnap_Config_Utils.to_result_option)
      |> Result.join
      |> Result.map(Option.map(Float.to_int))
      |> Result.map_error(
           fun
           | `Msg(message) =>
             OSnap_Response.Config_Parse_Error(message, None),
         );
    let* y1 =
      r
      |> Yaml.Util.find("y1")
      |> Result.map(Option.map(Yaml.Util.to_float))
      |> Result.map(OSnap_Config_Utils.to_result_option)
      |> Result.join
      |> Result.map(Option.map(Float.to_int))
      |> Result.map_error(
           fun
           | `Msg(message) =>
             OSnap_Response.Config_Parse_Error(message, None),
         );
    let* x2 =
      r
      |> Yaml.Util.find("x2")
      |> Result.map(Option.map(Yaml.Util.to_float))
      |> Result.map(OSnap_Config_Utils.to_result_option)
      |> Result.join
      |> Result.map(Option.map(Float.to_int))
      |> Result.map_error(
           fun
           | `Msg(message) =>
             OSnap_Response.Config_Parse_Error(message, None),
         );
    let* y2 =
      r
      |> Yaml.Util.find("y2")
      |> Result.map(Option.map(Yaml.Util.to_float))
      |> Result.map(OSnap_Config_Utils.to_result_option)
      |> Result.join
      |> Result.map(Option.map(Float.to_int))
      |> Result.map_error(
           fun
           | `Msg(message) =>
             OSnap_Response.Config_Parse_Error(message, None),
         );

    let* selector =
      r
      |> Yaml.Util.find("selector")
      |> Result.map(Option.map(Yaml.Util.to_string))
      |> Result.map(OSnap_Config_Utils.to_result_option)
      |> Result.join
      |> Result.map_error(
           fun
           | `Msg(message) =>
             OSnap_Response.Config_Parse_Error(message, None),
         );

    Common.collect_ignore(
      ~debug,
      ~size_restriction,
      selector,
      x1,
      y1,
      x2,
      y2,
    );
  };

  let parse_single_test = (global_config: OSnap_Config_Types.global, test) => {
    let debug = OSnap_Logger.debug(~header="Config.Test.YAML.parse");

    let* name =
      test
      |> Yaml.Util.find("name")
      |> Result.map(Option.map(Yaml.Util.to_string))
      |> Result.map(OSnap_Config_Utils.to_result_option)
      |> Result.join
      |> (
        fun
        | Ok(Some(name)) => Result.ok(name)
        | Ok(None) =>
          Result.error(
            OSnap_Response.Config_Parse_Error(
              "name is required but not provided",
              None,
            ),
          )
        | Error(`Msg(message)) =>
          Result.error(OSnap_Response.Config_Parse_Error(message, None))
      );

    debug(Printf.sprintf("name: %S", name));

    let* url =
      test
      |> Yaml.Util.find("url")
      |> Result.map(Option.map(Yaml.Util.to_string))
      |> Result.map(OSnap_Config_Utils.to_result_option)
      |> Result.join
      |> (
        fun
        | Ok(Some(name)) => Result.ok(name)
        | Ok(None) =>
          Result.error(
            OSnap_Response.Config_Parse_Error(
              "url is required but not provided",
              None,
            ),
          )
        | Error(`Msg(message)) =>
          Result.error(OSnap_Response.Config_Parse_Error(message, None))
      );

    debug(Printf.sprintf("url: %s", url));

    let* only =
      test
      |> Yaml.Util.find("only")
      |> Result.map(Option.map(Yaml.Util.to_bool))
      |> Result.map(OSnap_Config_Utils.to_result_option)
      |> Result.join
      |> Result.map(Option.value(~default=false))
      |> Result.map_error(
           fun
           | `Msg(message) =>
             OSnap_Response.Config_Parse_Error(message, None),
         );

    debug(Printf.sprintf("only: %b", only));

    let* skip =
      test
      |> Yaml.Util.find("skip")
      |> Result.map(Option.map(Yaml.Util.to_bool))
      |> Result.map(OSnap_Config_Utils.to_result_option)
      |> Result.join
      |> Result.map(Option.value(~default=false))
      |> Result.map_error(
           fun
           | `Msg(message) =>
             OSnap_Response.Config_Parse_Error(message, None),
         );

    debug(Printf.sprintf("skip: %b", only));

    let* threshold =
      test
      |> Yaml.Util.find("threshold")
      |> Result.map(Option.map(Yaml.Util.to_float))
      |> Result.map(OSnap_Config_Utils.to_result_option)
      |> Result.join
      |> Result.map(
           Option.fold(~none=global_config.threshold, ~some=Float.to_int),
         )
      |> Result.map_error(
           fun
           | `Msg(message) =>
             OSnap_Response.Config_Parse_Error(message, None),
         );

    debug(Printf.sprintf("threshold: %i", threshold));

    let* sizes =
      test
      |> Yaml.Util.find("sizes")
      |> Result.map(
           Option.map(
             fun
             | `Null => {
                 debug("no sizes present. using default sizes");
                 Result.ok(global_config.default_sizes);
               }
             | `A(list) => {
                 debug("parsing sizes");
                 list
                 |> OSnap_Utils.List.map_until_exception(
                      OSnap_Config_Utils.YAML.parse_size,
                    );
               }
             | _ =>
               Result.error(
                 OSnap_Response.Config_Invalid(
                   "sizes has an invalid format.",
                   None,
                 ),
               ),
           ),
         )
      |> Result.map_error(
           fun
           | `Msg(message) =>
             OSnap_Response.Config_Parse_Error(message, None),
         )
      |> Result.map(OSnap_Config_Utils.to_result_option)
      |> Result.join
      |> Result.map(Option.value(~default=global_config.default_sizes));

    let* actions =
      test
      |> Yaml.Util.find("actions")
      |> Result.map(
           Option.map(
             fun
             | `Null => {
                 Result.ok([]);
               }
             | `A(list) => {
                 debug("parsing actions");
                 list |> OSnap_Utils.List.map_until_exception(parse_action);
               }
             | _ =>
               Result.error(
                 OSnap_Response.Config_Invalid(
                   "sizes has an invalid format.",
                   None,
                 ),
               ),
           ),
         )
      |> Result.map_error(
           fun
           | `Msg(message) =>
             OSnap_Response.Config_Parse_Error(message, None),
         )
      |> Result.map(OSnap_Config_Utils.to_result_option)
      |> Result.join
      |> Result.map(Option.value(~default=[]));

    let* ignore =
      test
      |> Yaml.Util.find("ignore")
      |> Result.map(
           Option.map(
             fun
             | `Null => {
                 Result.ok([]);
               }
             | `A(list) => {
                 debug("parsing ignore regions");
                 list |> OSnap_Utils.List.map_until_exception(parse_ignore);
               }
             | _ =>
               Result.error(
                 OSnap_Response.Config_Invalid(
                   "sizes has an invalid format.",
                   None,
                 ),
               ),
           ),
         )
      |> Result.map_error(
           fun
           | `Msg(message) =>
             OSnap_Response.Config_Parse_Error(message, None),
         )
      |> Result.map(OSnap_Config_Utils.to_result_option)
      |> Result.join
      |> Result.map(Option.value(~default=[]));

    let* () = Common.collect_duplicates(~debug, sizes);

    Result.ok({only, skip, threshold, name, url, sizes, actions, ignore});
  };

  let parse = (global_config, path) => {
    let debug = OSnap_Logger.debug(~header="Config.Test.YAML.parse");
    let config = OSnap_Utils.get_file_contents(path);
    debug(Printf.sprintf("parsing test file %S", path));

    let* yaml =
      config
      |> Yaml.of_string
      |> Result.map_error(_ =>
           OSnap_Response.Config_Parse_Error(
             Printf.sprintf("YAML could not be parsed"),
             Some(path),
           )
         );

    yaml
    |> (
      fun
      | `A(lst) => Result.ok(lst)
      | _ =>
        Result.error(
          OSnap_Response.Config_Parse_Error(
            "A test file has to be an array of tests.",
            Some(path),
          ),
        )
    )
    |> Result.map(
         OSnap_Utils.List.map_until_exception(
           parse_single_test(global_config),
         ),
       )
    |> Result.join
    |> Result.map_error(err => {
         switch (err) {
         | OSnap_Response.Config_Parse_Error(err, None) =>
           OSnap_Response.Config_Parse_Error(err, Some(path))
         | OSnap_Response.Config_Parse_Error(err, Some(path)) =>
           OSnap_Response.Config_Parse_Error(err, Some(path))
         | OSnap_Response.Config_Global_Not_Found => OSnap_Response.Config_Global_Not_Found
         | OSnap_Response.Config_Unsupported_Format(f) =>
           OSnap_Response.Config_Unsupported_Format(f)
         | OSnap_Response.Config_Invalid(msg, None) =>
           OSnap_Response.Config_Invalid(msg, Some(path))
         | OSnap_Response.Config_Invalid(msg, Some(path)) =>
           OSnap_Response.Config_Invalid(msg, Some(path))
         | OSnap_Response.Config_Duplicate_Tests(t) =>
           OSnap_Response.Config_Duplicate_Tests(t)
         | OSnap_Response.Config_Duplicate_Size_Names(n) =>
           OSnap_Response.Config_Duplicate_Size_Names(n)
         | OSnap_Response.CDP_Protocol_Error(e) =>
           OSnap_Response.CDP_Protocol_Error(e)
         | OSnap_Response.CDP_Connection_Failed => OSnap_Response.CDP_Connection_Failed
         | OSnap_Response.Invalid_Run(s) => OSnap_Response.Invalid_Run(s)
         | OSnap_Response.FS_Error(e) => OSnap_Response.FS_Error(e)
         | OSnap_Response.Test_Failure => OSnap_Response.Test_Failure
         | OSnap_Response.Unknown_Error(e) => OSnap_Response.Unknown_Error(e)
         }
       });
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
  |> OSnap_Utils.List.map_until_exception(path => {
       let* format = OSnap_Config_Utils.get_format(path);
       Result.ok((path, format));
     });
};

let init = config => {
  let debug = OSnap_Logger.debug(~header="Config.Test.init");

  debug("looking for test files");
  let* tests =
    find(
      ~root_path=config.root_path,
      ~pattern=config.test_pattern,
      ~ignore_patterns=config.ignore_patterns,
      (),
    )
    |> Result.bind(
         _,
         OSnap_Utils.List.map_until_exception(((path, test_format)) =>
           switch (test_format) {
           | OSnap_Config_Types.JSON => JSON.parse(config, path)
           | OSnap_Config_Types.YAML => YAML.parse(config, path)
           }
         ),
       )
    |> Result.map(List.flatten);

  debug("looking for duplicate names in test files");
  let duplicates =
    tests
    |> OSnap_Utils.find_duplicates((t: OSnap_Config_Types.test) => t.name)
    |> List.map((t: OSnap_Config_Types.test) => {
         debug(Printf.sprintf("found test with duplicate name %S", t.name));
         t.name;
       });

  if (List.length(duplicates) != 0) {
    Result.error(OSnap_Response.Config_Duplicate_Tests(duplicates));
  } else {
    debug("did not find duplicates");
    Result.ok(tests);
  };
};
