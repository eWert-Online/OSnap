open OSnap_Config_Types

let ( let* ) = Result.bind

module Common = struct
  let collect_duplicates sizes =
    let duplicates =
      sizes
      |> List.filter (fun (s : OSnap_Config_Types.size) -> Option.is_some s.name)
      |> OSnap_Utils.find_duplicates (fun (s : OSnap_Config_Types.size) -> s.name)
      |> List.map (fun (s : OSnap_Config_Types.size) -> Option.value s.name ~default:"")
    in
    if List.length duplicates <> 0
    then Result.error (`OSnap_Config_Duplicate_Size_Names duplicates)
    else Result.ok ()
  ;;

  let collect_ignore ~path ~size_restriction ~selector ~selector_all ~x1 ~y1 ~x2 ~y2 =
    match selector_all, selector, x1, y1, x2, y2 with
    | Some selector_all, None, None, None, None, None ->
      SelectorAll (selector_all, size_restriction) |> Result.ok
    | None, Some selector, None, None, None, None ->
      Selector (selector, size_restriction) |> Result.ok
    | None, None, Some x1, Some y1, Some x2, Some y2 ->
      Coordinates ((x1, y1), (x2, y2), size_restriction) |> Result.ok
    | _ ->
      Result.error
        (`OSnap_Config_Invalid
          ("Did not find a complete configuration for an ignore region.", path))
  ;;
end

module JSON = struct
  let parse_ignore ~path r =
    let* size_restriction =
      try
        r
        |> Yojson.Basic.Util.member "@"
        |> Yojson.Basic.Util.to_option Yojson.Basic.Util.to_list
        |> Option.map (List.map Yojson.Basic.Util.to_string)
        |> Result.ok
      with
      | Yojson.Basic.Util.Type_error (message, _) ->
        Result.error (`OSnap_Config_Parse_Error (message, path))
    in
    let* x1 =
      try
        r |> Yojson.Basic.Util.member "x1" |> Yojson.Basic.Util.to_int_option |> Result.ok
      with
      | Yojson.Basic.Util.Type_error (message, _) ->
        Result.error (`OSnap_Config_Parse_Error (message, path))
    in
    let* y1 =
      try
        r |> Yojson.Basic.Util.member "y1" |> Yojson.Basic.Util.to_int_option |> Result.ok
      with
      | Yojson.Basic.Util.Type_error (message, _) ->
        Result.error (`OSnap_Config_Parse_Error (message, path))
    in
    let* x2 =
      try
        r |> Yojson.Basic.Util.member "x2" |> Yojson.Basic.Util.to_int_option |> Result.ok
      with
      | Yojson.Basic.Util.Type_error (message, _) ->
        Result.error (`OSnap_Config_Parse_Error (message, path))
    in
    let* y2 =
      try
        r |> Yojson.Basic.Util.member "y2" |> Yojson.Basic.Util.to_int_option |> Result.ok
      with
      | Yojson.Basic.Util.Type_error (message, _) ->
        Result.error (`OSnap_Config_Parse_Error (message, path))
    in
    let* selector =
      try
        r
        |> Yojson.Basic.Util.member "selector"
        |> Yojson.Basic.Util.to_string_option
        |> Result.ok
      with
      | Yojson.Basic.Util.Type_error (message, _) ->
        Result.error (`OSnap_Config_Parse_Error (message, path))
    in
    let* selector_all =
      try
        r
        |> Yojson.Basic.Util.member "selectorAll"
        |> Yojson.Basic.Util.to_string_option
        |> Result.ok
      with
      | Yojson.Basic.Util.Type_error (message, _) ->
        Result.error (`OSnap_Config_Parse_Error (message, path))
    in
    Common.collect_ignore ~path ~size_restriction ~selector ~selector_all ~x1 ~y1 ~x2 ~y2
  ;;

  let parse_single_test ~path (global_config : OSnap_Config_Types.global) test =
    let* name =
      try
        test
        |> Yojson.Basic.Util.member "name"
        |> Yojson.Basic.Util.to_string
        |> Result.ok
      with
      | Yojson.Basic.Util.Type_error (message, _) ->
        Result.error (`OSnap_Config_Parse_Error (message, path))
    in
    let* only =
      try
        test
        |> Yojson.Basic.Util.member "only"
        |> Yojson.Basic.Util.to_bool_option
        |> Option.value ~default:false
        |> Result.ok
      with
      | Yojson.Basic.Util.Type_error (message, _) ->
        Result.error (`OSnap_Config_Parse_Error (message, path))
    in
    let* skip =
      try
        test
        |> Yojson.Basic.Util.member "skip"
        |> Yojson.Basic.Util.to_bool_option
        |> Option.value ~default:false
        |> Result.ok
      with
      | Yojson.Basic.Util.Type_error (message, _) ->
        Result.error (`OSnap_Config_Parse_Error (message, path))
    in
    let* threshold =
      try
        test
        |> Yojson.Basic.Util.member "threshold"
        |> Yojson.Basic.Util.to_int_option
        |> Option.value ~default:global_config.threshold
        |> Result.ok
      with
      | Yojson.Basic.Util.Type_error (message, _) ->
        Result.error (`OSnap_Config_Parse_Error (message, path))
    in
    let* url =
      try
        test |> Yojson.Basic.Util.member "url" |> Yojson.Basic.Util.to_string |> Result.ok
      with
      | Yojson.Basic.Util.Type_error (message, _) ->
        Result.error (`OSnap_Config_Parse_Error (message, path))
    in
    let* sizes =
      test
      |> Yojson.Basic.Util.member "sizes"
      |> function
      | `Null -> Result.ok global_config.default_sizes
      | `List list ->
        list
        |> OSnap_Utils.List.map_until_exception (OSnap_Config_Utils.JSON.parse_size ~path)
      | _ -> Result.error (`OSnap_Config_Invalid ("sizes has an invalid format.", path))
    in
    let* actions =
      test
      |> Yojson.Basic.Util.member "actions"
      |> function
      | `List list ->
        list
        |> OSnap_Utils.List.map_until_exception
             (OSnap_Config_Utils.JSON.parse_action
                ~global_fns:global_config.functions
                ~path)
        |> Result.map List.flatten
      | _ -> Result.ok []
    in
    let* ignore =
      test
      |> Yojson.Basic.Util.member "ignore"
      |> function
      | `List list -> OSnap_Utils.List.map_until_exception (parse_ignore ~path) list
      | _ -> Result.ok []
    in
    let* () = Common.collect_duplicates sizes in
    Result.ok { only; skip; threshold; name; url; sizes; actions; ignore }
  ;;

  let parse global_config path =
    let config = OSnap_Utils.get_file_contents path in
    let json = config |> Yojson.Basic.from_string ~fname:path in
    try
      json
      |> Yojson.Basic.Util.to_list
      |> OSnap_Utils.List.map_until_exception (parse_single_test ~path global_config)
    with
    | Yojson.Basic.Util.Type_error (message, _) ->
      Result.error (`OSnap_Config_Parse_Error (message, path))
  ;;
end

module YAML = struct
  let parse_ignore ~path r =
    let* size_restriction =
      r |> OSnap_Config_Utils.YAML.get_string_list_option ~path "@"
    in
    let* x1 = r |> OSnap_Config_Utils.YAML.get_int_option ~path "x1" in
    let* y1 = r |> OSnap_Config_Utils.YAML.get_int_option ~path "y1" in
    let* x2 = r |> OSnap_Config_Utils.YAML.get_int_option ~path "x2" in
    let* y2 = r |> OSnap_Config_Utils.YAML.get_int_option ~path "y2" in
    let* selector = r |> OSnap_Config_Utils.YAML.get_string_option ~path "selector" in
    let* selector_all =
      r |> OSnap_Config_Utils.YAML.get_string_option ~path "selectorAll"
    in
    Common.collect_ignore ~path ~size_restriction ~selector ~selector_all ~x1 ~y1 ~x2 ~y2
  ;;

  let parse_single_test ~path (global_config : OSnap_Config_Types.global) test =
    let* name = test |> OSnap_Config_Utils.YAML.get_string ~path "name" in
    let* url = test |> OSnap_Config_Utils.YAML.get_string ~path "url" in
    let* only =
      test
      |> OSnap_Config_Utils.YAML.get_bool_option ~path "only"
      |> Result.map (Option.value ~default:false)
    in
    let* skip =
      test
      |> OSnap_Config_Utils.YAML.get_bool_option ~path "skip"
      |> Result.map (Option.value ~default:false)
    in
    let* threshold =
      test
      |> OSnap_Config_Utils.YAML.get_int_option ~path "threshold"
      |> Result.map (Option.value ~default:global_config.threshold)
    in
    let* sizes =
      test
      |> OSnap_Config_Utils.YAML.get_list_option
           ~path
           "sizes"
           ~parser:(OSnap_Config_Utils.YAML.parse_size ~path)
      |> Result.map (Option.value ~default:global_config.default_sizes)
    in
    let* actions =
      test
      |> OSnap_Config_Utils.YAML.get_list_option
           ~path
           "actions"
           ~parser:
             (OSnap_Config_Utils.YAML.parse_action
                ~global_fns:global_config.functions
                ~path)
      |> Result.map (Option.value ~default:[])
      |> Result.map List.flatten
    in
    let* ignore =
      test
      |> OSnap_Config_Utils.YAML.get_list_option
           ~path
           "ignore"
           ~parser:(parse_ignore ~path)
      |> Result.map (Option.value ~default:[])
    in
    let* () = Common.collect_duplicates sizes in
    Result.ok { only; skip; threshold; name; url; sizes; actions; ignore }
  ;;

  let parse global_config path =
    let config = OSnap_Utils.get_file_contents path in
    let* yaml =
      config
      |> Yaml.of_string
      |> Result.map_error (fun _ ->
           `OSnap_Config_Parse_Error ("YAML could not be parsed", path))
    in
    yaml
    |> (function
         | `A lst -> Result.ok lst
         | _ ->
           Result.error
             (`OSnap_Config_Parse_Error
               ("A test file has to be an array of tests.", path)))
    |> Result.map
         (OSnap_Utils.List.map_until_exception (parse_single_test ~path global_config))
    |> Result.join
  ;;
end

let find ?(root_path = "/") ?(pattern = "**/*.osnap.json") ?(ignore_patterns = []) () =
  let _, path_segments, pattern_segments =
    pattern
    |> String.split_on_char Filename.dir_sep.[0]
    |> List.fold_left
         (fun acc curr ->
           match acc, curr with
           | (true, left, right), s -> true, left, s :: right
           | (false, left, right), s when s = "**" || String.starts_with ~prefix:"*" s ->
             true, left, s :: right
           | (false, left, right), s -> false, s :: left, right)
         (false, [], [])
  in
  let pattern = pattern_segments |> OSnap_Utils.path_of_segments in
  let* root_path =
    try
      FilePath.make_absolute
        (FileUtil.pwd ())
        (Filename.concat root_path (OSnap_Utils.path_of_segments path_segments))
      |> Unix.realpath
      |> Result.ok
    with
    | _ ->
      Result.error
        (`OSnap_Config_Global_Invalid
          "The testPattern path could not be resolved. Please make sure it exists")
  in
  let pattern = pattern |> Re.Glob.glob |> Re.compile in
  let ignore_patterns =
    ignore_patterns |> List.map (fun pattern -> pattern |> Re.Glob.glob |> Re.compile)
  in
  let is_ignored path = ignore_patterns |> List.exists (fun __x -> Re.execp __x path) in
  FileUtil.find
    (Custom (fun path -> (not (is_ignored path)) && Re.execp pattern path))
    root_path
    (fun acc curr -> curr :: acc)
    []
  |> OSnap_Utils.List.map_until_exception (fun path ->
       let* format = OSnap_Config_Utils.get_format path in
       Result.ok (path, format))
;;

let init config =
  let* tests =
    find
      ~root_path:config.root_path
      ~pattern:config.test_pattern
      ~ignore_patterns:config.ignore_patterns
      ()
  in
  let* tests =
    tests
    |> OSnap_Utils.List.map_until_exception (fun (path, test_format) ->
         match test_format with
         | OSnap_Config_Types.JSON -> JSON.parse config path
         | OSnap_Config_Types.YAML -> YAML.parse config path)
    |> Result.map List.flatten
  in
  let duplicates =
    tests |> OSnap_Utils.find_duplicates (fun (t : OSnap_Config_Types.test) -> t.name)
  in
  match duplicates with
  | [] -> Result.ok tests
  | duplicates ->
    Result.error
      (`OSnap_Config_Duplicate_Tests
        (duplicates |> List.map (fun (t : OSnap_Config_Types.test) -> t.name)))
;;
