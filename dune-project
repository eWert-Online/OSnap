(lang dune 3.15)

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
  eio_main
  base64
  cdp
  cmdliner
  httpun-eio
  httpun-ws-eio
  decompress
  fileutils
  fmt
  libspng
  odiff-core
  re
  uri
  yaml
  yojson))
