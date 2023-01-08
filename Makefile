.PHONY: all build install update fmt clean clear chromium-version
.SILENT: all build install update fmt clean clear chromium-version

all: build

build:
	opam exec -- dune build -p osnap --profile=release

install:
	if ! [ -e _opam ]; then \
		opam switch create . --empty ; \
	fi
	opam install ./*.opam --locked --deps-only --with-test --yes
	opam lock .

update:
	opam update
	opam upgrade
	opam lock .

fmt:
	opam exec -- dune build @fmt --auto-promote

clean:
	rm -rf _build

clear: clean
	rm -rf _opam

chromium-version:
	opam exec -- dune exec chromium-version
