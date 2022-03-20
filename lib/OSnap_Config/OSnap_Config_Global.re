open OSnap_Config_Types;

module YAML = {
  let ( let* ) = Result.bind;
  let parse = path => {
    let debug = OSnap_Logger.debug(~header="Config.Global.YAML.parse");

    let config = OSnap_Utils.get_file_contents(path);

    let* yaml =
      config
      |> Yaml.of_string
      |> Result.map_error(_ =>
           OSnap_Response.Config_Parse_Error(
             Printf.sprintf("YAML could not be parsed"),
             Some(path),
           )
         );

    let* base_url = yaml |> OSnap_Config_Utils.YAML.get_string("baseUrl");
    debug(Printf.sprintf("baseUrl is set to %S", base_url));

    let* fullscreen =
      yaml
      |> OSnap_Config_Utils.YAML.get_bool_option("fullScreen")
      |> Result.map(Option.value(~default=false));
    debug(Printf.sprintf("fullScreen is set to %b", fullscreen));

    let* threshold =
      yaml
      |> OSnap_Config_Utils.YAML.get_int_option("threshold")
      |> Result.map(Option.value(~default=0));
    debug(Printf.sprintf("threshold is set to %i", threshold));

    let* ignore_patterns =
      yaml
      |> OSnap_Config_Utils.YAML.get_string_list_option("ignorePatterns")
      |> Result.map(Option.value(~default=["**/node_modules/**"]));
    debug(
      Printf.sprintf(
        "ignore_patterns are set to %s",
        String.concat(",", ignore_patterns),
      ),
    );

    let* default_sizes =
      yaml
      |> OSnap_Config_Utils.YAML.get_list_option(
           "defaultSizes",
           ~parser=OSnap_Config_Utils.YAML.parse_size,
         )
      |> Result.map(Option.value(~default=[]));

    let* functions = {
      let f = yaml |> Yaml.Util.find_exn("functions");
      f
      |> Option.map(f => {
           f
           |> Yaml.Util.keys_exn
           |> OSnap_Utils.List.map_until_exception(key => {
                let* actions =
                  f
                  |> OSnap_Config_Utils.YAML.get_list_option(
                       key,
                       ~parser=OSnap_Config_Utils.YAML.parse_action,
                     )
                  |> Result.map(Option.value(~default=[]));
                (key, actions) |> Result.ok;
              })
         })
      |> Option.value(~default=Result.ok([]));
    };

    let* snapshot_directory =
      yaml
      |> OSnap_Config_Utils.YAML.get_string_option("snapshotDirectory")
      |> Result.map(Option.value(~default="__snapshots__"));
    debug(
      Printf.sprintf("snapshot directory is set to %s", snapshot_directory),
    );

    let* parallelism =
      yaml
      |> OSnap_Config_Utils.YAML.get_int_option("parallelism")
      |> Result.map(Option.value(~default=8))
      |> Result.map(max(1));
    debug(Printf.sprintf("parallelism is set to %i", parallelism));

    let root_path =
      String.sub(
        path,
        0,
        String.length(path) - String.length("osnap.config.yaml"),
      );

    debug(Printf.sprintf("setting root path to %s", root_path));

    let* test_pattern =
      yaml
      |> OSnap_Config_Utils.YAML.get_string_option("testPattern")
      |> Result.map(Option.value(~default="**/*.osnap.yaml"));
    debug(Printf.sprintf("test pattern is set to %s", test_pattern));

    let* diff_pixel_color =
      yaml
      |> Yaml.Util.find("diffPixelColor")
      |> Result.map_error(
           fun
           | `Msg(message) =>
             OSnap_Response.Config_Parse_Error(message, Some(path)),
         )
      |> Result.map(
           Option.map(colors => {
             let get_color =
               fun
               | `Float(f) => Result.ok(int_of_float(f))
               | `String(s) => Result.ok(int_of_string(s))
               | _ =>
                 Result.error(
                   OSnap_Response.Config_Parse_Error(
                     "diffPixelColor does not have a correct format",
                     Some(path),
                   ),
                 );

             let* r =
               colors
               |> Yaml.Util.find("r")
               |> Result.map_error(
                    fun
                    | `Msg(message) =>
                      OSnap_Response.Config_Parse_Error(message, Some(path)),
                  )
               |> Result.map(Option.map(get_color))
               |> Result.map(OSnap_Config_Utils.to_result_option)
               |> Result.join
               |> Result.map(Option.value(~default=255));

             let* g =
               colors
               |> Yaml.Util.find("g")
               |> Result.map_error(
                    fun
                    | `Msg(message) =>
                      OSnap_Response.Config_Parse_Error(message, Some(path)),
                  )
               |> Result.map(Option.map(get_color))
               |> Result.map(OSnap_Config_Utils.to_result_option)
               |> Result.join
               |> Result.map(Option.value(~default=0));

             let* b =
               colors
               |> Yaml.Util.find("b")
               |> Result.map_error(
                    fun
                    | `Msg(message) =>
                      OSnap_Response.Config_Parse_Error(message, Some(path)),
                  )
               |> Result.map(Option.map(get_color))
               |> Result.map(OSnap_Config_Utils.to_result_option)
               |> Result.join
               |> Result.map(Option.value(~default=0));

             Result.ok((r, g, b));
           }),
         )
      |> Result.map(OSnap_Config_Utils.to_result_option)
      |> Result.join
      |> Result.map(Option.value(~default=(255, 0, 0)));

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
      Result.error(OSnap_Response.Config_Duplicate_Size_Names(duplicates));
    } else {
      debug("did not find duplicates");
      Result.ok({
        root_path,
        threshold,
        test_pattern,
        ignore_patterns,
        base_url,
        fullscreen,
        default_sizes,
        functions,
        snapshot_directory,
        diff_pixel_color,
        parallelism,
      });
    };
  };
};

module JSON = {
  let ( let* ) = Result.bind;
  let parse = path => {
    let debug = OSnap_Logger.debug(~header="Config.Global.JSON.parse");

    let config = OSnap_Utils.get_file_contents(path);
    let json = config |> Yojson.Basic.from_string(~fname=path);

    let* base_url =
      try(
        json
        |> Yojson.Basic.Util.member("baseUrl")
        |> Yojson.Basic.Util.to_string
        |> Result.ok
      ) {
      | Yojson.Basic.Util.Type_error(message, _) =>
        Result.error(OSnap_Response.Config_Parse_Error(message, Some(path)))
      };
    debug(Printf.sprintf("baseUrl is set to %S", base_url));

    let* fullscreen =
      try(
        json
        |> Yojson.Basic.Util.member("fullScreen")
        |> Yojson.Basic.Util.to_bool_option
        |> Option.value(~default=false)
        |> Result.ok
      ) {
      | Yojson.Basic.Util.Type_error(message, _) =>
        Result.error(OSnap_Response.Config_Parse_Error(message, Some(path)))
      };
    debug(Printf.sprintf("fullScreen is set to %b", fullscreen));

    let* threshold =
      try(
        json
        |> Yojson.Basic.Util.member("threshold")
        |> Yojson.Basic.Util.to_int_option
        |> Option.value(~default=0)
        |> Result.ok
      ) {
      | Yojson.Basic.Util.Type_error(message, _) =>
        Result.error(OSnap_Response.Config_Parse_Error(message, Some(path)))
      };
    debug(Printf.sprintf("threshold is set to %i", threshold));

    let* default_sizes =
      try(
        json
        |> Yojson.Basic.Util.member("defaultSizes")
        |> Yojson.Basic.Util.to_list
        |> OSnap_Utils.List.map_until_exception(
             OSnap_Config_Utils.JSON.parse_size,
           )
      ) {
      | Yojson.Basic.Util.Type_error(message, _) =>
        Result.error(OSnap_Response.Config_Parse_Error(message, Some(path)))
      };

    let* functions = {
      json
      |> Yojson.Basic.Util.member("functions")
      |> (
        fun
        | `Null => Result.ok([])
        | `Assoc(assoc) =>
          assoc
          |> OSnap_Utils.List.map_until_exception(((key, actions)) => {
               let* actions =
                 try(
                   actions
                   |> Yojson.Basic.Util.to_list
                   |> OSnap_Utils.List.map_until_exception(
                        OSnap_Config_Utils.JSON.parse_action,
                      )
                 ) {
                 | Yojson.Basic.Util.Type_error(message, _) =>
                   Result.error(
                     OSnap_Response.Config_Parse_Error(message, Some(path)),
                   )
                 };
               Result.ok((key, actions));
             })
        | _ =>
          Result.error(
            OSnap_Response.Config_Parse_Error(
              "The functions option has to be an object.",
              Some(path),
            ),
          )
      );
    };

    let* snapshot_directory =
      try(
        json
        |> Yojson.Basic.Util.member("snapshotDirectory")
        |> Yojson.Basic.Util.to_string_option
        |> Option.value(~default="__snapshots__")
        |> Result.ok
      ) {
      | Yojson.Basic.Util.Type_error(message, _) =>
        Result.error(OSnap_Response.Config_Parse_Error(message, Some(path)))
      };
    debug(
      Printf.sprintf("snapshot directory is set to %s", snapshot_directory),
    );

    let* parallelism =
      try(
        json
        |> Yojson.Basic.Util.member("parallelism")
        |> Yojson.Basic.Util.to_int_option
        |> Option.value(~default=8)
        |> max(1)
        |> Result.ok
      ) {
      | Yojson.Basic.Util.Type_error(message, _) =>
        Result.error(OSnap_Response.Config_Parse_Error(message, Some(path)))
      };
    debug(Printf.sprintf("parallelism is set to %i", parallelism));

    let root_path =
      String.sub(
        path,
        0,
        String.length(path) - String.length("osnap.config.json"),
      );

    debug(Printf.sprintf("setting root path to %s", root_path));

    let* ignore_patterns =
      try(
        json
        |> Yojson.Basic.Util.member("ignorePatterns")
        |> (
          fun
          | `List(list) =>
            list
            |> OSnap_Utils.List.map_until_exception(item =>
                 try(Yojson.Basic.Util.to_string(item) |> Result.ok) {
                 | Yojson.Basic.Util.Type_error(message, _) =>
                   Result.error(
                     OSnap_Response.Config_Parse_Error(message, Some(path)),
                   )
                 }
               )
          | _ => Result.ok(["**/node_modules/**"])
        )
      ) {
      | Yojson.Basic.Util.Type_error(message, _) =>
        Result.error(OSnap_Response.Config_Parse_Error(message, Some(path)))
      };

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

    let* test_pattern =
      try(
        json
        |> Yojson.Basic.Util.member("testPattern")
        |> Yojson.Basic.Util.to_string_option
        |> Option.value(~default="**/*.osnap.json")
        |> Result.ok
      ) {
      | Yojson.Basic.Util.Type_error(message, _) =>
        Result.error(OSnap_Response.Config_Parse_Error(message, Some(path)))
      };
    debug(Printf.sprintf("test pattern is set to %s", test_pattern));

    let* diff_pixel_color =
      json
      |> Yojson.Basic.Util.member("diffPixelColor")
      |> (
        fun
        | `Assoc(_) as assoc => {
            let get_color = (
              fun
              | `Int(i) => Result.ok(i)
              | `Float(f) => Result.ok(int_of_float(f))
              | _ =>
                Result.error(
                  OSnap_Response.Config_Parse_Error(
                    "diffPixelColor does not have a correct format",
                    Some(path),
                  ),
                )
            );

            let* r = assoc |> Yojson.Basic.Util.member("r") |> get_color;
            let* g = assoc |> Yojson.Basic.Util.member("g") |> get_color;
            let* b = assoc |> Yojson.Basic.Util.member("b") |> get_color;
            Result.ok((r, g, b));
          }
        | `Null => Result.ok((255, 0, 0))
        | _ =>
          Result.error(
            OSnap_Response.Config_Parse_Error(
              "diffPixelColor does not have a correct format",
              Some(path),
            ),
          )
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
      Result.error(OSnap_Response.Config_Duplicate_Size_Names(duplicates));
    } else {
      debug("did not find duplicates");
      Result.ok({
        root_path,
        threshold,
        test_pattern,
        ignore_patterns,
        base_url,
        fullscreen,
        default_sizes,
        functions,
        snapshot_directory,
        diff_pixel_color,
        parallelism,
      });
    };
  };
};

let find = config_names => {
  let debug = OSnap_Logger.debug(~header="Config.Global.find");

  let rec scan_dir = (~config_names, segments) => {
    let current_path = segments |> OSnap_Utils.path_of_segments;
    let elements = current_path |> Sys.readdir |> Array.to_list;

    debug(
      Printf.sprintf(
        "looking for %S in %S",
        String.concat(",", config_names),
        current_path,
      ),
    );

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
    let found_file =
      files |> List.find_opt(file => List.mem(file, config_names));
    switch (found_file) {
    | Some(file) => Some((file, segments))
    | None =>
      let parent_dir_segments = ["..", ...segments];
      let parent_dir = parent_dir_segments |> OSnap_Utils.path_of_segments;

      debug("did not find a config file in this directory");

      try(
        if (parent_dir |> Sys.is_directory) {
          debug("looking in parent directory");
          scan_dir(~config_names, parent_dir_segments);
        } else {
          debug("there is no parent directory anymore");
          None;
        }
      ) {
      | Sys_error(_) => None
      };
    };
  };

  let base_path = Sys.getcwd();
  let config_path = scan_dir(~config_names, [base_path]);

  switch (config_path) {
  | None =>
    debug("no config file was found");
    None;
  | Some((file, segments)) =>
    let path = [file, ...segments] |> OSnap_Utils.path_of_segments;
    debug(Printf.sprintf("found config file at %S", path));
    Some(path);
  };
};

let init = (~config_path) => {
  let ( let* ) = Result.bind;

  let debug = OSnap_Logger.debug(~header="Config.Global.init");
  let* config =
    if (config_path == "") {
      debug("looking for global config file");
      switch (find(["osnap.config.json", "osnap.config.yaml"])) {
      | Some(path) => Result.ok(path)
      | None => Result.error(OSnap_Response.Config_Global_Not_Found)
      };
    } else {
      debug(Printf.sprintf("using provided config path %S", config_path));
      Result.ok(config_path);
    };

  debug("found global config file at " ++ config);
  debug("parsing config file");
  let* format = OSnap_Config_Utils.get_format(config);

  switch (format) {
  | OSnap_Config_Types.JSON => JSON.parse(config)
  | OSnap_Config_Types.YAML => YAML.parse(config)
  };
};
