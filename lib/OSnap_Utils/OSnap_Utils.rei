type platform =
  | Win32
  | Win64
  | Darwin
  | Linux;

let detect_platform: unit => platform;

let get_file_contents: string => string;

let contains_substring: (~search: string, string) => bool;
