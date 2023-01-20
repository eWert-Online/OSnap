module Io = OSnap_Diff_Io
module Diff = Odiff.Diff.MakeDiff (Io.PNG) (Io.PNG)
open Bigarray

let ( let* ) = Result.bind

type failState =
  | Io
  | Pixel of int * float
  | Layout

let diff
  ~output
  ?(diffPixel = 255, 0, 0)
  ?(ignoreRegions = [])
  ?(threshold = 0)
  ~original_image_data
  ~new_image_data
  ()
  =
  let* original_image =
    try Io.PNG.loadImage original_image_data |> Result.ok with
    | _ -> Result.error Io
  in
  let* new_image =
    try Io.PNG.loadImage new_image_data |> Result.ok with
    | _ -> Result.error Io
  in
  let free_images () =
    Io.PNG.freeImage original_image;
    Io.PNG.freeImage new_image
  in
  let diff_pixel_color =
    let r, g, b = diffPixel in
    let a = (255 land 0xFF) lsl 24 in
    let b = (b land 0xFF) lsl 16 in
    let g = (g land 0xFF) lsl 8 in
    let r = (r land 0xFF) lsl 0 in
    Int32.of_int (a lor b lor g lor r)
  in
  Diff.diff
    original_image
    new_image
    ~outputDiffMask:true
    ~threshold:0.1
    ~failOnLayoutChange:false
    ~antialiasing:true
    ~ignoreRegions
    ~diffPixel
    ()
  |> function
  | Pixel (_, diffCount, _) when diffCount <= threshold ->
    free_images ();
    Result.ok ()
  | Layout ->
    free_images ();
    Result.error Layout
  | Pixel (diff_mask, diffCount, diffPercentage) ->
    let original_image = original_image in
    let diff_mask = diff_mask in
    let new_image = new_image in
    let border_width = 5 in
    let complete_width =
      original_image.width
      + border_width
      + diff_mask.width
      + border_width
      + new_image.width
    in
    let complete_height =
      original_image.height |> max diff_mask.height |> max new_image.height
    in
    let complete_image =
      Array1.create int32 c_layout (complete_width * complete_height * 4)
    in
    let original_image_start = 0 in
    let original_image_end = original_image.width in
    let diff_mask_start = original_image_end + border_width in
    let diff_mask_end = diff_mask_start + diff_mask.width in
    let new_image_start = diff_mask_end + border_width in
    let new_image_end = new_image_start + new_image.width in
    let row = ref (-1) in
    for offset = 0 to Array1.dim complete_image / 4 do
      let col = offset mod complete_width in
      if col = 0 then incr row;
      let fill_with =
        if col >= original_image_start
           && col < original_image_end
           && !row < original_image.height
        then Array1.unsafe_get original_image.image ((!row * original_image.width) + col)
        else if col > diff_mask_start && col < diff_mask_end
        then (
          let ( >> ) = Int32.shift_right in
          let ( & ) = Int32.logand in
          let diff_pixel =
            if !row < diff_mask.height
            then
              Array1.unsafe_get
                diff_mask.image
                ((!row * diff_mask.width) + (col - diff_mask_start))
            else diff_pixel_color
          in
          if (diff_pixel >> 24 & 0xFFl) = 0xFFl
          then diff_pixel
          else (
            let pixel =
              Array1.unsafe_get
                original_image.image
                ((!row * original_image.width) + (col - diff_mask_start))
              |> Int32.to_int
            in
            let a = (pixel lsr 24) land 0xFF in
            let b = (pixel lsr 16) land 0xFF in
            let g = (pixel lsr 8) land 0xFF in
            let r = (pixel lsr 0) land 0xFF in
            let brightness = ((r * 54) + (g * 182) + (b * 19)) / 255 in
            let mono = min 255 (brightness + 80) in
            let a = (a land 0xFF) lsl 24 in
            let b = (mono land 0xFF) lsl 16 in
            let g = (mono land 0xFF) lsl 8 in
            let r = (mono land 0xFF) lsl 0 in
            Int32.of_int (a lor b lor g lor r)))
        else if col > new_image_start && col <= new_image_end
        then
          Array1.unsafe_get
            new_image.image
            ((!row * new_image.width) + (col - new_image_start))
        else 0x00000000l
      in
      Array1.unsafe_set complete_image offset fill_with
    done;
    free_images ();
    WritePng.write_png_bigarray output complete_image complete_width complete_height;
    Result.error (Pixel (diffCount, diffPercentage))
;;
