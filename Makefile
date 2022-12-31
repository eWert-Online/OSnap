.PHONY: all build install update fmt clean clear
.SILENT: all build install update fmt clean clear

all: build

build:
	dune build -p chromium-version --profile=release
	dune build -p osnap --profile=release

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
	dune build @fmt --auto-promote

clean:
	rm -rf _build

clear: clean
	rm -rf _opam
