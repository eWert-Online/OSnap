let get_snapshot_root_path (config : OSnap_Config.Types.global) =
  config.root_path ^ config.snapshot_directory
;;

let get_base_images_dir (config : OSnap_Config.Types.global) =
  let base_path = get_snapshot_root_path config in
  base_path ^ "/__base_images__"
;;

let get_updated_dir (config : OSnap_Config.Types.global) =
  let base_path = get_snapshot_root_path config in
  base_path ^ "/__updated__"
;;

let get_diff_dir (config : OSnap_Config.Types.global) =
  let base_path = get_snapshot_root_path config in
  base_path ^ "/__diff__"
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
