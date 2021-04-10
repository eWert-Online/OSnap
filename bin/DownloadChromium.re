open Lwt.Syntax;
open Cohttp_lwt_unix;

type platform =
  | Win32
  | Win64
  | Darwin
  | Linux;

let detect_platform = () => {
  let win = Sys.win32 || Sys.cygwin;
  let ic = Unix.open_process_in("uname");
  let uname = input_line(ic);
  let () = close_in(ic);

  switch (win, Sys.word_size, uname) {
  | (true, 64, _) => Win64
  | (true, _, _) => Win32
  | (false, _, "Darwin") => Darwin
  | _ => Linux
  };
};

let get_download_url = revision => {
  let base_path = "https://storage.googleapis.com/chromium-browser-snapshots";
  switch (detect_platform()) {
  | Darwin => base_path ++ "/Mac/" ++ revision ++ "/chrome-mac.zip"
  | Linux => base_path ++ "/Linux_x64/" ++ revision ++ "/chrome-linux.zip"
  | Win64 => base_path ++ "/Win_x64/" ++ revision ++ "/chrome-win.zip"
  | Win32 =>
    print_endline("Error: x86 is currently not supported on Windows");
    exit(1);
  };
};

let download = (~revision, dir) => {
  let zip_path = Filename.concat(dir, "chromium.zip");
  let* io = Lwt_io.open_file(~mode=Output, zip_path);

  print_endline(
    Printf.sprintf(
      "Downloading Chrome Revision %s. This could take a while...",
      revision,
    ),
  );

  let uri = Uri.of_string(get_download_url(revision));
  let* (response, body) = Client.get(uri);

  let status = response |> Response.status;
  let* () =
    switch (status) {
    | `OK =>
      let* () =
        body
        |> Cohttp_lwt.Body.to_stream
        |> Lwt_stream.iter_s(Lwt_io.write(io));
      Lwt_io.close(io);
    | _ =>
      print_endline("Chrome could not be downloaded");
      exit(1);
    };

  zip_path |> Lwt.return;
};

let extract_zip = source => {
  let extract_entry = (ifile, entry) => {
    let file = entry.Camlzip.Zip.filename;
    if (entry.Camlzip.Zip.is_directory && !Sys.file_exists(file)) {
      Unix.mkdir(file, 511);
    } else {
      Camlzip.Zip.copy_entry_to_file(ifile, entry, file);
    };
  };

  print_endline("Extracting Chromium...");

  let ic = Camlzip.Zip.open_in(source);
  ic |> Camlzip.Zip.entries |> List.iter(extract_entry(ic));
  Camlzip.Zip.close_in(ic);
};

let main = () => {
  let revision = "856583";
  print_newline();
  print_newline();
  Lwt_io.with_temp_dir(~prefix="osnap_chromium_", dir => {
    let* path = dir |> download(~revision);
    extract_zip(path);
    print_endline("Done!");
    Lwt.return();
  });
};

Lwt_main.run(main());
