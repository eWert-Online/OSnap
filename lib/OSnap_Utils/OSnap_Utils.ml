type platform =
  | Win32
  | Win64
  | MacOS
  | MacOS_ARM
  | Linux

let detect_platform () =
  let win = Sys.win32 || Sys.cygwin in
  match win, Sys.word_size with
  | true, 64 -> Win64
  | true, _ -> Win32
  | false, _ ->
    let ic = Unix.open_process_in "uname" in
    let uname = input_line ic in
    let () = close_in ic in
    let ic = Unix.open_process_in "uname -m" in
    let arch = input_line ic in
    let () = close_in ic in
    if uname = "Darwin"
    then (
      match arch with
      | "arm64" -> MacOS_ARM
      | _ -> MacOS)
    else Linux
;;

let get_file_contents filename = In_channel.with_open_bin filename In_channel.input_all

let contains_substring ~search str =
  let search_length = String.length search in
  let len = String.length str in
  try
    for i = 0 to len - search_length do
      let j = ref 0 in
      while str.[i + !j] = search.[!j] do
        incr j;
        if !j = search_length then raise_notrace Exit
      done
    done;
    false
  with
  | Exit -> true
;;

let find_duplicates get_key list =
  let hash = Hashtbl.create (List.length list) in
  list
  |> List.filter (fun item ->
       let key = get_key item in
       if Hashtbl.mem hash key
       then true
       else (
         Hashtbl.add hash key true;
         false))
;;

let path_of_segments paths =
  paths
  |> List.rev
  |> List.fold_left
       (fun acc curr ->
         match acc with
         | "" -> curr
         | path -> path ^ "/" ^ curr)
       ""
;;

module List = struct
  include List

  let map_until_exception fn list =
    let rec loop acc list =
      match list with
      | [] -> Result.ok (List.rev acc)
      | hd :: tl ->
        let result = fn hd in
        (match result with
         | Ok v -> loop (v :: acc) tl
         | Error e -> Result.error e)
    in
    loop [] list
  ;;
end

module Lwt_list = struct
  include Lwt_list

  let map_p_until_exception fn list =
    let open! Lwt.Syntax in
    let rec loop acc list =
      match list with
      | [] -> Lwt_result.return acc
      | list ->
        let* resolved, pending = Lwt.nchoose_split list in
        let success, error =
          resolved
          |> List.partition_map (function
               | Ok v -> Either.left v
               | Error e -> Either.right e)
        in
        (match error with
         | [] -> loop (success @ acc) pending
         | hd :: _tl ->
           pending |> List.iter Lwt.cancel;
           Lwt_result.fail hd)
    in
    let promises = list |> List.map (Lwt.apply fn) in
    loop [] promises
  ;;
end
