type failState =
  | Pixel(int, float)
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
