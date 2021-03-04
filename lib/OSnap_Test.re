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

  let select = path => {
    let matches = path |> Re.execp(re);
    let not_ignored = ignore_res |> List.for_all(re => !Re.execp(re, path));

    matches && not_ignored;
  };

  let rec walk = (acc, dirs) => {
    switch (dirs) {
    | [] => acc
    | [dir, ...tail] =>
      let contents = dir |> Sys.readdir |> Array.to_list;
      let contents = contents |> List.rev_map(Filename.concat(dir));
      let (dirs, files) = contents |> List.partition(Sys.is_directory);

      let matched = List.filter(select, files);
      let acc = List.append(acc, matched);
      let dirs = List.append(dirs, tail);

      walk(acc, dirs);
    };
  };
  walk([], [root_path]);
};
