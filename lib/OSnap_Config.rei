type size = (int, int);

type t = {
  root_path: string,
  base_url: string,
  fullscreen: bool,
  default_sizes: list(size),
  snapshot_directory: string,
};

let parse: string => Result.t(t, string);

let find: unit => Result.t(string, string);
