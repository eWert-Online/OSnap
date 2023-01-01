open OSnap_Config_Types

module YAML = struct
  let ( let* ) = Result.bind

  let parse path =
    let config = OSnap_Utils.get_file_contents path in
    let* yaml =
      config
      |> Yaml.of_string
      |> Result.map_error (fun _ ->
           OSnap_Response.Config_Parse_Error
             (Printf.sprintf "YAML could not be parsed", Some path))
    in
    let* base_url = yaml |> OSnap_Config_Utils.YAML.get_string "baseUrl" in
    let* fullscreen =
      yaml
      |> OSnap_Config_Utils.YAML.get_bool_option "fullScreen"
      |> Result.map (Option.value ~default:false)
    in
    let* threshold =
      yaml
      |> OSnap_Config_Utils.YAML.get_int_option "threshold"
      |> Result.map (Option.value ~default:0)
    in
    let* ignore_patterns =
      yaml
      |> OSnap_Config_Utils.YAML.get_string_list_option "ignorePatterns"
      |> Result.map (Option.value ~default:[ "**/node_modules/**" ])
    in
    let* default_sizes =
      yaml
      |> OSnap_Config_Utils.YAML.get_list_option
           "defaultSizes"
           ~parser:OSnap_Config_Utils.YAML.parse_size
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
                       key
                       ~parser:OSnap_Config_Utils.YAML.parse_action
                  |> Result.map (Option.value ~default:[])
                in
                (key, actions) |> Result.ok))
      |> Option.value ~default:(Result.ok [])
    in
    let* snapshot_directory =
      yaml
      |> OSnap_Config_Utils.YAML.get_string_option "snapshotDirectory"
      |> Result.map (Option.value ~default:"__snapshots__")
    in
    let* parallelism =
      yaml
      |> OSnap_Config_Utils.YAML.get_int_option "parallelism"
      |> Result.map (Option.value ~default:8)
      |> Result.map (max 1)
    in
    let root_path =
      String.sub path 0 (String.length path - String.length "osnap.config.yaml")
    in
    let* test_pattern =
      yaml
      |> OSnap_Config_Utils.YAML.get_string_option "testPattern"
      |> Result.map (Option.value ~default:"**/*.osnap.yaml")
    in
    let* diff_pixel_color =
      yaml
      |> Yaml.Util.find "diffPixelColor"
      |> Result.map_error (function `Msg message ->
           OSnap_Response.Config_Parse_Error (message, Some path))
      |> Result.map
           (Option.map (fun colors ->
              let get_color = function
                | `Float f -> Result.ok (int_of_float f)
                | `String s -> Result.ok (int_of_string s)
                | _ ->
                  Result.error
                    (OSnap_Response.Config_Parse_Error
                       ("diffPixelColor does not have a correct format", Some path))
              in
              let* r =
                colors
                |> Yaml.Util.find "r"
                |> Result.map_error (function `Msg message ->
                     OSnap_Response.Config_Parse_Error (message, Some path))
                |> Result.map (Option.map get_color)
                |> Result.map OSnap_Config_Utils.to_result_option
                |> Result.join
                |> Result.map (Option.value ~default:255)
              in
              let* g =
                colors
                |> Yaml.Util.find "g"
                |> Result.map_error (function `Msg message ->
                     OSnap_Response.Config_Parse_Error (message, Some path))
                |> Result.map (Option.map get_color)
                |> Result.map OSnap_Config_Utils.to_result_option
                |> Result.join
                |> Result.map (Option.value ~default:0)
              in
              let* b =
                colors
                |> Yaml.Util.find "b"
                |> Result.map_error (function `Msg message ->
                     OSnap_Response.Config_Parse_Error (message, Some path))
                |> Result.map (Option.map get_color)
                |> Result.map OSnap_Config_Utils.to_result_option
                |> Result.join
                |> Result.map (Option.value ~default:0)
              in
              Result.ok (r, g, b)))
      |> Result.map OSnap_Config_Utils.to_result_option
      |> Result.join
      |> Result.map (Option.value ~default:(255, 0, 0))
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
    then Result.error (OSnap_Response.Config_Duplicate_Size_Names duplicates)
    else
      Result.ok
        { root_path
        ; threshold
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

module JSON = struct
  let ( let* ) = Result.bind

  let parse path =
    let config = OSnap_Utils.get_file_contents path in
    let json = config |> Yojson.Basic.from_string ~fname:path in
    let* base_url =
      try
        json
        |> Yojson.Basic.Util.member "baseUrl"
        |> Yojson.Basic.Util.to_string
        |> Result.ok
      with
      | Yojson.Basic.Util.Type_error (message, _) ->
        Result.error (OSnap_Response.Config_Parse_Error (message, Some path))
    in
    let* fullscreen =
      try
        json
        |> Yojson.Basic.Util.member "fullScreen"
        |> Yojson.Basic.Util.to_bool_option
        |> Option.value ~default:false
        |> Result.ok
      with
      | Yojson.Basic.Util.Type_error (message, _) ->
        Result.error (OSnap_Response.Config_Parse_Error (message, Some path))
    in
    let* threshold =
      try
        json
        |> Yojson.Basic.Util.member "threshold"
        |> Yojson.Basic.Util.to_int_option
        |> Option.value ~default:0
        |> Result.ok
      with
      | Yojson.Basic.Util.Type_error (message, _) ->
        Result.error (OSnap_Response.Config_Parse_Error (message, Some path))
    in
    let* default_sizes =
      try
        json
        |> Yojson.Basic.Util.member "defaultSizes"
        |> Yojson.Basic.Util.to_list
        |> OSnap_Utils.List.map_until_exception OSnap_Config_Utils.JSON.parse_size
      with
      | Yojson.Basic.Util.Type_error (message, _) ->
        Result.error (OSnap_Response.Config_Parse_Error (message, Some path))
    in
    let* functions =
      json
      |> Yojson.Basic.Util.member "functions"
      |> function
      | `Null -> Result.ok []
      | `Assoc assoc ->
        assoc
        |> OSnap_Utils.List.map_until_exception (fun (key, actions) ->
             let* actions =
               try
                 actions
                 |> Yojson.Basic.Util.to_list
                 |> OSnap_Utils.List.map_until_exception
                      OSnap_Config_Utils.JSON.parse_action
               with
               | Yojson.Basic.Util.Type_error (message, _) ->
                 Result.error (OSnap_Response.Config_Parse_Error (message, Some path))
             in
             Result.ok (key, actions))
      | _ ->
        Result.error
          (OSnap_Response.Config_Parse_Error
             ("The functions option has to be an object.", Some path))
    in
    let* snapshot_directory =
      try
        json
        |> Yojson.Basic.Util.member "snapshotDirectory"
        |> Yojson.Basic.Util.to_string_option
        |> Option.value ~default:"__snapshots__"
        |> Result.ok
      with
      | Yojson.Basic.Util.Type_error (message, _) ->
        Result.error (OSnap_Response.Config_Parse_Error (message, Some path))
    in
    let* parallelism =
      try
        json
        |> Yojson.Basic.Util.member "parallelism"
        |> Yojson.Basic.Util.to_int_option
        |> Option.value ~default:8
        |> max 1
        |> Result.ok
      with
      | Yojson.Basic.Util.Type_error (message, _) ->
        Result.error (OSnap_Response.Config_Parse_Error (message, Some path))
    in
    let root_path =
      String.sub path 0 (String.length path - String.length "osnap.config.json")
    in
    let* ignore_patterns =
      try
        json
        |> Yojson.Basic.Util.member "ignorePatterns"
        |> function
        | `List list ->
          list
          |> OSnap_Utils.List.map_until_exception (fun item ->
               try Yojson.Basic.Util.to_string item |> Result.ok with
               | Yojson.Basic.Util.Type_error (message, _) ->
                 Result.error (OSnap_Response.Config_Parse_Error (message, Some path)))
        | _ -> Result.ok [ "**/node_modules/**" ]
      with
      | Yojson.Basic.Util.Type_error (message, _) ->
        Result.error (OSnap_Response.Config_Parse_Error (message, Some path))
    in
    let* test_pattern =
      try
        json
        |> Yojson.Basic.Util.member "testPattern"
        |> Yojson.Basic.Util.to_string_option
        |> Option.value ~default:"**/*.osnap.json"
        |> Result.ok
      with
      | Yojson.Basic.Util.Type_error (message, _) ->
        Result.error (OSnap_Response.Config_Parse_Error (message, Some path))
    in
    let* diff_pixel_color =
      json
      |> Yojson.Basic.Util.member "diffPixelColor"
      |> function
      | `Assoc _ as assoc ->
        let get_color = function
          | `Int i -> Result.ok i
          | `Float f -> Result.ok (int_of_float f)
          | _ ->
            Result.error
              (OSnap_Response.Config_Parse_Error
                 ("diffPixelColor does not have a correct format", Some path))
        in
        let* r = assoc |> Yojson.Basic.Util.member "r" |> get_color in
        let* g = assoc |> Yojson.Basic.Util.member "g" |> get_color in
        let* b = assoc |> Yojson.Basic.Util.member "b" |> get_color in
        Result.ok (r, g, b)
      | `Null -> Result.ok (255, 0, 0)
      | _ ->
        Result.error
          (OSnap_Response.Config_Parse_Error
             ("diffPixelColor does not have a correct format", Some path))
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
    then Result.error (OSnap_Response.Config_Duplicate_Size_Names duplicates)
    else
      Result.ok
        { root_path
        ; threshold
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

let find config_names =
  let rec scan_dir ~config_names segments =
    let current_path = segments |> OSnap_Utils.path_of_segments in
    let elements = current_path |> Sys.readdir |> Array.to_list in
    let files =
      elements
      |> List.find_all (fun el ->
           let path = OSnap_Utils.path_of_segments (el :: segments) in
           let is_direcoty = path |> Sys.is_directory in
           not is_direcoty)
    in
    let found_file = files |> List.find_opt (fun file -> List.mem file config_names) in
    match found_file with
    | Some file -> Some (file, segments)
    | None ->
      let parent_dir_segments = ".." :: segments in
      let parent_dir = parent_dir_segments |> OSnap_Utils.path_of_segments in
      (try
         if parent_dir |> Sys.is_directory
         then scan_dir ~config_names parent_dir_segments
         else None
       with
       | Sys_error _ -> None)
  in
  let base_path = Sys.getcwd () in
  let config_path = scan_dir ~config_names [ base_path ] in
  match config_path with
  | None -> None
  | Some (file, segments) ->
    let path = file :: segments |> OSnap_Utils.path_of_segments in
    Some path
;;

let init ~config_path =
  let ( let* ) = Result.bind in
  let* config =
    if config_path = ""
    then (
      match find [ "osnap.config.json"; "osnap.config.yaml" ] with
      | Some path -> Result.ok path
      | None -> Result.error OSnap_Response.Config_Global_Not_Found)
    else Result.ok config_path
  in
  let* format = OSnap_Config_Utils.get_format config in
  match format with
  | OSnap_Config_Types.JSON -> JSON.parse config
  | OSnap_Config_Types.YAML -> YAML.parse config
;;
