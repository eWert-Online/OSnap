type platform =
  | Win32
  | Win64
  | MacOS
  | MacOS_ARM
  | Linux

val detect_platform : unit -> platform
val get_file_contents : string -> string
val contains_substring : search:string -> string -> bool
val find_duplicates : ('a -> 'b) -> 'a list -> 'a list
val path_of_segments : string list -> string

module List : sig
  val map_until_exception : ('a -> ('b, 'c) result) -> 'a list -> ('b list, 'c) result
end
