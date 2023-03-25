(lang dune 3.1)

(name osnap)

(implicit_transitive_deps false)

(generate_opam_files true)

(source
 (github eWert-Online/OSnap))

(license MIT)

(authors "Torben Ewert")

(maintainers "torben@ewert-online.com")

(package
 (name osnap)
 (synopsis
  "The speedy and easy to use snapshot testing tool for your project!")
 (depends
  ocaml
  dune
  base64
  cdp
  cmdliner
  cohttp
  cohttp-lwt-unix
  decompress
  eio_main
  fileutils
  fmt
  libspng
  lwt
  lwt_eio
  odiff-core
  re
  uri
  websocket
  websocket-lwt-unix
  yaml
  yojson))
