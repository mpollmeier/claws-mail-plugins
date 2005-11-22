#!/bin/sh

aclocal-1.9 \
  && libtoolize --force --copy \
  && autoheader \
  && automake-1.9 --add-missing --foreign --copy \
  && autoconf \
  && ./configure --enable-maintainer-mode $@
