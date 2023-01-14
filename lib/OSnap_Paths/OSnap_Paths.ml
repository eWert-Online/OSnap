let get_snapshot_root_path (config : OSnap_Config.Types.global) =
  Filename.concat config.root_path config.snapshot_directory
;;

let get_base_images_dir (config : OSnap_Config.Types.global) =
  let base_path = get_snapshot_root_path config in
  Filename.concat base_path "__base_images__"
;;

let get_updated_dir (config : OSnap_Config.Types.global) =
  let base_path = get_snapshot_root_path config in
  Filename.concat base_path "__updated__"
;;

let get_diff_dir (config : OSnap_Config.Types.global) =
  let base_path = get_snapshot_root_path config in
  Filename.concat base_path "__diff__"
;;

type t =
  { base : string
  ; updated : string
  ; diff : string
  }

let get config =
  { base = get_base_images_dir config
  ; updated = get_updated_dir config
  ; diff = get_diff_dir config
  }
;;

let init_folder_structure config =
  let dirs = get config in
  if not (Sys.file_exists dirs.base)
  then FileUtil.mkdir ~parent:true ~mode:(`Octal 0o755) dirs.base;
  FileUtil.rm ~recurse:true [ dirs.updated ];
  FileUtil.mkdir ~parent:true ~mode:(`Octal 0o755) dirs.updated;
  FileUtil.rm ~recurse:true [ dirs.diff ];
  FileUtil.mkdir ~parent:true ~mode:(`Octal 0o755) dirs.diff
;;
