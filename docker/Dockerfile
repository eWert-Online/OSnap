FROM alpine:latest as zlib-build

ENV CFLAGS="-fPIC -O3"
WORKDIR /usr/zlib

RUN apk add --no-cache --update make git gcc g++

RUN git clone https://github.com/madler/zlib.git .

RUN ./configure --prefix=/usr --libdir=/lib --static && \
  make && \
  make install pkgconfigdir="/usr/lib/pkgconfig"


FROM ocaml/opam:alpine-ocaml-4.14-flambda

RUN sudo apk add --no-cache --update \
  bash make cmake curl perl-utils git patch gcc g++ \
  linux-headers pkgconfig m4 musl-dev perl \
  autoconf automake bzip2-dev gmp-dev

COPY --from=zlib-build /lib/libz.a /lib/libz.a
COPY --from=zlib-build /usr/lib/pkgconfig/zlib.pc /usr/lib/pkgconfig/zlib.pc
COPY --from=zlib-build /usr/include/zlib.h /usr/include/zlib.h
COPY --from=zlib-build /usr/include/zconf.h /usr/include/zconf.h

WORKDIR /usr/app

COPY ./osnap.opam .

RUN opam install . --with-test --yes && \
  eval $(opam env)

COPY . .

RUN opam exec -- dune build -p osnap --profile=static
