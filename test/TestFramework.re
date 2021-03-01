include Rely.Make({
  let config =
    Rely.TestFrameworkConfig.initialize({
      snapshotDir: "test/__snapshots__",
      projectDir: ".",
    });
});

module Utils = {
  let make_path_relative = absolute => {
    let cwd_segments = Sys.getcwd() |> String.split_on_char('/');

    absolute
    |> String.split_on_char('/')
    |> List.mapi((index, segment) => {
         switch (List.nth_opt(cwd_segments, index)) {
         | None => segment
         | Some(cwd_segment) =>
           if (cwd_segment == segment) {
             "";
           } else {
             segment;
           }
         }
       })
    |> List.filter(s => s != "")
    |> String.concat("/");
  };
};
