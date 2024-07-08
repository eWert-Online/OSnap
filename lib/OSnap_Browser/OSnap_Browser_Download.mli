val download
  :  env:Eio_unix.Stdenv.base
  -> OSnap_Browser_Path.revision
  -> (unit, [> `OSnap_Chromium_Download_Failed ]) Result.t

val is_revision_downloaded : OSnap_Browser_Path.revision -> bool
