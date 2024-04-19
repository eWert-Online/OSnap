module C = Configurator.V1

let ( let* ) = Option.bind

let main c =
  let get_path lib dir =
    match C.which c "pkg-config" with
    | None -> raise Not_found
    | Some pkgcfg ->
      let env =
        match C.ocaml_config_var c "system" with
        | Some "macosx" ->
          let* brew = C.which c "brew" in
          let new_pkg_config_path =
            let prefix = String.trim (C.Process.run_capture_exn c brew [ "--prefix" ]) in
            let p = Printf.sprintf "%s/opt/%s/lib/pkgconfig" prefix lib in
            try if Sys.is_directory p then Some p else None with
            | _ -> None
          in
          new_pkg_config_path
          |> Option.map (fun new_pkg_config_path ->
            let pkg_config_path =
              match Sys.getenv_opt "PKG_CONFIG_PATH" with
              | Some s -> s ^ ":"
              | None -> ""
            in
            [ Printf.sprintf "PKG_CONFIG_PATH=%s%s" pkg_config_path new_pkg_config_path ])
        | _ -> None
      in
      C.Process.run_capture_exn c ?env pkgcfg [ lib; "--variable=" ^ dir ]
  in
  let switch_prefix = Sys.getenv "OPAM_SWITCH_PREFIX" in
  let spng_include_path = switch_prefix ^ "/include" in
  let libspng = switch_prefix ^ "/lib/libspng_static.a" in
  let z_lib_path = get_path "zlib" "libdir" |> String.trim in
  let zlib = z_lib_path ^ "/libz.a" in
  C.Flags.write_sexp "png_write_c_flags.sexp" [ "-fPIC"; "-I" ^ spng_include_path ];
  C.Flags.write_sexp "png_write_c_library_flags.sexp" [ libspng; zlib ];
  C.Flags.write_sexp "png_write_flags.sexp" [ "-cclib"; libspng ];
  C.Flags.write_sexp "png_c_flags.sexp" [ "-fPIC"; "-I" ^ spng_include_path ];
  C.Flags.write_sexp "png_c_library_flags.sexp" [ libspng; zlib ];
  C.Flags.write_sexp "png_flags.sexp" [ "-cclib"; libspng ]
;;

C.main ~name:"c-lib-package-resolver" main
