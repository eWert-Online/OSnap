type platform =
  | Win32
  | Win64
  | Darwin
  | Linux;

let detect_platform = () => {
  let win = Sys.win32 || Sys.cygwin;

  switch (win, Sys.word_size) {
  | (true, 64) => Win64
  | (true, _) => Win32
  | (false, _) =>
    let ic = Unix.open_process_in("uname");
    let uname = input_line(ic);
    let () = close_in(ic);

    if (uname == "Darwin") {
      Darwin;
    } else {
      Linux;
    };
  };
};

let get_file_contents = filename => {
  let ic = open_in_bin(filename);
  let file_length = in_channel_length(ic);
  let data = really_input_string(ic, file_length);
  close_in(ic);
  data;
};

let contains_substring = (~search, str) => {
  let search_length = String.length(search);
  let len = String.length(str);
  try(
    {
      for (i in 0 to len - search_length) {
        let j = ref(0);
        while (str.[i + j^] == search.[j^]) {
          incr(j);
          if (j^ == search_length) {
            raise(Exit);
          };
        };
      };
      false;
    }
  ) {
  | Exit => true
  };
};

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

let path_of_segments = paths => {
  paths
  |> List.rev
  |> List.fold_left(
       (acc, curr) => {
         switch (acc) {
         | "" => curr
         | path => path ++ "/" ++ curr
         }
       },
       "",
     );
};
