#!/bin/sh

aclocal -I m4 \
  && libtoolize --copy --force \
  && autoheader \
  && automake --add-missing --foreign --copy \
  && autoconf \
  && ./configure --enable-maintainer-mode $@
