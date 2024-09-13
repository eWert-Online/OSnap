(lang dune 3.15)

(name osnap)

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
  eio_main
  lwt_eio
  httpun-eio
  base64
  cdp
  cmdliner
  decompress
  fileutils
  fmt
  libspng
  odiff-core
  re
  uri
  websocket
  websocket-lwt-unix
  yaml
  yojson
  (ocaml-lsp-server :with-dev-setup)
  (ocamlformat :with-dev-setup)))
