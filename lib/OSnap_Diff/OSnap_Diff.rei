type failState =
  | Pixel(int)
  | Layout;

let diff:
  (
    ~output: string,
    ~diffPixel: (int, int, int)=?,
    ~threshold: int=?,
    string,
    string
  ) =>
  result(unit, failState);
