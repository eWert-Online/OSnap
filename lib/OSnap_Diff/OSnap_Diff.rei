type failState =
  | Pixel(int, float)
  | Layout;

let diff:
  (
    ~output: string,
    ~diffPixel: (int, int, int)=?,
    ~ignoreRegions: list(((int, int), (int, int)))=?,
    ~threshold: int=?,
    ~original_image_data: string,
    ~new_image_data: string,
    unit
  ) =>
  result(unit, failState);
