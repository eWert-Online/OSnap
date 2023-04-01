type revision

val revision_to_string : revision -> string
val get_latest_revision : unit -> revision
val get_previous_revisions : unit -> revision list
val get_chromium_path : revision -> string
