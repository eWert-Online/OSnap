module Config = OSnap_Config
module Browser = OSnap_Browser
module Diff = OSnap_Diff
module Printer = OSnap_Test_Printer
module Types = OSnap_Test_Types
open OSnap_Utils
open OSnap_Test_Types

let save_screenshot ~path data = Eio.Path.save ~create:(`Or_truncate 0o666) path data
let read_file_contents ~path = Eio.Path.load path

let execute_actions ~clock ~document ~target ?size_name actions =
  let open Config.Types in
  let run action =
    match action, size_name with
    | Scroll (_, Some _), None -> Result.ok ()
    | Scroll (`Selector selector, None), _ ->
      target |> Browser.Actions.scroll ~clock ~document ~selector:(Some selector) ~px:None
    | Scroll (`PxAmount px, None), _ ->
      target |> Browser.Actions.scroll ~clock ~document ~selector:None ~px:(Some px)
    | Scroll (`Selector selector, Some size_restr), Some size_name ->
      if size_restr |> List.mem size_name
      then
        target
        |> Browser.Actions.scroll ~clock ~document ~selector:(Some selector) ~px:None
      else Result.ok ()
    | Scroll (`PxAmount px, Some size_restr), Some size_name ->
      if size_restr |> List.mem size_name
      then target |> Browser.Actions.scroll ~clock ~document ~selector:None ~px:(Some px)
      else Result.ok ()
    | Click (_, Some _), None -> Result.ok ()
    | Click (selector, None), _ ->
      target |> Browser.Actions.click ~clock ~document ~selector
    | Click (selector, Some size_restr), Some size_name ->
      if size_restr |> List.mem size_name
      then target |> Browser.Actions.click ~clock ~document ~selector
      else Result.ok ()
    | Type (_, _, Some _), None -> Result.ok ()
    | Type (selector, text, None), _ ->
      target |> Browser.Actions.type_text ~clock ~document ~selector ~text
    | Type (selector, text, Some size), Some size_name ->
      if size |> List.mem size_name
      then target |> Browser.Actions.type_text ~clock ~document ~selector ~text
      else Result.ok ()
    | Wait (_, Some _), None -> Result.ok ()
    | Wait (ms, Some size), Some size_name ->
      if size |> List.mem size_name
      then (
        let timeout = float_of_int ms /. 1000.0 in
        Eio.Time.sleep clock timeout |> Result.ok)
      else Result.ok ()
    | Wait (ms, None), _ ->
      let timeout = float_of_int ms /. 1000.0 in
      Eio.Time.sleep clock timeout |> Result.ok
  in
  actions
  |> List.fold_left
       (fun acc curr ->
         let result = run curr in
         match result with
         | Ok () -> acc
         | Error (`OSnap_Selector_Not_Found _s) -> acc
         | Error (`OSnap_Selector_Not_Visible _s) -> acc
         | Error (`OSnap_CDP_Protocol_Error _) as e -> e)
       (Result.ok ())
;;

let get_ignore_regions ~document target size_name regions =
  let ( let*? ) = Result.bind in
  let open Config.Types in
  let get_ignore_region = function
    | Coordinates (a, b, _) -> Result.ok [ a, b ]
    | SelectorAll (selector, _) ->
      let*? quads = target |> Browser.Actions.get_quads_all ~document ~selector in
      quads
      |> List.map (fun ((x1, y1), (x2, y2)) ->
           let x1 = Int.of_float x1 in
           let y1 = Int.of_float y1 in
           let x2 = Int.of_float x2 in
           let y2 = Int.of_float y2 in
           (x1, y1), (x2, y2))
      |> Result.ok
    | Selector (selector, _) ->
      let*? (x1, y1), (x2, y2) =
        target |> Browser.Actions.get_quads ~document ~selector
      in
      let x1 = Int.of_float x1 in
      let y1 = Int.of_float y1 in
      let x2 = Int.of_float x2 in
      let y2 = Int.of_float y2 in
      Result.ok [ (x1, y1), (x2, y2) ]
  in
  regions
  |> Eio.Fiber.List.filter_map (fun region ->
       let r =
         match region, size_name with
         | Coordinates (_, _, Some _), None
         | Selector (_, Some _), None
         | SelectorAll (_, Some _), None -> None
         | Coordinates (_, _, Some size_restr), Some size_name
         | Selector (_, Some size_restr), Some size_name
         | SelectorAll (_, Some size_restr), Some size_name ->
           if List.mem size_name size_restr then Some (get_ignore_region region) else None
         | Coordinates (_, _, None), _ | Selector (_, None), _ | SelectorAll (_, None), _
           -> Some (get_ignore_region region)
       in
       Option.bind r (function
         | Ok regions -> Some (Result.ok regions)
         | Error (`OSnap_Selector_Not_Found _s) -> None
         | Error (`OSnap_Selector_Not_Visible _s) -> None
         | Error (`OSnap_CDP_Protocol_Error _ as e) -> Some (Result.error e)))
  |> List.map_until_exception Fun.id
  |> Result.map List.flatten
;;

let get_filename ?(diff = false) name width height =
  if diff
  then Printf.sprintf "/diff_%s_%ix%i.png" name width height
  else Printf.sprintf "/%s_%ix%i.png" name width height
;;

let run ~fs ~clock (global_config : Config.Types.global) target test =
  let ( let*? ) = Result.bind in
  if test.skip
  then (
    Printer.skipped_message ~name:test.name ~width:test.width ~height:test.height;
    { test with result = Some `Skipped } |> Result.ok)
  else (
    let dirs = OSnap_Paths.get global_config in
    let filename = get_filename test.name test.width test.height in
    let diff_filename = get_filename ~diff:true test.name test.width test.height in
    let url = global_config.base_url ^ test.url in
    let base_snapshot = Eio.Path.(fs / dirs.base / filename) in
    let updated_snapshot = Eio.Path.(fs / dirs.updated / filename) in
    let diff_image = dirs.diff ^ diff_filename in
    let*? () = target |> Browser.Actions.clear_cookies in
    let*? () =
      target
      |> Browser.Actions.set_size ~width:(`Int test.width) ~height:(`Int test.height)
    in
    let*? loaderId = target |> Browser.Actions.go_to ~url in
    let*? () = target |> Browser.Actions.wait_for_network_idle ~loaderId |> Result.ok in
    let*? document = target |> Browser.Actions.get_document in
    let () =
      target
      |> Browser.Actions.mousemove
           ~document
           ~to_:(`Coordinates (`Int (-100), `Int (-100)))
      |> ignore
    in
    let*? () =
      test.actions |> execute_actions ~clock ~document ~target ?size_name:test.size_name
    in
    let*? screenshot =
      target
      |> Browser.Actions.screenshot ~full_size:global_config.fullscreen
      |> Result.map Base64.decode_exn
    in
    let*? result =
      if not test.exists
      then (
        let () = save_screenshot ~path:base_snapshot screenshot in
        Printer.created_message ~name:test.name ~width:test.width ~height:test.height;
        Result.ok `Created)
      else (
        let original_image_data = read_file_contents ~path:base_snapshot in
        if original_image_data = screenshot
        then (
          Printer.success_message ~name:test.name ~width:test.width ~height:test.height;
          Result.ok `Passed)
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
            Result.ok `Passed
          | Error Io ->
            Printer.corrupted_message
              ~print_head:true
              ~name:test.name
              ~width:test.width
              ~height:test.height;
            Result.ok (`Failed `Io)
          | Error Layout ->
            Printer.layout_message
              ~print_head:true
              ~name:test.name
              ~width:test.width
              ~height:test.height;
            let () = save_screenshot screenshot ~path:updated_snapshot in
            Result.ok (`Failed `Layout)
          | Error (Pixel (diffCount, diffPercentage)) ->
            Printer.diff_message
              ~print_head:true
              ~name:test.name
              ~width:test.width
              ~height:test.height
              ~diffCount
              ~diffPercentage;
            let () = save_screenshot screenshot ~path:updated_snapshot in
            Result.ok (`Failed (`Pixel (diffCount, diffPercentage))))
    in
    { test with result = Some result } |> Result.ok)
;;
