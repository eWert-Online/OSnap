module Config = OSnap_Config
module Browser = OSnap_Browser
module Diff = OSnap_Diff
module Printer = OSnap_Test_Printer
module Types = OSnap_Test_Types
open OSnap_Utils
open OSnap_Test_Types

let save_screenshot ~path data =
  let*? io =
    try Lwt_io.open_file ~mode:Output path |> Lwt_result.ok with
    | _ ->
      `OSnap_FS_Error (Printf.sprintf "Could not save screenshot to %s" path)
      |> Lwt_result.fail
  in
  let*? () = Lwt_io.write io data |> Lwt_result.ok in
  Lwt_io.close io |> Lwt_result.ok
;;

let read_file_contents ~path =
  let*? io =
    try Lwt_io.open_file ~mode:Input path |> Lwt_result.ok with
    | _ ->
      `OSnap_FS_Error (Printf.sprintf "Could not open file %S for reading" path)
      |> Lwt_result.fail
  in
  let*? data = Lwt_io.read io |> Lwt_result.ok in
  let*? () = Lwt_io.close io |> Lwt_result.ok in
  Lwt_result.return data
;;

let execute_actions ~document ~target ?size_name actions =
  let open Config.Types in
  let run action =
    match action, size_name with
    | Scroll (_, Some _), None -> Lwt_result.return ()
    | Scroll (`Selector selector, None), _ ->
      target |> Browser.Actions.scroll ~document ~selector:(Some selector) ~px:None
    | Scroll (`PxAmount px, None), _ ->
      target |> Browser.Actions.scroll ~document ~selector:None ~px:(Some px)
    | Scroll (`Selector selector, Some size_restr), Some size_name ->
      if size_restr |> List.mem size_name
      then target |> Browser.Actions.scroll ~document ~selector:(Some selector) ~px:None
      else Lwt_result.return ()
    | Scroll (`PxAmount px, Some size_restr), Some size_name ->
      if size_restr |> List.mem size_name
      then target |> Browser.Actions.scroll ~document ~selector:None ~px:(Some px)
      else Lwt_result.return ()
    | Click (_, Some _), None -> Lwt_result.return ()
    | Click (selector, None), _ -> target |> Browser.Actions.click ~document ~selector
    | Click (selector, Some size_restr), Some size_name ->
      if size_restr |> List.mem size_name
      then target |> Browser.Actions.click ~document ~selector
      else Lwt_result.return ()
    | Type (_, _, Some _), None -> Lwt_result.return ()
    | Type (selector, text, None), _ ->
      target |> Browser.Actions.type_text ~document ~selector ~text
    | Type (selector, text, Some size), Some size_name ->
      if size |> List.mem size_name
      then target |> Browser.Actions.type_text ~document ~selector ~text
      else Lwt_result.return ()
    | Wait (_, Some _), None -> Lwt_result.return ()
    | Wait (ms, Some size), Some size_name ->
      if size |> List.mem size_name
      then (
        let timeout = float_of_int ms /. 1000.0 in
        Lwt_unix.sleep timeout |> Lwt_result.ok)
      else Lwt_result.return ()
    | Wait (ms, None), _ ->
      let timeout = float_of_int ms /. 1000.0 in
      Lwt_unix.sleep timeout |> Lwt_result.ok
  in
  actions
  |> Lwt_list.fold_left_s
       (fun acc curr ->
         let* result = run curr in
         match result with
         | Ok () -> Lwt.return acc
         | Error (`OSnap_Selector_Not_Found _s) -> Lwt.return acc
         | Error (`OSnap_Selector_Not_Visible _s) -> Lwt.return acc
         | Error (`OSnap_CDP_Protocol_Error _) as e -> Lwt.return e)
       (Result.ok ())
;;

let get_ignore_regions ~document target size_name regions =
  let open Config.Types in
  let get_ignore_region = function
    | Coordinates (a, b, _) -> Lwt_result.return [ a, b ]
    | SelectorAll (selector, _) ->
      let*? quads = target |> Browser.Actions.get_quads_all ~document ~selector in
      quads
      |> List.map (fun ((x1, y1), (x2, y2)) ->
           let x1 = Int.of_float x1 in
           let y1 = Int.of_float y1 in
           let x2 = Int.of_float x2 in
           let y2 = Int.of_float y2 in
           (x1, y1), (x2, y2))
      |> Lwt_result.return
    | Selector (selector, _) ->
      let*? (x1, y1), (x2, y2) =
        target |> Browser.Actions.get_quads ~document ~selector
      in
      let x1 = Int.of_float x1 in
      let y1 = Int.of_float y1 in
      let x2 = Int.of_float x2 in
      let y2 = Int.of_float y2 in
      Lwt_result.return [ (x1, y1), (x2, y2) ]
  in
  let* regions =
    regions
    |> List.filter (fun region ->
         match region, size_name with
         | Coordinates (_a, _b, None), _ -> true
         | Coordinates (_, _, Some _), None -> false
         | Coordinates (_a, _b, Some size_restr), Some size_name ->
           List.mem size_name size_restr
         | Selector (_, Some _), None -> false
         | Selector (_, Some size_restr), Some size_name -> List.mem size_name size_restr
         | Selector (_selector, None), _ -> true
         | SelectorAll (_, Some _), None -> false
         | SelectorAll (_, Some size_restr), Some size_name ->
           List.mem size_name size_restr
         | SelectorAll (_selector, None), _ -> true)
    |> Lwt_list.map_p get_ignore_region
  in
  regions
  |> List.filter_map (function
       | Ok regions -> Some (Lwt_result.return regions)
       | Error (`OSnap_Selector_Not_Found _s) -> None
       | Error (`OSnap_Selector_Not_Visible _s) -> None
       | Error (`OSnap_CDP_Protocol_Error _ as e) -> Some (Lwt_result.fail e))
  |> Lwt_list.map_p_until_exception Fun.id
  |> Lwt_result.map List.flatten
;;

let get_filename ?(diff = false) name width height =
  if diff
  then Printf.sprintf "diff_%s_%ix%i.png" name width height
  else Printf.sprintf "%s_%ix%i.png" name width height
;;

let run (global_config : Config.Types.global) target test =
  if test.skip
  then (
    Printer.skipped_message ~name:test.name ~width:test.width ~height:test.height;
    { test with result = Some `Skipped } |> Lwt_result.return)
  else (
    let dirs = OSnap_Paths.get global_config in
    let filename = get_filename test.name test.width test.height in
    let diff_filename = get_filename ~diff:true test.name test.width test.height in
    let url = global_config.base_url ^ test.url in
    let base_snapshot = dirs.base ^ filename in
    let updated_snapshot = dirs.updated ^ filename in
    let diff_image = dirs.diff ^ diff_filename in
    let*? () = target |> Browser.Actions.clear_cookies in
    let*? () =
      target
      |> Browser.Actions.set_size ~width:(`Int test.width) ~height:(`Int test.height)
    in
    let*? loaderId = target |> Browser.Actions.go_to ~url in
    let*? () =
      target |> Browser.Actions.wait_for_network_idle ~loaderId |> Lwt_result.ok
    in
    let*? document = target |> Browser.Actions.get_document in
    let* () =
      target
      |> Browser.Actions.mousemove
           ~document
           ~to_:(`Coordinates (`Int (-100), `Int (-100)))
      |> Lwt.map ignore
    in
    let*? () =
      test.actions |> execute_actions ~document ~target ?size_name:test.size_name
    in
    let*? screenshot =
      target
      |> Browser.Actions.screenshot ~full_size:global_config.fullscreen
      |> Lwt_result.map Base64.decode_exn
    in
    let*? result =
      if not test.exists
      then (
        let*? () = save_screenshot ~path:base_snapshot screenshot in
        Printer.created_message ~name:test.name ~width:test.width ~height:test.height;
        Lwt_result.return `Created)
      else
        let*? original_image_data = read_file_contents ~path:base_snapshot in
        if original_image_data = screenshot
        then (
          Printer.success_message ~name:test.name ~width:test.width ~height:test.height;
          Lwt_result.return `Passed)
        else
          let*? ignoreRegions =
            test.ignore_regions |> get_ignore_regions ~document target test.size_name
          in
          let diff =
            Diff.diff
              ~threshold:test.threshold
              ~diffPixel:global_config.diff_pixel_color
              ~ignoreRegions
              ~output:diff_image
              ~original_image_data
              ~new_image_data:screenshot
          in
          match diff () with
          | Ok () ->
            Printer.success_message ~name:test.name ~width:test.width ~height:test.height;
            Lwt_result.return `Passed
          | Error Io ->
            Printer.corrupted_message
              ~print_head:true
              ~name:test.name
              ~width:test.width
              ~height:test.height;
            Lwt_result.return (`Failed `Io)
          | Error Layout ->
            Printer.layout_message
              ~print_head:true
              ~name:test.name
              ~width:test.width
              ~height:test.height;
            let*? () = save_screenshot screenshot ~path:updated_snapshot in
            Lwt_result.return (`Failed `Layout)
          | Error (Pixel (diffCount, diffPercentage)) ->
            Printer.diff_message
              ~print_head:true
              ~name:test.name
              ~width:test.width
              ~height:test.height
              ~diffCount
              ~diffPercentage;
            let*? () = save_screenshot screenshot ~path:updated_snapshot in
            Lwt_result.return (`Failed (`Pixel (diffCount, diffPercentage)))
    in
    { test with result = Some result } |> Lwt_result.return)
;;
