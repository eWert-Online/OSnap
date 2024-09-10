.PHONY: all build install update fmt clean clear
.SILENT: all build install update fmt clean clear

all: build

build:
	opam exec -- dune build -p osnap --profile=release

install:
	if ! [ -e _opam ]; then \
		opam switch create . --empty ; \
	fi
	opam install . --deps-only --working-dir --with-test --with-dev-setup --yes

update:
	opam update
	opam upgrade

fmt:
	opam exec -- dune build @fmt --auto-promote

clean:
	rm -rf _build

clear: clean
	rm -rf _opam
