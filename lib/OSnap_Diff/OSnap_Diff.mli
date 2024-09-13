type failState =
  | Io
  | Pixel of int * float
  | Layout

val diff
  :  output:string
  -> ?ignoreRegions:((int * int) * (int * int)) list
  -> ?threshold:int
  -> diffPixel:int32
  -> original_image_data:string
  -> new_image_data:string
  -> unit
  -> (unit, failState) result
