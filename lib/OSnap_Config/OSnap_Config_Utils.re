let get_format = path =>
  path
  |> Filename.extension
  |> String.lowercase_ascii
  |> (
    fun
    | ".json" => Result.ok(OSnap_Config_Types.JSON)
    | ".yaml" => Result.ok(OSnap_Config_Types.YAML)
    | _ => Result.error(OSnap_Response.Config_Unsupported_Format(path))
  );

let to_result_option =
  fun
  | None => Ok(None)
  | Some(Ok(v)) => Ok(Some(v))
  | Some(Error(e)) => Error(e);

let collect_action =
    (~debug, ~selector, ~size_restriction, ~name, ~text, ~timeout, action) => {
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
      OSnap_Config_Types.Click(selector, size_restriction) |> Result.ok;
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
      OSnap_Config_Types.Type(selector, text, size_restriction) |> Result.ok;
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
      OSnap_Config_Types.Wait(timeout, size_restriction) |> Result.ok;
    };
  | "function" =>
    debug("found function action");
    switch (name) {
    | None =>
      Result.error(
        OSnap_Response.Config_Invalid(
          "no name for function action provided",
          None,
        ),
      )
    | Some(name) =>
      debug(Printf.sprintf("name for function action is %s", name));
      OSnap_Config_Types.Function(name, size_restriction) |> Result.ok;
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

module JSON = {
  let ( let* ) = Result.bind;
  let parse_size = size => {
    let debug = OSnap_Logger.debug(~header="Config.Test.parse_size");

    let* name =
      try(
        size
        |> Yojson.Basic.Util.member("name")
        |> Yojson.Basic.Util.to_string_option
        |> Result.ok
      ) {
      | Yojson.Basic.Util.Type_error(message, _) =>
        Result.error(OSnap_Response.Config_Parse_Error(message, None))
      };

    let* width =
      try(
        size
        |> Yojson.Basic.Util.member("width")
        |> (
          fun
          | `Null =>
            Result.error(
              OSnap_Response.Config_Parse_Error(
                "defaultSize has an invalid format. \"width\" is required but not provided!",
                None,
              ),
            )
          | v => Result.ok(v)
        )
        |> Result.map(Yojson.Basic.Util.to_int)
      ) {
      | Yojson.Basic.Util.Type_error(message, _) =>
        Result.error(OSnap_Response.Config_Parse_Error(message, None))
      };

    let* height =
      try(
        size
        |> Yojson.Basic.Util.member("height")
        |> (
          fun
          | `Null =>
            Result.error(
              OSnap_Response.Config_Parse_Error(
                "defaultSize has an invalid format. \"height\" is required but not provided!",
                None,
              ),
            )
          | v => Result.ok(v)
        )
        |> Result.map(Yojson.Basic.Util.to_int)
      ) {
      | Yojson.Basic.Util.Type_error(message, _) =>
        Result.error(OSnap_Response.Config_Parse_Error(message, None))
      };

    debug(Printf.sprintf("size is set to %ix%i", width, height));

    OSnap_Config_Types.{name, width, height} |> Result.ok;
  };

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

    let* name =
      try(
        a
        |> Yojson.Basic.Util.member("name")
        |> Yojson.Basic.Util.to_string_option
        |> Result.ok
      ) {
      | Yojson.Basic.Util.Type_error(message, _) =>
        Result.error(OSnap_Response.Config_Parse_Error(message, None))
      };

    collect_action(
      ~debug,
      ~selector,
      ~size_restriction,
      ~text,
      ~timeout,
      ~name,
      action,
    );
  };
};

module YAML = {
  let ( let* ) = Result.bind;

  let get_list_option = (~parser, key, obj) => {
    obj
    |> Yaml.Util.find(key)
    |> Result.map_error(
         fun
         | `Msg(message) => OSnap_Response.Config_Parse_Error(message, None),
       )
    |> Result.map(
         Option.map(v => {
           switch (v) {
           | `A(l) => Result.ok(l)
           | `Bool(_) =>
             Result.error(
               OSnap_Response.Config_Parse_Error(
                 Printf.sprintf(
                   "%S is in an invalid format. Expected array, got boolean.",
                   key,
                 ),
                 None,
               ),
             )
           | `Float(_) =>
             Result.error(
               OSnap_Response.Config_Parse_Error(
                 Printf.sprintf(
                   "%S is in an invalid format. Expected array, got number.",
                   key,
                 ),
                 None,
               ),
             )
           | `Null =>
             Result.error(
               OSnap_Response.Config_Parse_Error(
                 Printf.sprintf(
                   "%S is in an invalid format. Expected array, got null.",
                   key,
                 ),
                 None,
               ),
             )
           | `O(_) =>
             Result.error(
               OSnap_Response.Config_Parse_Error(
                 Printf.sprintf(
                   "%S is in an invalid format. Expected array, got object.",
                   key,
                 ),
                 None,
               ),
             )
           | `String(_) =>
             Result.error(
               OSnap_Response.Config_Parse_Error(
                 Printf.sprintf(
                   "%S is in an invalid format. Expected array, got string.",
                   key,
                 ),
                 None,
               ),
             )
           }
         }),
       )
    |> Result.map(to_result_option)
    |> Result.join
    |> Result.map(Option.map(OSnap_Utils.List.map_until_exception(parser)))
    |> Result.map(to_result_option)
    |> Result.join;
  };

  let get_string_list_option = (key, obj) => {
    let parser = v =>
      Yaml.Util.to_string(v)
      |> Result.map_error(
           fun
           | `Msg(message) =>
             OSnap_Response.Config_Parse_Error(message, None),
         );

    get_list_option(~parser, key, obj);
  };

  let get_string_option = (key, obj) => {
    obj
    |> Yaml.Util.find(key)
    |> Result.map(Option.map(Yaml.Util.to_string))
    |> Result.map(to_result_option)
    |> Result.join
    |> Result.map_error(
         fun
         | `Msg(message) => OSnap_Response.Config_Parse_Error(message, None),
       );
  };

  let get_string = (~additional_error_message="", key, obj) => {
    obj
    |> Yaml.Util.find(key)
    |> Result.map(Option.map(Yaml.Util.to_string))
    |> Result.map(to_result_option)
    |> Result.join
    |> (
      fun
      | Ok(Some(string)) => Result.ok(string)
      | Ok(None) =>
        Result.error(
          OSnap_Response.Config_Parse_Error(
            Printf.sprintf(
              "%S is required but not provided! %s",
              key,
              additional_error_message,
            ),
            None,
          ),
        )
      | Error(`Msg(message)) =>
        Result.error(OSnap_Response.Config_Parse_Error(message, None))
    );
  };

  let get_bool_option = (key, obj) => {
    obj
    |> Yaml.Util.find(key)
    |> Result.map(Option.map(Yaml.Util.to_bool))
    |> Result.map(to_result_option)
    |> Result.join
    |> Result.map_error(
         fun
         | `Msg(message) => OSnap_Response.Config_Parse_Error(message, None),
       );
  };

  let get_bool = (~additional_error_message="", key, obj) => {
    obj
    |> Yaml.Util.find(key)
    |> Result.map(Option.map(Yaml.Util.to_bool))
    |> Result.map(to_result_option)
    |> Result.join
    |> (
      fun
      | Ok(Some(string)) => Result.ok(string)
      | Ok(None) =>
        Result.error(
          OSnap_Response.Config_Parse_Error(
            Printf.sprintf(
              "%S is required but not provided! %s",
              key,
              additional_error_message,
            ),
            None,
          ),
        )
      | Error(`Msg(message)) =>
        Result.error(OSnap_Response.Config_Parse_Error(message, None))
    );
  };

  let get_int = (~additional_error_message="", key, obj) => {
    obj
    |> Yaml.Util.find(key)
    |> Result.map(Option.map(Yaml.Util.to_float))
    |> Result.map(to_result_option)
    |> Result.join
    |> Result.map(Option.map(Float.to_int))
    |> (
      fun
      | Ok(Some(number)) => Result.ok(number)
      | Ok(None) =>
        Result.error(
          OSnap_Response.Config_Parse_Error(
            Printf.sprintf(
              "%S is required but not provided! %s",
              key,
              additional_error_message,
            ),
            None,
          ),
        )
      | Error(`Msg(message)) =>
        Result.error(OSnap_Response.Config_Parse_Error(message, None))
    );
  };

  let get_int_option = (key, obj) => {
    obj
    |> Yaml.Util.find(key)
    |> Result.map(Option.map(Yaml.Util.to_float))
    |> Result.map(to_result_option)
    |> Result.join
    |> Result.map(Option.map(Float.to_int))
    |> Result.map_error(
         fun
         | `Msg(message) => OSnap_Response.Config_Parse_Error(message, None),
       );
  };

  let parse_size = size => {
    let debug = OSnap_Logger.debug(~header="Config.Test.parse_size");

    let* name = size |> get_string_option("name");
    let* height = size |> get_int("height");
    let* width = size |> get_int("width");

    debug(Printf.sprintf("adding size %ix%i", width, height));

    OSnap_Config_Types.{name, width, height} |> Result.ok;
  };

  let parse_action = a => {
    let debug = OSnap_Logger.debug(~header="Config.Test.YAML.parse_action");

    let* size_restriction = a |> get_string_list_option("@");

    let* action = a |> get_string("action");
    let* selector = a |> get_string_option("selector");
    let* name = a |> get_string_option("name");
    let* text = a |> get_string_option("text");
    let* timeout = a |> get_int_option("timeout");

    collect_action(
      ~debug,
      ~selector,
      ~size_restriction,
      ~text,
      ~name,
      ~timeout,
      action,
    );
  };
};
