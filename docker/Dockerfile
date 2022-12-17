FROM ocaml/opam:alpine-ocaml-4.14-flambda as build

RUN sudo apk add --no-cache --update \
  yarn bash make curl perl-utils git patch gcc g++ \
  linux-headers pkgconfig m4 musl-dev perl \
  autoconf automake bzip2 bzip2-dev zlib zlib-dev gmp-dev

WORKDIR /usr/app

RUN opam update

COPY . .

RUN opam install ./*.opam --deps-only --with-test --yes

RUN git -C /usr/app apply /usr/app/static.patch

RUN dune build -p osnap --profile=release
