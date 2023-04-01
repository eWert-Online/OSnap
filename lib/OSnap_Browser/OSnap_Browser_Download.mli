val download
  :  OSnap_Browser_Path.revision
  -> (unit, [> `OSnap_Chromium_Download_Failed ]) Lwt_result.t

val is_revision_downloaded : OSnap_Browser_Path.revision -> bool
