let listen:
  (~event: string, ~sessionId: string, (string, unit => unit) => unit) => unit;

let close: unit => Lwt.t(unit);

let send: string => Lwt.t(string);

let connect: string => Lwt.t(unit);
