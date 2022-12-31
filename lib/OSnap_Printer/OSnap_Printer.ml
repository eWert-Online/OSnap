open Fmt

let test_name ~name ~width ~height =
  Fmt.str_like
    Fmt.stdout
    "%s %a"
    name
    (styled `Faint string)
    (Printf.sprintf "(%ix%i)" width height)
;;

let created_message ~name ~width ~height =
  Fmt.pr
    "%a\t%s @."
    (styled `Bold (styled `Blue string))
    "CREATE"
    (test_name ~name ~width ~height)
;;

let skipped_message ~name ~width ~height =
  Fmt.pr
    "%a\t%s @."
    (styled `Bold (styled `Yellow string))
    "SKIP"
    (test_name ~name ~width ~height)
;;

let success_message ~name ~width ~height =
  Fmt.pr
    "%a\t%s @."
    (styled `Bold (styled `Green string))
    "PASS"
    (test_name ~name ~width ~height)
;;

let layout_message ~print_head ~name ~width ~height =
  if print_head
  then
    Fmt.pr
      "%a\t%s %a @."
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
      "%a\t%s %a @."
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
      "%a\t%s %a @."
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

let stats ~test_count ~create_count ~passed_count ~failed_tests ~skipped_count ~seconds =
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
  Fmt.pr
    "\n\nDone! 🚀\nI did run a total of %a snapshots in %s%s%s@."
    (styled `Bold int)
    test_count
    hours
    minutes
    seconds;
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
  let failed_count = List.length failed_tests in
  if failed_count > 0
  then (
    Fmt.pr
      "%a %a @."
      (styled `Bold (styled `Red int))
      failed_count
      (styled `Bold (styled `Red string))
      "Snapshots failed";
    Fmt.pr "\n%a\n@." (styled `Bold string) "Summary of failed tests:";
    failed_tests
    |> List.iter (function
         | `Failed (`Io (name, width, height)) ->
           corrupted_message ~print_head:false ~name ~width ~height
         | `Failed (`Layout (name, width, height)) ->
           layout_message ~print_head:false ~name ~width ~height
         | `Failed (`Pixel (name, width, height, diffCount, diffPercentage)) ->
           diff_message ~print_head:false ~name ~width ~height ~diffCount ~diffPercentage))
;;
