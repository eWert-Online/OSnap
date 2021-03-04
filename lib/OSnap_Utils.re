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
