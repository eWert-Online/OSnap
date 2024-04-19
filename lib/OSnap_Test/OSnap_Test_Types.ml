type t =
  { url : string
  ; name : string
  ; size_name : string option
  ; width : int
  ; height : int
  ; actions : OSnap_Config.Types.action list
  ; ignore_regions : OSnap_Config.Types.ignoreType list
  ; threshold : int
  ; exists : bool
  ; skip : bool
  ; warnings : string list
  ; result :
      [ `Created
      | `Failed of [ `Io | `Layout | `Pixel of int * float ]
      | `Passed
      | `Skipped
      ]
        option
  }
