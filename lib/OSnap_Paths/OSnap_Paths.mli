val get_snapshot_root_path : OSnap_Config.Types.global -> Eio.Fs.dir_ty Eio.Path.t
val get_base_images_dir : OSnap_Config.Types.global -> Eio.Fs.dir_ty Eio.Path.t
val get_updated_dir : OSnap_Config.Types.global -> Eio.Fs.dir_ty Eio.Path.t
val get_diff_dir : OSnap_Config.Types.global -> Eio.Fs.dir_ty Eio.Path.t

type t =
  { base : Eio.Fs.dir_ty Eio.Path.t
  ; updated : Eio.Fs.dir_ty Eio.Path.t
  ; diff : Eio.Fs.dir_ty Eio.Path.t
  }

val get : OSnap_Config.Types.global -> t
val init_folder_structure : OSnap_Config.Types.global -> unit
