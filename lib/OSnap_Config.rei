type size = (int, int);

type t = {
  baseUrl: string,
  fullScreen: bool,
  defaultSizes: list(size),
  snapshotDirectory: string,
};

let parse: string => result(t, string);
