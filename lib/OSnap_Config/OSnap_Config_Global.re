type size = (int, int);

type t = {
  root_path: string,
  ignore_patterns: list(string),
  test_pattern: string,
  base_url: string,
  fullscreen: bool,
  default_sizes: list(size),
  snapshot_directory: string,
};

exception Parse_Error(string);
exception No_Config_Found;

let parse = path => {
  let ic = open_in(path);
  let file_length = in_channel_length(ic);
  let config = really_input_string(ic, file_length);
  close_in(ic);

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

    {
      root_path,
      test_pattern,
      ignore_patterns,
      base_url,
      fullscreen,
      default_sizes,
      snapshot_directory,
    };
  }) {
  | Yojson.Basic.Util.Type_error(msg, _) => raise(Parse_Error(msg))
  };
};

let find = () => {
  let config_name = "osnap.config.json";

  let base_path = Sys.getcwd();

  let rec scan_dir = segments => {
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
          scan_dir(parent_dir_segments);
        } else {
          None;
        }
      ) {
      | Sys_error(_) => None
      };
    };
  };

  let config_path = scan_dir([base_path]);
  switch (config_path) {
  | None => raise(No_Config_Found)
  | Some(paths) => [config_name, ...paths] |> Utils.path_of_segments
  };
};
