let get_snapshot_root_path (config : OSnap_Config.Types.global) =
  Eio.Path.(config.root_path / config.snapshot_directory)
;;

let get_base_images_dir (config : OSnap_Config.Types.global) =
  let base_path = get_snapshot_root_path config in
  Eio.Path.(base_path / "__base_images__")
;;

let get_updated_dir (config : OSnap_Config.Types.global) =
  let base_path = get_snapshot_root_path config in
  Eio.Path.(base_path / "__updated__")
;;

let get_diff_dir (config : OSnap_Config.Types.global) =
  let base_path = get_snapshot_root_path config in
  Eio.Path.(base_path / "__diff__")
;;

type t =
  { base : Eio.Fs.dir_ty Eio.Path.t
  ; updated : Eio.Fs.dir_ty Eio.Path.t
  ; diff : Eio.Fs.dir_ty Eio.Path.t
  }

let get config =
  { base = get_base_images_dir config
  ; updated = get_updated_dir config
  ; diff = get_diff_dir config
  }
;;

let init_folder_structure config =
  let dirs = get config in
  Eio.Path.mkdirs ~exists_ok:true ~perm:0o755 dirs.base;
  Eio.Path.rmtree ~missing_ok:true dirs.updated;
  Eio.Path.mkdirs ~exists_ok:true ~perm:0o755 dirs.updated;
  Eio.Path.rmtree ~missing_ok:true dirs.diff;
  Eio.Path.mkdirs ~exists_ok:true ~perm:0o755 dirs.diff
;;
