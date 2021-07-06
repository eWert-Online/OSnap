open Lwt.Syntax;

open Httpaf;
open Httpaf_lwt_unix;

Printexc.record_backtrace(true);

let get_download_url = revision => {
  let base_path = "http://storage.googleapis.com/chromium-browser-snapshots";
  switch (OSnap.Utils.detect_platform()) {
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

  let (finished, notify_finished) = Lwt.wait();
  let socket = Lwt_unix.socket(Unix.PF_INET, Unix.SOCK_STREAM, 0);

  let uri = revision |> get_download_url |> Uri.of_string;
  let host = uri |> Uri.host_with_default(~default="storage.googleapis.com");
  let port = uri |> Uri.port |> Option.value(~default=80);
  let* addresses =
    Lwt_unix.getaddrinfo(
      host,
      Int.to_string(port),
      [Unix.(AI_FAMILY(PF_INET))],
    );

  let error_handler = error => {
    switch (error) {
    | `Exn(exn) =>
      print_endline("Chrome could not be downloaded");
      Printexc.to_string(exn) |> print_endline;
      exit(1);
    | `Invalid_response_body_length(_response) =>
      print_endline("Chrome could not be downloaded");
      exit(1);
    | `Malformed_response(string) =>
      print_endline(
        "Chrome could not be downloaded. Malformed Response: \n" ++ string,
      );
      exit(1);
    };
  };

  let response_handler = (response, response_body) => {
    let on_eof = Lwt.wakeup_later(notify_finished);

    let rec on_read = (bs, ~off, ~len) => {
      Lwt.async(() => {
        Bigstringaf.substring(~off, ~len, bs)
        |> Lwt_io.write(io)
        |> Lwt.map(() => Body.schedule_read(response_body, ~on_read, ~on_eof))
      });
    };

    switch (response) {
    | {Response.status: `OK, _} =>
      Body.schedule_read(response_body, ~on_read, ~on_eof)
    | response =>
      print_endline("Chrome could not be downloaded:");
      Format.fprintf(
        Format.err_formatter,
        "%a\n%!",
        Response.pp_hum,
        response,
      );
      exit(1);
    };
  };

  let* () = Lwt_unix.connect(socket, List.hd(addresses).Unix.ai_addr);

  let request_body =
    uri
    |> Uri.to_string
    |> Request.create(`GET, ~headers=Headers.of_list([("Host", host)]))
    |> Client.request(socket, ~error_handler, ~response_handler);

  Body.close_writer(request_body);

  let* () = finished;
  let* () = Lwt_io.close(io);

  zip_path |> Lwt.return;
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

let main = () => {
  let revision = OSnap_Browser.Path.get_revision();
  let extract_path = OSnap_Browser.Path.get_chromium_path();

  print_newline();
  print_newline();

  if (!Sys.file_exists(extract_path) || !Sys.is_directory(extract_path)) {
    Unix.mkdir(extract_path, 511);
    Lwt_io.with_temp_dir(~prefix="osnap_chromium_", dir => {
      let* path = dir |> download(~revision);
      extract_zip(path, ~dest=extract_path);
      print_endline("Done!");
      Lwt.return();
    });
  } else {
    print_endline(
      Printf.sprintf(
        "Found Chromium at \"%s\". Skipping Download!",
        extract_path,
      ),
    );
    Lwt.return();
  };
};

Lwt_main.run(main());
