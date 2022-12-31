(lang dune 3.1)

(name osnap)

; (implicit_transitive_deps false)

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
  reason
  dune
  ocaml-lsp-server
  ocamlformat
  base64
  cdp
  cmdliner
  cohttp
  cohttp-lwt-unix
  decompress
  fileutils
  fmt
  libspng
  logs
  lwt
  odiff-core
  re
  uri
  websocket
  websocket-lwt-unix
  yaml
  yojson))

(package
 (name chromium-version)
 (synopsis
  "A resolver for the current available chromium version across all platforms")
 (depends
  ocaml
  reason
  dune
  ocaml-lsp-server
  ocamlformat
  cmdliner
  fmt
  cohttp
  cohttp-lwt-unix
  fileutils
  lwt))
