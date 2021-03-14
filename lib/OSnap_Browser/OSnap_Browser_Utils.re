let detect_platform = () => {
  let win = Sys.win32 || Sys.cygwin;
  let ic = Unix.open_process_in("uname");
  let uname = input_line(ic);
  let () = close_in(ic);

  switch (win, uname) {
  | (true, _) =>
    switch (Sys.word_size) {
    | 64 => "win64"
    | _ => "win32"
    }
  | (_, "Darwin") => "darwin"
  | _ => "linux"
  };
};

let contains_substring = (search, str) => {
  let re = Str.regexp_string(search);
  try(Str.search_forward(re, str, 0) >= 0) {
  | Not_found => false
  };
};
// module Download = {
//   let get_url = () => {
//     switch (detect_platform()) {
//     | "darwin" => "https://storage.googleapis.com/chromium-browser-snapshots/Mac/856583/chrome-mac.zip"
//     | "linux" => "https://storage.googleapis.com/chromium-browser-snapshots/Linux_x64/856583/chrome-linux.zip"
//     | "win64" => "https://storage.googleapis.com/chromium-browser-snapshots/Win_x64/856583/chrome-win.zip"
//     | _ => ""
//     };
//   };
// };
