type size = (int, int);

type t = {
  baseUrl: string,
  fullScreen: bool,
  defaultSizes: list(size),
  snapshotDirectory: string,
};

let parse: string => Result.t(t, string);

let find:
  (~base_path: string=?, ~config_name: string=?, unit) =>
  Result.t(string, string);
