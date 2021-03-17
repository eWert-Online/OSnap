type failState =
  | Pixel(int)
  | Layout;

let diff:
  (
    ~output: string,
    ~diffPixel: (int, int, int)=?,
    ~threshold: float=?,
    string,
    string
  ) =>
  result(unit, failState);
