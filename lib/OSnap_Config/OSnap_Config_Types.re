type format =
  | JSON
  | YAML;

type size = {
  name: option(string),
  width: int,
  height: int,
};

type size_restriction = option(list(string));

type action =
  | Function(string, size_restriction)
  | Scroll([ | `Selector(string) | `PxAmount(int)], size_restriction)
  | Click(string, size_restriction)
  | Type(string, string, size_restriction)
  | Wait(int, size_restriction);

type ignoreType =
  | Coordinates((int, int), (int, int), size_restriction)
  | Selector(string, size_restriction)
  | SelectorAll(string, size_restriction);

type test = {
  only: bool,
  skip: bool,
  threshold: int,
  name: string,
  url: string,
  sizes: list(size),
  actions: list(action),
  ignore: list(ignoreType),
};

type global = {
  root_path: string,
  threshold: int,
  ignore_patterns: list(string),
  test_pattern: string,
  base_url: string,
  fullscreen: bool,
  default_sizes: list(size),
  functions: list((string, list(action))),
  snapshot_directory: string,
  diff_pixel_color: (int, int, int),
  parallelism: int,
};
