type platform =
  | Win32
  | Win64
  | Darwin
  | Linux;

let detect_platform: unit => platform;

let get_file_contents: string => string;

let contains_substring: (~search: string, string) => bool;

let find_duplicates: ('a => 'b, list('a)) => list('a);

let path_of_segments: list(string) => string;

module List: {
  let map_until_exception:
    ('a => result('b, 'c), list('a)) => result(list('b), 'c);
};
