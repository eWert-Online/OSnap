type size = (int, int);

type t = {
  baseUrl: string,
  fullScreen: bool,
  defaultSizes: list(size),
  snapshotDirectory: string,
};

let parse = config =>
  try({
    let json = config |> Yojson.Basic.from_string;

    let baseUrl =
      json
      |> Yojson.Basic.Util.member("baseUrl")
      |> Yojson.Basic.Util.to_string;

    let fullScreen =
      json
      |> Yojson.Basic.Util.member("fullScreen")
      |> Yojson.Basic.Util.to_bool_option
      |> Option.value(~default=false);

    let defaultSizes =
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

    let snapshotDirectory =
      json
      |> Yojson.Basic.Util.member("snapshotDirectory")
      |> Yojson.Basic.Util.to_string_option
      |> Option.value(~default="__snapshots__");

    Result.ok({baseUrl, fullScreen, defaultSizes, snapshotDirectory});
  }) {
  | Yojson.Basic.Util.Type_error(msg, _) => Result.error(msg)
  };

let find = (~base_path="", ~config_name="osnap.config.json", ()) => {
  let path_of_segments = paths =>
    paths
    |> List.rev
    |> List.fold_left(
         (acc, curr) =>
           switch (acc) {
           | "" => curr
           | path => path ++ "/" ++ curr
           },
         "",
       );

  let base_path =
    switch (base_path) {
    | "" => Sys.getcwd()
    | path => Sys.getcwd() ++ "/" ++ path
    };

  let rec scan_dir = segments => {
    let elements =
      segments |> path_of_segments |> Sys.readdir |> Array.to_list;
    let (dirs, files) =
      elements
      |> List.partition(el => {
           let path = path_of_segments([el, ...segments]);
           path |> Sys.is_directory;
         });
    let exists = files |> List.exists(file => file == config_name);
    if (exists) {
      Some(segments);
    } else {
      dirs |> List.find_map(dir => scan_dir([dir, ...segments]));
    };
  };

  let config_path = scan_dir([base_path]);
  switch (config_path) {
  | None => Result.error("No config file found")
  | Some(paths) => paths |> path_of_segments |> Result.ok
  };
};
