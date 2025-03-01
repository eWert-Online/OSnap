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
    let* retry =
      test
      |> OSnap_Config_Utils.YAML.get_int_option ~path "retry"
      |> Result.map (Option.value ~default:global_config.retry)
    in
    let* sizes =
      test
      |> OSnap_Config_Utils.YAML.get_list_option
           ~path
           "sizes"
           ~parser:
             (OSnap_Config_Utils.YAML.parse_size
                ~default_sizes:global_config.default_sizes
                ~path)
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
    Result.ok { only; skip; threshold; retry; name; url; sizes; actions; ignore }
  ;;

  let parse global_config path =
    let config = Eio.Path.load path in
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
         (`OSnap_Config_Parse_Error ("A test file has to be an array of tests.", path)))
    |> Result.map
         (OSnap_Utils.List.map_until_exception (parse_single_test ~path global_config))
    |> Result.join
  ;;
end

let find ~root_path ~pattern ~ignore_patterns =
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
      Eio.Path.(root_path / OSnap_Utils.path_of_segments path_segments) |> Result.ok
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
  let is_ignored path =
    ignore_patterns |> List.exists (fun pattern -> Re.execp pattern path)
  in
  let rec find_matching_files files = function
    | dir :: rest when Eio.Path.is_directory dir ->
      dir
      |> Eio.Path.read_dir
      |> List.map (fun file -> Eio.Path.(dir / file))
      |> List.append rest
      |> find_matching_files files
    | file :: rest
      when (not (is_ignored (Eio.Path.native_exn file)))
           && Re.execp pattern (Eio.Path.native_exn file) ->
      rest |> find_matching_files (file :: files)
    | _ :: rest -> rest |> find_matching_files files
    | [] -> files
  in
  Result.ok @@ find_matching_files [] [ root_path ]
;;

let init config =
  let* tests =
    find
      ~root_path:config.root_path
      ~pattern:config.test_pattern
      ~ignore_patterns:config.ignore_patterns
  in
  let* tests =
    tests
    |> OSnap_Utils.List.map_until_exception (YAML.parse config)
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
