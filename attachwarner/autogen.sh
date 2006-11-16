#!/bin/sh

aclocal \
  && libtoolize --copy --force \
  && automake --add-missing --foreign --copy \
  && autoconf \
  && ./configure --enable-maintainer-mode $@
