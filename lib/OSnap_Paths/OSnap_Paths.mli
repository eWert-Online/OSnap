val get_snapshot_root_path : OSnap_Config.Types.global -> string
val get_base_images_dir : OSnap_Config.Types.global -> string
val get_updated_dir : OSnap_Config.Types.global -> string
val get_diff_dir : OSnap_Config.Types.global -> string

type t =
  { base : string
  ; updated : string
  ; diff : string
  }

val get : OSnap_Config.Types.global -> t
val init_folder_structure : OSnap_Config.Types.global -> unit
