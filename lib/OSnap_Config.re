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
