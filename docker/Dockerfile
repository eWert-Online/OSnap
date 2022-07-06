FROM esydev/esy:nightly-alpine-latest as build

RUN apk add --no-cache --update \
  yarn bash make curl perl-utils git patch gcc g++ \
  linux-headers pkgconfig m4 musl-dev perl \
  autoconf automake bzip2 bzip2-dev zlib zlib-dev

WORKDIR /usr/app

COPY ./package.json .
COPY ./esy.lock .

RUN esy install --ocaml-pkg-name ocaml --ocaml-version 4.14.0 && \
  esy build-dependencies --ocaml-pkg-name ocaml --ocaml-version 4.14.0

COPY . .

RUN git -C /usr/app apply /usr/app/static.patch

RUN esy install --ocaml-pkg-name ocaml --ocaml-version 4.14.0

RUN esy release --static --ocaml-pkg-name ocaml --ocaml-version 4.14.0

RUN yarn global remove esy && \
  rm -rf /usr/local/share/.cache/yarn && \
  yarn global --prefix=/usr/local --force add /usr/app/_release
