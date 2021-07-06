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
  let re = Str.regexp_string(search);
  try(Str.search_forward(re, str, 0) >= 0) {
  | Not_found => false
  };
};
