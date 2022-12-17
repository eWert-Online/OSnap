FROM ocaml/opam:alpine-ocaml-4.14-flambda as build

RUN sudo apk add --no-cache --update \
  yarn bash make cmake curl perl-utils git patch gcc g++ \
  linux-headers pkgconfig m4 musl-dev perl \
  autoconf automake bzip2-dev zlib-dev zlib-static gmp-dev

WORKDIR /usr/app

COPY ./osnap.opam .

RUN opam install . --with-test --yes && \
  eval $(opam env)

COPY . .

RUN opam exec -- dune build -p osnap --profile=static
