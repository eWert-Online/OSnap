exception Parse_Error(string);
exception No_Config_Found;

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

let parse: string => t;

let find: (~config_path: string) => string;
