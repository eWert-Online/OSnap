module Config = OSnap_Config;
module Browser = OSnap_Browser;

module Diff = OSnap_Diff;
module Printer = OSnap_Printer;

type t = {
  url: string,
  name: string,
  size_name: option(string),
  width: int,
  height: int,
  actions: list(Config.Types.action),
  ignore_regions: list(Config.Types.ignoreType),
  threshold: int,
  exists: bool,
};

let save_screenshot = (~path, data) => {
  open Lwt_result.Syntax;

  let* io =
    try(Lwt_io.open_file(~mode=Output, path) |> Lwt_result.ok) {
    | _ =>
      OSnap_Response.FS_Error(
        Printf.sprintf("Could not save screenshot to %s", path),
      )
      |> Lwt_result.fail
    };

  let* () = Lwt_io.write(io, data) |> Lwt_result.ok;
  Lwt_io.close(io) |> Lwt_result.ok;
};

let read_file_contents = (~path) => {
  open Lwt_result.Syntax;

  let* io =
    try(Lwt_io.open_file(~mode=Input, path) |> Lwt_result.ok) {
    | _ =>
      OSnap_Response.FS_Error(
        Printf.sprintf("Could not open file %S for reading", path),
      )
      |> Lwt_result.fail
    };

  let* data = Lwt_io.read(io) |> Lwt_result.ok;
  let* () = Lwt_io.close(io) |> Lwt_result.ok;

  Lwt_result.return(data);
};

let rec execute_action =
        (~document, ~global_config, target, size_name, action) => {
  Config.Types.(
    switch (action, size_name) {
    | (Scroll(_, Some(_)), None) => Lwt_result.return()
    | (Scroll(`Selector(selector), None), _) =>
      target
      |> Browser.Actions.scroll(
           ~document,
           ~selector=Some(selector),
           ~px=None,
         )
    | (Scroll(`PxAmount(px), None), _) =>
      target
      |> Browser.Actions.scroll(~document, ~selector=None, ~px=Some(px))
    | (Scroll(`Selector(selector), Some(size_restr)), Some(size_name)) =>
      if (size_restr |> List.mem(size_name)) {
        target
        |> Browser.Actions.scroll(
             ~document,
             ~selector=Some(selector),
             ~px=None,
           );
      } else {
        Lwt_result.return();
      }
    | (Scroll(`PxAmount(px), Some(size_restr)), Some(size_name)) =>
      if (size_restr |> List.mem(size_name)) {
        target
        |> Browser.Actions.scroll(~document, ~selector=None, ~px=Some(px));
      } else {
        Lwt_result.return();
      }

    | (Click(_, Some(_)), None) => Lwt_result.return()
    | (Click(selector, None), _) =>
      target |> Browser.Actions.click(~document, ~selector)
    | (Click(selector, Some(size_restr)), Some(size_name)) =>
      if (size_restr |> List.mem(size_name)) {
        target |> Browser.Actions.click(~document, ~selector);
      } else {
        Lwt_result.return();
      }

    | (Type(_, _, Some(_)), None) => Lwt_result.return()
    | (Type(selector, text, None), _) =>
      target |> Browser.Actions.type_text(~document, ~selector, ~text)
    | (Type(selector, text, Some(size)), Some(size_name)) =>
      if (size |> List.mem(size_name)) {
        target |> Browser.Actions.type_text(~document, ~selector, ~text);
      } else {
        Lwt_result.return();
      }

    | (Wait(_, Some(_)), None) => Lwt_result.return()
    | (Wait(ms, Some(size)), Some(size_name)) =>
      if (size |> List.mem(size_name)) {
        let timeout = float_of_int(ms) /. 1000.0;
        Lwt_unix.sleep(timeout) |> Lwt_result.ok;
      } else {
        Lwt_result.return();
      }
    | (Wait(ms, None), _) =>
      let timeout = float_of_int(ms) /. 1000.0;
      Lwt_unix.sleep(timeout) |> Lwt_result.ok;

    | (Function(_, Some(_)), None) => Lwt_result.return()
    | (Function(name, Some(size)), Some(size_name)) =>
      if (size |> List.mem(size_name)) {
        switch (global_config.functions |> List.assoc_opt(name)) {
        | Some(actions) =>
          Lwt.Infix.(
            actions
            |> Lwt_list.map_s(
                 execute_action(
                   ~document,
                   ~global_config,
                   target,
                   Some(size_name),
                 ),
               )
            >>= Lwt_list.fold_left_s(
                  (acc: Result.t(unit, OSnap_Response.t), curr) =>
                    if (Result.is_ok(acc) && Result.is_ok(curr)) {
                      Lwt.return(acc);
                    } else {
                      Lwt.return(curr);
                    },
                  Result.ok(),
                )
          )
        | None =>
          Lwt_result.fail(
            OSnap_Response.Invalid_Run(
              "Tried to call non existant function " ++ name,
            ),
          )
        };
      } else {
        Lwt_result.return();
      }
    | (Function(name, None), _) =>
      switch (global_config.functions |> List.assoc_opt(name)) {
      | Some(actions) =>
        Lwt.Infix.(
          actions
          |> Lwt_list.map_s(
               execute_action(~document, ~global_config, target, size_name),
             )
          >>= Lwt_list.fold_left_s(
                (acc: Result.t(unit, OSnap_Response.t), curr) =>
                  if (Result.is_ok(acc) && Result.is_ok(curr)) {
                    Lwt.return(acc);
                  } else {
                    Lwt.return(curr);
                  },
                Result.ok(),
              )
        )
      | None =>
        Lwt_result.fail(
          OSnap_Response.Invalid_Run(
            "Tried to call non existant function " ++ name,
          ),
        )
      }
    }
  );
};

let get_ignore_regions = (~document, target, size_name, regions) => {
  Lwt_result.Syntax.(
    Config.Types.(
      regions
      |> List.filter(region => {
           switch (region, size_name) {
           | (Coordinates(_a, _b, None), _) => true
           | (Coordinates(_, _, Some(_)), None) => false
           | (Coordinates(_a, _b, Some(size_restr)), Some(size_name)) =>
             List.mem(size_name, size_restr)
           | (Selector(_, Some(_)), None) => false
           | (Selector(_, Some(size_restr)), Some(size_name)) =>
             List.mem(size_name, size_restr)
           | (Selector(_selector, None), _) => true
           }
         })
      |> Lwt_list.map_p(region => {
           switch (region) {
           | Coordinates(a, b, _) => Lwt_result.return((a, b))
           | Selector(selector, _) =>
             let* ((x1, y1), (x2, y2)) =
               target |> Browser.Actions.get_quads(~document, ~selector);
             let x1 = Int.of_float(x1);
             let y1 = Int.of_float(y1);
             let x2 = Int.of_float(x2);
             let y2 = Int.of_float(y2);
             Lwt_result.return(((x1, y1), (x2, y2)));
           }
         })
    )
  );
};

let get_filename = (name, width, height) =>
  Printf.sprintf("/%s_%ix%i.png", name, width, height);

let run = (global_config: Config.Types.global, target, test) => {
  open Lwt_result.Syntax;
  open Lwt.Infix;

  let dirs = OSnap_Paths.get(global_config);

  let filename = get_filename(test.name, test.width, test.height);
  let url = global_config.base_url ++ test.url;
  let base_snapshot = dirs.base ++ filename;
  let updated_snapshot = dirs.updated ++ filename;
  let diff_image = dirs.diff ++ filename;

  let* () = target |> Browser.Actions.clear_cookies;

  let* () =
    target
    |> Browser.Actions.set_size(
         ~width=`Int(test.width),
         ~height=`Int(test.height),
       );

  let* loaderId = target |> Browser.Actions.go_to(~url);

  let* () =
    target
    |> Browser.Actions.wait_for_network_idle(~loaderId)
    |> Lwt_result.ok;

  let* document = target |> Browser.Actions.get_document;

  let* () =
    test.actions
    |> Lwt_list.map_s(
         execute_action(~document, ~global_config, target, test.size_name),
       )
    >>= Lwt_list.fold_left_s(
          (acc: Result.t(unit, OSnap_Response.t), curr) =>
            if (Result.is_ok(acc) && Result.is_ok(curr)) {
              Lwt.return(acc);
            } else {
              Lwt.return(curr);
            },
          Result.ok(),
        );

  let* screenshot =
    target
    |> Browser.Actions.screenshot(~full_size=global_config.fullscreen)
    |> Lwt_result.map(Base64.decode_exn);

  if (!test.exists) {
    Printer.created_message(
      ~name=test.name,
      ~width=test.width,
      ~height=test.height,
    );
    let* () = save_screenshot(~path=base_snapshot, screenshot);
    Lwt_result.return(`Created);
  } else {
    let* original_image_data = read_file_contents(~path=base_snapshot);

    if (original_image_data == screenshot) {
      Printer.success_message(
        ~name=test.name,
        ~width=test.width,
        ~height=test.height,
      );
      Lwt_result.return(`Passed);
    } else {
      let* ignoreRegions =
        test.ignore_regions
        |> get_ignore_regions(~document, target, test.size_name)
        >>= Lwt_list.fold_left_s(
              (acc, curr) => {
                switch (acc, curr) {
                | (Ok(acc), Ok(curr)) =>
                  Lwt.return(Result.ok([curr, ...acc]))
                | (Error(acc), Ok(_curr)) => Lwt.return(Result.error(acc))
                | (Ok(_acc), Error(curr)) =>
                  Lwt.return(Result.error(curr))
                | (Error(acc), Error(_)) => Lwt.return(Result.error(acc))
                }
              },
              Result.ok([]),
            );

      let diff =
        Diff.diff(
          ~threshold=test.threshold,
          ~diffPixel=global_config.diff_pixel_color,
          ~ignoreRegions,
          ~output=diff_image,
          ~original_image_data,
          ~new_image_data=screenshot,
        );

      switch (diff()) {
      | Ok () =>
        Printer.success_message(
          ~name=test.name,
          ~width=test.width,
          ~height=test.height,
        );
        Lwt_result.return(`Passed);
      | Error(Layout) =>
        Printer.layout_message(
          ~name=test.name,
          ~width=test.width,
          ~height=test.height,
        );
        let* () = save_screenshot(screenshot, ~path=updated_snapshot);
        Lwt_result.return(`Failed);
      | Error(Pixel(diffCount, diffPercentage)) =>
        Printer.diff_message(
          ~name=test.name,
          ~width=test.width,
          ~height=test.height,
          ~diffCount,
          ~diffPercentage,
        );
        let* () = save_screenshot(screenshot, ~path=updated_snapshot);
        Lwt_result.return(`Failed);
      };
    };
  };
};
