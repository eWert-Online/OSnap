let get_format = path =>
  path
  |> Filename.extension
  |> String.lowercase_ascii
  |> (
    fun
    | ".json" => OSnap_Config_Types.JSON
    | ".yaml" => OSnap_Config_Types.YAML
    | _ => raise(OSnap_Config_Types.Unsupported_Format)
  );

module JSON = {
  let parse_size = size => {
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
  let parse_size = size => {
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
