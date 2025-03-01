open Fmt
open OSnap_Test_Types

module Progress = struct
  type t =
    { mutable current : int
    ; mutable total : int
    ; mutable total_length : int
    }

  let progress = { current = 0; total = 0; total_length = 0 }
  let progress_mutex = Mutex.create ()

  let get_and_incr () =
    Mutex.lock progress_mutex;
    progress.current <- succ progress.current;
    Mutex.unlock progress_mutex;
    Fmt.str_like
      Fmt.stdout
      "%a"
      (styled `Faint string)
      (Printf.sprintf "%*i / %i " progress.total_length progress.current progress.total)
  ;;

  let none () =
    Fmt.str_like
      Fmt.stdout
      "%a"
      (styled `Faint string)
      (Printf.sprintf "%*s / %i " progress.total_length "-" progress.total)
  ;;

  let set_total i =
    Mutex.lock progress_mutex;
    progress.total <- i;
    progress.total_length <- String.length (string_of_int i);
    Mutex.unlock progress_mutex
  ;;
end

let test_name ~name ~width ~height =
  Fmt.str_like
    Fmt.stdout
    "%s %a"
    name
    (styled `Faint string)
    (Printf.sprintf "@ %i×%i" width height)
;;

let created_message ~name ~width ~height =
  Fmt.pr
    "%s %a %s @."
    (Progress.get_and_incr ())
    (styled `Bold (styled `Blue string))
    "CREATE"
    (test_name ~name ~width ~height)
;;

let skipped_message ~name ~width ~height =
  Fmt.pr
    "%s %a %s @."
    (Progress.get_and_incr ())
    (styled `Bold (styled `Magenta string))
    "SKIP"
    (test_name ~name ~width ~height)
;;

let retry_message ~count ~name ~width ~height =
  Fmt.pr
    "%s %a %s %a @."
    (Progress.none ())
    (styled `Bold (styled `Yellow string))
    "RETRY"
    (test_name ~name ~width ~height)
    (styled `Bold (styled `Yellow string))
    (Printf.sprintf "(%i)" count)
;;

let success_message ~name ~width ~height =
  Fmt.pr
    "%s %a  %s @."
    (Progress.get_and_incr ())
    (styled `Bold (styled `Green string))
    "PASS"
    (test_name ~name ~width ~height)
;;

let layout_message ~print_head ~name ~width ~height =
  if print_head
  then
    Fmt.pr
      "%s %a  %s %a @."
      (Progress.get_and_incr ())
      (styled `Bold (styled `Red string))
      "FAIL"
      (test_name ~name ~width ~height)
      (styled `Red string)
      "Images have different layout."
  else
    Fmt.pr
      "%s %a @."
      (test_name ~name ~width ~height)
      (styled `Red string)
      "Images have different layout."
;;

let diff_message ~print_head ~name ~width ~height ~diffCount ~diffPercentage =
  if print_head
  then
    Fmt.pr
      "%s %a  %s %a @."
      (Progress.get_and_incr ())
      (styled `Bold (styled `Red string))
      "FAIL"
      (test_name ~name ~width ~height)
      (styled `Red string)
      (Printf.sprintf "Different pixels: %i (%f%%)" diffCount diffPercentage)
  else
    Fmt.pr
      "%s %a @."
      (test_name ~name ~width ~height)
      (styled `Red string)
      (Printf.sprintf "Different pixels: %i (%f%%)" diffCount diffPercentage)
;;

let corrupted_message ~print_head ~name ~width ~height =
  if print_head
  then
    Fmt.pr
      "%s %a  %s %a @."
      (Progress.get_and_incr ())
      (styled `Bold (styled `Red string))
      "FAIL"
      (test_name ~name ~width ~height)
      (styled `Red string)
      "The base image for this test is corrupted. Please regenerate the snapshot by \
       deleting the current base image!"
  else
    Fmt.pr
      "%s %a @."
      (test_name ~name ~width ~height)
      (styled `Red string)
      "The base image for this test is corrupted. Please regenerate the snapshot by \
       deleting the current base image!"
;;

let get_time_from_seconds seconds =
  let ( % ) = mod_float in
  let hours =
    let t = Int.of_float (seconds /. 3600.) in
    if t > 0
    then
      Fmt.str_like
        Fmt.stdout
        (match t = 1 with
         | true -> "%a hour, "
         | false -> "%a hours, ")
        (styled `Bold int)
        t
    else ""
  in
  let minutes =
    let t = Int.of_float (seconds % 3600. /. 60.) in
    if t > 0 || hours <> ""
    then
      Fmt.str_like
        Fmt.stdout
        (match t = 1 with
         | true -> "%a minute and "
         | false -> "%a minutes and ")
        (styled `Bold int)
        t
    else ""
  in
  let seconds =
    let t = seconds % 3600. % 60. in
    Fmt.str_like
      Fmt.stdout
      "%a seconds"
      (styled
         `Bold
         (float_dfrac
            (match minutes = "" with
             | true -> 3
             | false -> 0)))
      t
  in
  Printf.sprintf "%s%s%s" hours minutes seconds
;;

let stats ~seconds results =
  let created, skipped, passed, failed =
    results
    |> List.fold_left
         (fun acc test ->
            let created, skipped, passed, failed = acc in
            match test with
            | OSnap_Test_Types.{ result = Some `Created; _ } ->
              test :: created, skipped, passed, failed
            | OSnap_Test_Types.{ result = Some `Skipped; _ } ->
              created, test :: skipped, passed, failed
            | OSnap_Test_Types.{ result = Some `Passed; _ } ->
              created, skipped, test :: passed, failed
            | OSnap_Test_Types.{ result = Some (`Failed _); _ } ->
              created, skipped, passed, test :: failed
            | _ -> acc)
         ([], [], [], [])
  in
  let create_count = List.length created in
  let passed_count = List.length passed in
  let failed_count = List.length failed in
  let test_count = List.length results in
  let skipped_count = List.length skipped in
  Fmt.pr
    "\n\nDone! 🚀\nI did run a total of %a snapshots in %s@."
    (styled `Bold int)
    test_count
    (get_time_from_seconds seconds);
  Fmt.pr "Results:@.";
  if create_count > 0
  then
    Fmt.pr
      "%a %a @."
      (styled `Bold int)
      create_count
      (styled `Bold string)
      "Snapshots created";
  if skipped_count > 0
  then
    Fmt.pr
      "%a %a @."
      (styled `Bold (styled `Yellow int))
      skipped_count
      (styled `Bold (styled `Yellow string))
      "Snapshots skipped";
  Fmt.pr
    "%a %a @."
    (styled `Bold (styled `Green int))
    passed_count
    (styled `Bold (styled `Green string))
    "Snapshots passed";
  if failed_count > 0
  then (
    Fmt.pr
      "%a %a @."
      (styled `Bold (styled `Red int))
      failed_count
      (styled `Bold (styled `Red string))
      "Snapshots failed";
    Fmt.pr "\n%a\n@." (styled `Bold string) "Summary of failed tests:";
    failed
    |> List.iter (function
      | { name; width; height; result = Some (`Failed `Io); _ } ->
        corrupted_message ~print_head:false ~name ~width ~height
      | { name; width; height; result = Some (`Failed `Layout); _ } ->
        layout_message ~print_head:false ~name ~width ~height
      | { name
        ; width
        ; height
        ; result = Some (`Failed (`Pixel (diffCount, diffPercentage)))
        ; _
        } ->
        diff_message ~print_head:false ~name ~width ~height ~diffCount ~diffPercentage
      | _ -> ()));
  match failed with
  | [] -> Result.ok ()
  | _ -> Result.error `OSnap_Test_Failure
;;
