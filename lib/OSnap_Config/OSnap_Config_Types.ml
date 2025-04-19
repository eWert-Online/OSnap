type size =
  { name : string option
  ; width : int
  ; height : int
  }

type size_restriction = string list option

type pseudo_states =
  { active : bool
  ; focus : bool
  ; hover : bool
  ; visited : bool
  }

type action =
  | Scroll of [ `Selector of string | `PxAmount of int ] * size_restriction
  | Click of string * size_restriction
  | Type of string * string * size_restriction
  | Wait of int * size_restriction
  | ForcePseudoState of string * pseudo_states * size_restriction

type ignoreType =
  | Coordinates of (int * int) * (int * int) * size_restriction
  | Selector of string * size_restriction
  | SelectorAll of string * size_restriction

type additional_headers = (string * string) list option

type test =
  { only : bool
  ; skip : bool
  ; threshold : int
  ; retry : int
  ; name : string
  ; url : string
  ; sizes : size list
  ; actions : action list
  ; ignore : ignoreType list
  ; additional_headers : additional_headers
  }

type global =
  { root_path : Eio.Fs.dir_ty Eio.Path.t
  ; threshold : int
  ; retry : int
  ; ignore_patterns : string list
  ; test_pattern : string
  ; base_url : string
  ; fullscreen : bool
  ; default_sizes : size list
  ; functions : (string * action list) list
  ; snapshot_directory : string
  ; diff_pixel_color : int32
  ; parallelism : int option
  ; additional_headers : additional_headers
  }
