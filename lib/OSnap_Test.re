type size = (int, int);

type action =
  | Click(string)
  | Type(string, string)
  | Wait(int);

type t = {
  name: string,
  url: string,
  sizes: list(size),
  actions: list(action),
};

let find =
    (~root_path="/", ~pattern="**/*.osnap.json", ~ignore_patterns=[], ()) => {
  let re = pattern |> Re.Glob.glob |> Re.compile;
  let ignore_res =
    ignore_patterns
    |> List.map(pattern => pattern |> Re.Glob.glob |> Re.compile);

  let is_not_ignored = path => {
    ignore_res |> List.for_all(re => !Re.execp(re, path));
  };

  let select = path => {
    let matches = path |> Re.execp(re);
    if (matches) {
      path |> is_not_ignored;
    } else {
      false;
    };
  };

  let rec walk = (acc, dirs) => {
    switch (dirs) {
    | [] => acc
    | [dir, ...tail] =>
      let contents = dir |> Sys.readdir |> Array.to_list;
      let contents = contents |> List.rev_map(Filename.concat(dir));
      let (dirs, files) = contents |> List.partition(Sys.is_directory);

      let matched = files |> List.filter(select);
      let non_ignored_dirs = dirs |> List.filter(is_not_ignored);
      let acc = List.append(acc, matched);
      let dirs = List.append(non_ignored_dirs, tail);

      walk(acc, dirs);
    };
  };
  walk([], [root_path]);
};
