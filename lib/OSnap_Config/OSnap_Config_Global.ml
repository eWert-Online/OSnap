open OSnap_Config_Types

module YAML = struct
  let ( let* ) = Result.bind

  let parse path =
    let config = Eio.Path.load path in
    let* yaml =
      config
      |> Yaml.of_string
      |> Result.map_error (fun _ ->
        `OSnap_Config_Parse_Error ("YAML could not be parsed", path))
    in
    let* base_url = yaml |> OSnap_Config_Utils.YAML.get_string ~path "baseUrl" in
    let* fullscreen =
      yaml
      |> OSnap_Config_Utils.YAML.get_bool_option ~path "fullScreen"
      |> Result.map (Option.value ~default:false)
    in
    let* threshold =
      yaml
      |> OSnap_Config_Utils.YAML.get_int_option ~path "threshold"
      |> Result.map (Option.value ~default:0)
    in
    let* retry =
      yaml
      |> OSnap_Config_Utils.YAML.get_int_option ~path "retry"
      |> Result.map (Option.value ~default:1)
    in
    let* ignore_patterns =
      yaml
      |> OSnap_Config_Utils.YAML.get_string_list_option ~path "ignorePatterns"
      |> Result.map (Option.value ~default:[ "**/node_modules/**" ])
    in
    let* default_sizes =
      yaml
      |> OSnap_Config_Utils.YAML.get_list_option
           ~path
           "defaultSizes"
           ~parser:(OSnap_Config_Utils.YAML.parse_size ~path)
      |> Result.map (Option.value ~default:[])
    in
    let* functions =
      let f = yaml |> Yaml.Util.find_exn "functions" in
      f
      |> Option.map (fun f ->
        f
        |> Yaml.Util.keys_exn
        |> OSnap_Utils.List.map_until_exception (fun key ->
          let* actions =
            f
            |> OSnap_Config_Utils.YAML.get_list_option
                 ~path
                 key
                 ~parser:(OSnap_Config_Utils.YAML.parse_action ~global_fns:[] ~path)
            |> Result.map (Option.value ~default:[])
            |> Result.map List.flatten
          in
          (key, actions) |> Result.ok))
      |> Option.value ~default:(Result.ok [])
    in
    let* snapshot_directory =
      yaml
      |> OSnap_Config_Utils.YAML.get_string_option ~path "snapshotDirectory"
      |> Result.map (Option.value ~default:"__snapshots__")
    in
    let* parallelism =
      yaml
      |> OSnap_Config_Utils.YAML.get_int_option ~path "parallelism"
      |> Result.map (Option.map (max 1))
    in
    let root_path = Eio.Path.split path |> Option.map fst |> Option.get in
    let* test_pattern =
      yaml
      |> OSnap_Config_Utils.YAML.get_string_option ~path "testPattern"
      |> Result.map (Option.value ~default:"**/*.osnap.yaml")
    in
    let* diff_pixel_color =
      yaml
      |> Yaml.Util.find "diffPixelColor"
      |> Result.map_error (function `Msg message ->
          `OSnap_Config_Parse_Error (message, path))
      |> Result.map
           (Option.map (fun colors ->
              let get_color = function
                | `Float f -> Result.ok (int_of_float f)
                | `String s -> Result.ok (int_of_string s)
                | _ ->
                  Result.error
                    (`OSnap_Config_Parse_Error
                        ("diffPixelColor does not have a correct format", path))
              in
              let* r =
                colors
                |> Yaml.Util.find "r"
                |> Result.map_error (function `Msg message ->
                    `OSnap_Config_Parse_Error (message, path))
                |> Result.map (Option.map get_color)
                |> Result.map OSnap_Config_Utils.to_result_option
                |> Result.join
                |> Result.map (Option.value ~default:255)
              in
              let* g =
                colors
                |> Yaml.Util.find "g"
                |> Result.map_error (function `Msg message ->
                    `OSnap_Config_Parse_Error (message, path))
                |> Result.map (Option.map get_color)
                |> Result.map OSnap_Config_Utils.to_result_option
                |> Result.join
                |> Result.map (Option.value ~default:0)
              in
              let* b =
                colors
                |> Yaml.Util.find "b"
                |> Result.map_error (function `Msg message ->
                    `OSnap_Config_Parse_Error (message, path))
                |> Result.map (Option.map get_color)
                |> Result.map OSnap_Config_Utils.to_result_option
                |> Result.join
                |> Result.map (Option.value ~default:0)
              in
              Result.ok
              @@ Int32.of_int ((0xFF lsl 24) lor (b lsl 16) lor (g lsl 8) lor (r lsl 0))))
      |> Result.map OSnap_Config_Utils.to_result_option
      |> Result.join
      |> Result.map (Option.value ~default:0xFF0000FFl)
    in
    let duplicates =
      default_sizes
      |> List.filter (fun (s : OSnap_Config_Types.size) -> Option.is_some s.name)
      |> OSnap_Utils.find_duplicates (fun (s : OSnap_Config_Types.size) -> s.name)
      |> List.map (fun (s : OSnap_Config_Types.size) ->
        let name = Option.value s.name ~default:"" in
        name)
    in
    if List.length duplicates <> 0
    then Result.error (`OSnap_Config_Duplicate_Size_Names duplicates)
    else
      Result.ok
        { root_path
        ; threshold
        ; retry
        ; test_pattern
        ; ignore_patterns
        ; base_url
        ; fullscreen
        ; default_sizes
        ; functions
        ; snapshot_directory
        ; diff_pixel_color
        ; parallelism
        }
  ;;
end

let find ~fs config_names =
  let rec scan_dir ~config_names current_path =
    let elements = current_path |> Eio.Path.read_dir in
    let files =
      elements
      |> List.find_all (fun el ->
        let path = Eio.Path.(current_path / el) in
        Eio.Path.is_file path)
    in
    let found_file = files |> List.find_opt (fun file -> List.mem file config_names) in
    match found_file with
    | Some file -> Some Eio.Path.(current_path / file)
    | None ->
      let parent_dir = Eio.Path.split current_path |> Option.map fst in
      (try
         match parent_dir with
         | Some dir when Eio.Path.is_directory dir -> scan_dir ~config_names dir
         | _ -> None
       with
       | _ -> None)
  in
  scan_dir ~config_names Eio.Path.(fs / Sys.getcwd ())
;;

let init ~env ~config_path =
  let ( let* ) = Result.bind in
  let fs = Eio.Stdenv.fs env in
  let* config =
    if config_path = ""
    then (
      match find ~fs [ "osnap.config.yaml" ] with
      | Some path -> Result.ok path
      | None -> Result.error `OSnap_Config_Global_Not_Found)
    else Result.ok Eio.Path.(fs / config_path)
  in
  YAML.parse config
;;
