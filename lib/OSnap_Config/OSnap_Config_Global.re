open OSnap_Config_Types;

exception Parse_Error(string);
exception No_Config_Found;

let parse = path => {
  let debug = OSnap_Logger.debug(~header="Config.Global.parse");

  let config = OSnap_Utils.get_file_contents(path);

  try({
    let json = config |> Yojson.Basic.from_string;

    let base_url =
      json
      |> Yojson.Basic.Util.member("baseUrl")
      |> Yojson.Basic.Util.to_string;

    debug(Printf.sprintf("baseUrl is set to %S", base_url));

    let fullscreen =
      json
      |> Yojson.Basic.Util.member("fullScreen")
      |> Yojson.Basic.Util.to_bool_option
      |> Option.value(~default=false);

    debug(Printf.sprintf("fullScreen is set to %b", fullscreen));

    let threshold =
      json
      |> Yojson.Basic.Util.member("threshold")
      |> Yojson.Basic.Util.to_int_option
      |> Option.value(~default=0);

    debug(Printf.sprintf("threshold is set to %i", threshold));

    let default_sizes =
      json
      |> Yojson.Basic.Util.member("defaultSizes")
      |> Yojson.Basic.Util.to_list
      |> List.map(item => {
           let name =
             item
             |> Yojson.Basic.Util.member("name")
             |> Yojson.Basic.Util.to_string_option;

           let height =
             item
             |> Yojson.Basic.Util.member("height")
             |> Yojson.Basic.Util.to_int;

           let width =
             item
             |> Yojson.Basic.Util.member("width")
             |> Yojson.Basic.Util.to_int;

           debug(Printf.sprintf("adding default size %ix%i", width, height));
           {name, width, height};
         });

    let snapshot_directory =
      json
      |> Yojson.Basic.Util.member("snapshotDirectory")
      |> Yojson.Basic.Util.to_string_option
      |> Option.value(~default="__snapshots__");

    debug(
      Printf.sprintf("snapshot directory is set to %s", snapshot_directory),
    );

    let parallelism =
      json
      |> Yojson.Basic.Util.member("parallelism")
      |> Yojson.Basic.Util.to_int_option
      |> Option.value(~default=8)
      |> max(1);

    debug(Printf.sprintf("parallelism is set to %i", parallelism));

    let root_path =
      String.sub(
        path,
        0,
        String.length(path) - String.length("osnap.config.json"),
      );

    debug(Printf.sprintf("setting root path to %s", root_path));

    let ignore_patterns =
      json
      |> Yojson.Basic.Util.member("ignorePatterns")
      |> (
        fun
        | `List(list) => list |> List.map(Yojson.Basic.Util.to_string)
        | _ => ["**/node_modules/**"]
      );

    debug(
      Printf.sprintf(
        "ignore_patterns are set to %s",
        List.fold_left(
          (curr, acc) => acc ++ " " ++ curr,
          "",
          ignore_patterns,
        ),
      ),
    );

    let test_pattern =
      json
      |> Yojson.Basic.Util.member("testPattern")
      |> Yojson.Basic.Util.to_string_option
      |> Option.value(~default="**/*.osnap.json");

    debug(Printf.sprintf("test pattern is set to %s", test_pattern));

    let diff_pixel_color =
      json
      |> Yojson.Basic.Util.member("diffPixelColor")
      |> (
        fun
        | `Assoc(_) as assoc => {
            let get_color = (
              fun
              | `Int(i) => i
              | `Float(f) => int_of_float(f)
              | `String(s) => int_of_string(s)
              | _ =>
                raise(
                  Parse_Error(
                    "diffPixelColor does not have a correct format",
                  ),
                )
            );
            (
              assoc |> Yojson.Basic.Util.member("r") |> get_color,
              assoc |> Yojson.Basic.Util.member("g") |> get_color,
              assoc |> Yojson.Basic.Util.member("b") |> get_color,
            );
          }
        | `Null => (255, 0, 0)
        | _ =>
          raise(Parse_Error("diffPixelColor does not have a correct format"))
      );

    let (r, g, b) = diff_pixel_color;
    debug(Printf.sprintf("diff pixel color is set to %i,%i,%i", r, g, b));

    debug("looking for duplicate names in defined sizes");
    let duplicates =
      default_sizes
      |> List.filter((s: OSnap_Config_Types.size) => Option.is_some(s.name))
      |> OSnap_Utils.find_duplicates((s: OSnap_Config_Types.size) => s.name)
      |> List.map((s: OSnap_Config_Types.size) => {
           let name = Option.value(s.name, ~default="");
           debug(Printf.sprintf("found size with duplicate name %S", name));
           name;
         });

    if (List.length(duplicates) != 0) {
      raise(Duplicate_Size_Names(duplicates));
    } else {
      debug("did not find duplicates");
    };

    {
      root_path,
      threshold,
      test_pattern,
      ignore_patterns,
      base_url,
      fullscreen,
      default_sizes,
      snapshot_directory,
      diff_pixel_color,
      parallelism,
    };
  }) {
  | Yojson.Basic.Util.Type_error(msg, _) => raise(Parse_Error(msg))
  };
};

let find = (~config_path) => {
  let debug = OSnap_Logger.debug(~header="Config.Global.find");

  let rec scan_dir = (~config_name, segments) => {
    let current_path = segments |> OSnap_Utils.path_of_segments;
    let elements = current_path |> Sys.readdir |> Array.to_list;

    debug(Printf.sprintf("looking for %S in %S", config_name, current_path));

    let files =
      elements
      |> List.find_all(el => {
           let path = OSnap_Utils.path_of_segments([el, ...segments]);
           let is_direcoty = path |> Sys.is_directory;
           !is_direcoty;
         });

    debug(
      Printf.sprintf("found %i files in this directory", List.length(files)),
    );
    let exists = files |> List.exists(file => file == config_name);
    if (exists) {
      Some(segments);
    } else {
      let parent_dir_segments = ["..", ...segments];
      let parent_dir = parent_dir_segments |> OSnap_Utils.path_of_segments;

      debug("did not find a config file in this directory");

      try(
        if (parent_dir |> Sys.is_directory) {
          debug("looking in parent directory");
          scan_dir(~config_name, parent_dir_segments);
        } else {
          debug("there is no parent directory anymore");
          None;
        }
      ) {
      | Sys_error(_) => None
      };
    };
  };

  if (config_path == "") {
    let config_name = "osnap.config.json";
    let base_path = Sys.getcwd();

    debug(
      Printf.sprintf("looking for %S in %S and up.", config_name, base_path),
    );

    let config_path = scan_dir(~config_name, [base_path]);
    switch (config_path) {
    | None =>
      debug("no config file was found");
      raise(No_Config_Found);
    | Some(paths) =>
      let path = [config_name, ...paths] |> OSnap_Utils.path_of_segments;
      debug(Printf.sprintf("found config file at %S", path));
      path;
    };
  } else {
    debug(Printf.sprintf("using provided config path %S", config_path));
    config_path;
  };
};
