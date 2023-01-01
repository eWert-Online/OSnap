type failState =
  | Io
  | Pixel of int * float
  | Layout

val diff
  :  output:string
  -> ?diffPixel:int * int * int
  -> ?ignoreRegions:((int * int) * (int * int)) list
  -> ?threshold:int
  -> original_image_data:string
  -> new_image_data:string
  -> unit
  -> (unit, failState) result
