let find_duplicates = (get_key, list) => {
  let hash = Hashtbl.create(List.length(list));
  list
  |> List.filter(item =>
       if (Hashtbl.mem(hash, get_key(item))) {
         true;
       } else {
         Hashtbl.add(hash, get_key(item), true);
         false;
       }
     );
};

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
