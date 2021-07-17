open Cohttp_lwt_unix;

let get_download_url = revision => {
  let base_path = "http://storage.googleapis.com/chromium-browser-snapshots";
  switch (OSnap_Utils.detect_platform()) {
  | Darwin => base_path ++ "/Mac/" ++ revision ++ "/chrome-mac.zip"
  | Linux => base_path ++ "/Linux_x64/" ++ revision ++ "/chrome-linux.zip"
  | Win64 => base_path ++ "/Win_x64/" ++ revision ++ "/chrome-win.zip"
  | Win32 =>
    print_endline("Error: x86 is currently not supported on Windows");
    exit(1);
  };
};

let download = (~revision, dir) => {
  open Lwt.Syntax;

  let zip_path = Filename.concat(dir, "chromium.zip");
  let* io = Lwt_io.open_file(~mode=Output, zip_path);

  print_endline(
    Printf.sprintf(
      "Downloading Chrome Revision %s. This could take a while...",
      revision,
    ),
  );

  let uri = revision |> get_download_url |> Uri.of_string;

  let* (response, body) = Client.get(uri);

  switch (response) {
  | {status: `OK, _} =>
    let* () =
      Cohttp_lwt.Body.to_stream(body) |> Lwt_stream.iter_s(Lwt_io.write(io));
    let* () = Lwt_io.close(io);
    zip_path |> Lwt_result.return;
  | response =>
    let* () = Lwt_io.close(io);
    Format.fprintf(
      Format.err_formatter,
      "Chrome could not be downloaded:\n%a\n%!",
      Response.pp_hum,
      response,
    );
    Lwt_result.fail();
  };
};

let extract_zip = (~dest="", source) => {
  let extract_entry = (in_file, entry: Camlzip.Zip.entry) => {
    let out_file = Filename.concat(dest, entry.filename);
    if (entry.is_directory && !Sys.file_exists(out_file)) {
      Unix.mkdir(out_file, 511);
    } else {
      let oc =
        open_out_gen([Open_creat, Open_binary, Open_append], 511, out_file);
      try(
        {
          Camlzip.Zip.copy_entry_to_channel(in_file, entry, oc);
          close_out(oc);
          try(Unix.utimes(out_file, entry.mtime, entry.mtime)) {
          | Unix.Unix_error(_, _, _)
          | Invalid_argument(_) => ()
          };
        }
      ) {
      | err =>
        close_out(oc);
        Sys.remove(out_file);
        raise(err);
      };
    };
  };

  print_endline("Extracting Chromium...");

  let ic = Camlzip.Zip.open_in(source);
  ic |> Camlzip.Zip.entries |> List.iter(extract_entry(ic));
  Camlzip.Zip.close_in(ic);
};

let download = () => {
  open Lwt_result.Syntax;

  let revision = OSnap_Browser_Path.get_revision();
  let extract_path = OSnap_Browser_Path.get_chromium_path();

  print_newline();
  print_newline();

  if (!Sys.file_exists(extract_path) || !Sys.is_directory(extract_path)) {
    Unix.mkdir(extract_path, 511);
    Lwt_io.with_temp_dir(~prefix="osnap_chromium_", dir => {
      let* path = dir |> download(~revision);
      extract_zip(path, ~dest=extract_path);
      print_endline("Done!");
      Lwt_result.return();
    });
  } else {
    print_endline(
      Printf.sprintf(
        "Found Chromium at \"%s\". Skipping Download!",
        extract_path,
      ),
    );
    Lwt_result.return();
  };
};
