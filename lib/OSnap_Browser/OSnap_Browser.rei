type t;

module Launcher: {
  let make: unit => Lwt.t(t);

  let shutdown: t => unit;
};

module Actions: {
  let wait_for: string => Lwt.t(unit);

  let go_to: (string, t) => Lwt.t(string);

  let set_size: (~width: int, ~height: int, t) => Lwt.t(unit);

  let screenshot: (~full_size: bool=?, t) => Lwt.t(string);
};
