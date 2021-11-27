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

  let parse_size_exn = size => {
    let debug = OSnap_Logger.debug(~header="Config.Test.parse_size");
    let name =
      size
      |> Yojson.Basic.Util.member("name")
      |> Yojson.Basic.Util.to_string_option;

    let width =
      size |> Yojson.Basic.Util.member("width") |> Yojson.Basic.Util.to_int;

    let height =
      size |> Yojson.Basic.Util.member("height") |> Yojson.Basic.Util.to_int;

    debug(Printf.sprintf("size is set to %ix%i", width, height));

    OSnap_Config_Types.{name, width, height};
  };
};

module YAML = {
  let ( let* ) = Result.bind;
  let parse_size = size => {
    let debug = OSnap_Logger.debug(~header="Config.Test.parse_size");

    let* name =
      size
      |> Yaml.Util.find("name")
      |> Result.map(Option.map(Yaml.Util.to_string))
      |> Result.map(to_result_option)
      |> Result.join
      |> Result.map_error(
           fun
           | `Msg(message) =>
             OSnap_Response.Config_Parse_Error(message, None),
         );

    let* height =
      size
      |> Yaml.Util.find("height")
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
              "defaultSize has an invalid format. \"height\" is required but not provided!",
              None,
            ),
          )
        | Error(`Msg(message)) =>
          Result.error(OSnap_Response.Config_Parse_Error(message, None))
      );

    let* width =
      size
      |> Yaml.Util.find("width")
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
              "defaultSize has an invalid format. \"width\" is required but not provided!",
              None,
            ),
          )
        | Error(`Msg(message)) =>
          Result.error(OSnap_Response.Config_Parse_Error(message, None))
      );

    debug(Printf.sprintf("adding default size %ix%i", width, height));

    OSnap_Config_Types.{name, width, height} |> Result.ok;
  };

  let parse_size_exn = size => {
    let debug = OSnap_Logger.debug(~header="Config.Test.parse_size");
    let name =
      size
      |> Yaml.Util.find_exn("name")
      |> Option.map(Yaml.Util.to_string_exn);

    let height =
      size
      |> Yaml.Util.find_exn("height")
      |> Option.map(Yaml.Util.to_float_exn)
      |> Option.map(Float.to_int)
      |> (
        fun
        | Some(f) => f
        | None =>
          raise(
            OSnap_Config_Types.Parse_Error(
              "defaultSize has an invalid format. \"height\" is required but not provided!",
            ),
          )
      );

    let width =
      size
      |> Yaml.Util.find_exn("width")
      |> Option.map(Yaml.Util.to_float_exn)
      |> Option.map(Float.to_int)
      |> (
        fun
        | Some(f) => f
        | None =>
          raise(
            OSnap_Config_Types.Parse_Error(
              "defaultSize has an invalid format. \"width\" is required but not provided!",
            ),
          )
      );

    debug(Printf.sprintf("adding default size %ix%i", width, height));

    OSnap_Config_Types.{name, width, height};
  };
};
