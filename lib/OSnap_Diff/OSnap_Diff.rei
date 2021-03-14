type failState =
  | Pixel(int)
  | Layout;

let diff: (~output: string, string, string) => result(unit, failState);
