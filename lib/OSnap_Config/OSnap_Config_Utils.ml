let get_format path =
  path
  |> Eio.Path.split
  |> Option.map snd
  |> Option.value ~default:""
  |> Filename.extension
  |> String.lowercase_ascii
  |> function
  | ".json" -> Result.ok OSnap_Config_Types.JSON
  | ".yaml" -> Result.ok OSnap_Config_Types.YAML
  | _ -> Result.error (`OSnap_Config_Unsupported_Format path)
;;

let to_result_option = function
  | None -> Ok None
  | Some (Ok v) -> Ok (Some v)
  | Some (Error e) -> Error e
;;

let collect_action
  ~global_fns
  ~path
  ~selector
  ~size_restriction
  ~name
  ~text
  ~timeout
  ~px
  action
  =
  match action with
  | "scroll" ->
    (match selector, px with
     | None, None ->
       Result.error
         (`OSnap_Config_Invalid
           ( "Neither selector nor px was provided for scroll action. Please provide one \
              of them."
           , path ))
     | Some _, Some _ ->
       Result.error
         (`OSnap_Config_Invalid
           ( "Both selector and px were provided for scroll action. Please provide only \
              one of them."
           , path ))
     | None, Some px ->
       [ OSnap_Config_Types.Scroll (`PxAmount px, size_restriction) ] |> Result.ok
     | Some selector, None ->
       [ OSnap_Config_Types.Scroll (`Selector selector, size_restriction) ] |> Result.ok)
  | "click" ->
    (match selector with
     | None ->
       Result.error
         (`OSnap_Config_Invalid ("no selector for click action provided", path))
     | Some selector ->
       [ OSnap_Config_Types.Click (selector, size_restriction) ] |> Result.ok)
  | "type" ->
    (match selector, text with
     | None, _ ->
       Result.error (`OSnap_Config_Invalid ("no selector for type action provided", path))
     | _, None ->
       Result.error (`OSnap_Config_Invalid ("no text for type action provided", path))
     | Some selector, Some text ->
       [ OSnap_Config_Types.Type (selector, text, size_restriction) ] |> Result.ok)
  | "wait" ->
    (match timeout with
     | None ->
       Result.error (`OSnap_Config_Invalid ("no timeout for wait action provided", path))
     | Some timeout ->
       [ OSnap_Config_Types.Wait (timeout, size_restriction) ] |> Result.ok)
  | "function" ->
    (match name with
     | None ->
       Result.error (`OSnap_Config_Invalid ("no name for function action provided", path))
     | Some name ->
       let actions =
         global_fns
         |> List.find_map (function
           | n, actions when n = name -> Some actions
           | _ -> None)
       in
       (match actions with
        | None -> Result.error (`OSnap_Config_Undefined_Function (name, path))
        | Some actions ->
          actions
          |> List.map (function
            | OSnap_Config_Types.Scroll (a, _) ->
              OSnap_Config_Types.Scroll (a, size_restriction)
            | OSnap_Config_Types.Click (a, _) ->
              OSnap_Config_Types.Click (a, size_restriction)
            | OSnap_Config_Types.Type (a, b, _) ->
              OSnap_Config_Types.Type (a, b, size_restriction)
            | OSnap_Config_Types.Wait (t, _) ->
              OSnap_Config_Types.Wait (t, size_restriction))
          |> Result.ok))
  | action ->
    Result.error
      (`OSnap_Config_Invalid (Printf.sprintf "found unknown action %S" action, path))
;;

module JSON = struct
  let ( let* ) = Result.bind

  let parse_size ~path size =
    let* name =
      try
        size
        |> Yojson.Basic.Util.member "name"
        |> Yojson.Basic.Util.to_string_option
        |> Result.ok
      with
      | Yojson.Basic.Util.Type_error (message, _) ->
        Result.error (`OSnap_Config_Parse_Error (message, path))
    in
    let* width =
      try
        size
        |> Yojson.Basic.Util.member "width"
        |> (function
              | `Null ->
                Result.error
                  (`OSnap_Config_Parse_Error
                    ( "defaultSize has an invalid format. \"width\" is required but not \
                       provided!"
                    , path ))
              | v -> Result.ok v)
        |> Result.map Yojson.Basic.Util.to_int
      with
      | Yojson.Basic.Util.Type_error (message, _) ->
        Result.error (`OSnap_Config_Parse_Error (message, path))
    in
    let* height =
      try
        size
        |> Yojson.Basic.Util.member "height"
        |> (function
              | `Null ->
                Result.error
                  (`OSnap_Config_Parse_Error
                    ( "defaultSize has an invalid format. \"height\" is required but not \
                       provided!"
                    , path ))
              | v -> Result.ok v)
        |> Result.map Yojson.Basic.Util.to_int
      with
      | Yojson.Basic.Util.Type_error (message, _) ->
        Result.error (`OSnap_Config_Parse_Error (message, path))
    in
    OSnap_Config_Types.{ name; width; height } |> Result.ok
  ;;

  let parse_action ~global_fns ~path a =
    let* action =
      try
        a |> Yojson.Basic.Util.member "action" |> Yojson.Basic.Util.to_string |> Result.ok
      with
      | Yojson.Basic.Util.Type_error (message, _) ->
        Result.error (`OSnap_Config_Parse_Error (message, path))
    in
    let* size_restriction =
      try
        a
        |> Yojson.Basic.Util.member "@"
        |> Yojson.Basic.Util.to_option Yojson.Basic.Util.to_list
        |> Option.map (List.map Yojson.Basic.Util.to_string)
        |> Result.ok
      with
      | Yojson.Basic.Util.Type_error (message, _) ->
        Result.error (`OSnap_Config_Parse_Error (message, path))
    in
    let* px =
      try
        a |> Yojson.Basic.Util.member "px" |> Yojson.Basic.Util.to_int_option |> Result.ok
      with
      | Yojson.Basic.Util.Type_error (message, _) ->
        Result.error (`OSnap_Config_Parse_Error (message, path))
    in
    let* selector =
      try
        a
        |> Yojson.Basic.Util.member "selector"
        |> Yojson.Basic.Util.to_string_option
        |> Result.ok
      with
      | Yojson.Basic.Util.Type_error (message, _) ->
        Result.error (`OSnap_Config_Parse_Error (message, path))
    in
    let* text =
      try
        a
        |> Yojson.Basic.Util.member "text"
        |> Yojson.Basic.Util.to_string_option
        |> Result.ok
      with
      | Yojson.Basic.Util.Type_error (message, _) ->
        Result.error (`OSnap_Config_Parse_Error (message, path))
    in
    let* timeout =
      try
        a
        |> Yojson.Basic.Util.member "timeout"
        |> Yojson.Basic.Util.to_int_option
        |> Result.ok
      with
      | Yojson.Basic.Util.Type_error (message, _) ->
        Result.error (`OSnap_Config_Parse_Error (message, path))
    in
    let* name =
      try
        a
        |> Yojson.Basic.Util.member "name"
        |> Yojson.Basic.Util.to_string_option
        |> Result.ok
      with
      | Yojson.Basic.Util.Type_error (message, _) ->
        Result.error (`OSnap_Config_Parse_Error (message, path))
    in
    collect_action
      ~global_fns
      ~path
      ~selector
      ~size_restriction
      ~text
      ~timeout
      ~name
      ~px
      action
  ;;
end

module YAML = struct
  let ( let* ) = Result.bind

  let get_list_option ~path ~parser key obj =
    obj
    |> Yaml.Util.find key
    |> Result.map_error (function `Msg message ->
      `OSnap_Config_Parse_Error (message, path))
    |> Result.map
         (Option.map (fun v ->
            match v with
            | `A l -> Result.ok l
            | `Bool _ ->
              Result.error
                (`OSnap_Config_Parse_Error
                  ( Printf.sprintf
                      "%S is in an invalid format. Expected array, got boolean."
                      key
                  , path ))
            | `Float _ ->
              Result.error
                (`OSnap_Config_Parse_Error
                  ( Printf.sprintf
                      "%S is in an invalid format. Expected array, got number."
                      key
                  , path ))
            | `Null ->
              Result.error
                (`OSnap_Config_Parse_Error
                  ( Printf.sprintf
                      "%S is in an invalid format. Expected array, got null."
                      key
                  , path ))
            | `O _ ->
              Result.error
                (`OSnap_Config_Parse_Error
                  ( Printf.sprintf
                      "%S is in an invalid format. Expected array, got object."
                      key
                  , path ))
            | `String _ ->
              Result.error
                (`OSnap_Config_Parse_Error
                  ( Printf.sprintf
                      "%S is in an invalid format. Expected array, got string."
                      key
                  , path ))))
    |> Result.map to_result_option
    |> Result.join
    |> Result.map (Option.map (OSnap_Utils.List.map_until_exception parser))
    |> Result.map to_result_option
    |> Result.join
  ;;

  let get_string_list_option ~path key obj =
    let parser v =
      Yaml.Util.to_string v
      |> Result.map_error (function `Msg message ->
        `OSnap_Config_Parse_Error (message, path))
    in
    get_list_option ~path ~parser key obj
  ;;

  let get_string_option ~path key obj =
    obj
    |> Yaml.Util.find key
    |> Result.map (Option.map Yaml.Util.to_string)
    |> Result.map to_result_option
    |> Result.join
    |> Result.map_error (function `Msg message ->
      `OSnap_Config_Parse_Error (message, path))
  ;;

  let get_string ~path ?(additional_error_message = "") key obj =
    obj
    |> Yaml.Util.find key
    |> Result.map (Option.map Yaml.Util.to_string)
    |> Result.map to_result_option
    |> Result.join
    |> function
    | Ok (Some string) -> Result.ok string
    | Ok None ->
      Result.error
        (`OSnap_Config_Parse_Error
          ( Printf.sprintf
              "%S is required but not provided! %s"
              key
              additional_error_message
          , path ))
    | Error (`Msg message) -> Result.error (`OSnap_Config_Parse_Error (message, path))
  ;;

  let get_bool_option ~path key obj =
    obj
    |> Yaml.Util.find key
    |> Result.map (Option.map Yaml.Util.to_bool)
    |> Result.map to_result_option
    |> Result.join
    |> Result.map_error (function `Msg message ->
      `OSnap_Config_Parse_Error (message, path))
  ;;

  let get_bool ~path ?(additional_error_message = "") key obj =
    obj
    |> Yaml.Util.find key
    |> Result.map (Option.map Yaml.Util.to_bool)
    |> Result.map to_result_option
    |> Result.join
    |> function
    | Ok (Some string) -> Result.ok string
    | Ok None ->
      Result.error
        (`OSnap_Config_Parse_Error
          ( Printf.sprintf
              "%S is required but not provided! %s"
              key
              additional_error_message
          , path ))
    | Error (`Msg message) -> Result.error (`OSnap_Config_Parse_Error (message, path))
  ;;

  let get_int ~path ?(additional_error_message = "") key obj =
    obj
    |> Yaml.Util.find key
    |> Result.map (Option.map Yaml.Util.to_float)
    |> Result.map to_result_option
    |> Result.join
    |> Result.map (Option.map Float.to_int)
    |> function
    | Ok (Some number) -> Result.ok number
    | Ok None ->
      Result.error
        (`OSnap_Config_Parse_Error
          ( Printf.sprintf
              "%S is required but not provided! %s"
              key
              additional_error_message
          , path ))
    | Error (`Msg message) -> Result.error (`OSnap_Config_Parse_Error (message, path))
  ;;

  let get_int_option ~path key obj =
    obj
    |> Yaml.Util.find key
    |> Result.map (Option.map Yaml.Util.to_float)
    |> Result.map to_result_option
    |> Result.join
    |> Result.map (Option.map Float.to_int)
    |> Result.map_error (function `Msg message ->
      `OSnap_Config_Parse_Error (message, path))
  ;;

  let parse_size ~path size =
    let* name = size |> get_string_option ~path "name" in
    let* height = size |> get_int ~path "height" in
    let* width = size |> get_int ~path "width" in
    OSnap_Config_Types.{ name; width; height } |> Result.ok
  ;;

  let parse_action ~global_fns ~path a =
    let* size_restriction = a |> get_string_list_option ~path "@" in
    let* action = a |> get_string ~path "action" in
    let* selector = a |> get_string_option ~path "selector" in
    let* px = a |> get_int_option ~path "px" in
    let* name = a |> get_string_option ~path "name" in
    let* text = a |> get_string_option ~path "text" in
    let* timeout = a |> get_int_option ~path "timeout" in
    collect_action
      ~global_fns
      ~path
      ~selector
      ~size_restriction
      ~text
      ~name
      ~timeout
      ~px
      action
  ;;
end
