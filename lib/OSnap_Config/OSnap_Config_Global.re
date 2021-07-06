type size = (int, int);

type t = {
  root_path: string,
  threshold: int,
  ignore_patterns: list(string),
  test_pattern: string,
  base_url: string,
  fullscreen: bool,
  default_sizes: list(size),
  snapshot_directory: string,
  diff_pixel_color: (int, int, int),
  parallelism: int,
};

exception Parse_Error(string);
exception No_Config_Found;

let parse = path => {
  let config = OSnap_Config_Utils.get_file_contents(path);

  try({
    let json = config |> Yojson.Basic.from_string;

    let base_url =
      json
      |> Yojson.Basic.Util.member("baseUrl")
      |> Yojson.Basic.Util.to_string;

    let fullscreen =
      json
      |> Yojson.Basic.Util.member("fullScreen")
      |> Yojson.Basic.Util.to_bool_option
      |> Option.value(~default=false);

    let threshold =
      json
      |> Yojson.Basic.Util.member("threshold")
      |> Yojson.Basic.Util.to_int_option
      |> Option.value(~default=0);

    let default_sizes =
      json
      |> Yojson.Basic.Util.member("defaultSizes")
      |> Yojson.Basic.Util.to_list
      |> List.map(item => {
           let height =
             item
             |> Yojson.Basic.Util.member("height")
             |> Yojson.Basic.Util.to_int;
           let width =
             item
             |> Yojson.Basic.Util.member("width")
             |> Yojson.Basic.Util.to_int;
           (width, height);
         });

    let snapshot_directory =
      json
      |> Yojson.Basic.Util.member("snapshotDirectory")
      |> Yojson.Basic.Util.to_string_option
      |> Option.value(~default="__snapshots__");

    let parallelism =
      json
      |> Yojson.Basic.Util.member("parallelism")
      |> Yojson.Basic.Util.to_int_option
      |> Option.value(~default=3)
      |> max(1);

    let root_path =
      String.sub(
        path,
        0,
        String.length(path) - String.length("osnap.config.json"),
      );

    let ignore_patterns =
      json
      |> Yojson.Basic.Util.member("ignorePatterns")
      |> (
        fun
        | `List(list) => list |> List.map(Yojson.Basic.Util.to_string)
        | _ => ["**/node_modules/**"]
      );

    let test_pattern =
      json
      |> Yojson.Basic.Util.member("testPattern")
      |> Yojson.Basic.Util.to_string_option
      |> Option.value(~default="**/*.osnap.json");

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
  let rec scan_dir = (~config_name, segments) => {
    let elements =
      segments |> Utils.path_of_segments |> Sys.readdir |> Array.to_list;

    let files =
      elements
      |> List.find_all(el => {
           let path = Utils.path_of_segments([el, ...segments]);
           let is_direcoty = path |> Sys.is_directory;
           !is_direcoty;
         });
    let exists = files |> List.exists(file => file == config_name);
    if (exists) {
      Some(segments);
    } else {
      let parent_dir_segments = ["..", ...segments];
      let parent_dir = parent_dir_segments |> Utils.path_of_segments;

      try(
        if (parent_dir |> Sys.is_directory) {
          scan_dir(~config_name, parent_dir_segments);
        } else {
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

    let config_path = scan_dir(~config_name, [base_path]);
    switch (config_path) {
    | None => raise(No_Config_Found)
    | Some(paths) => [config_name, ...paths] |> Utils.path_of_segments
    };
  } else {
    config_path;
  };
};
