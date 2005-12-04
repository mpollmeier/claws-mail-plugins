AC_PREREQ(2.50)
AC_INIT(src/plugin.c)
AM_MAINTAINER_MODE

AC_CONFIG_HEADERS(config.h)

PLUGINVERSION=0.3pre3
AM_INIT_AUTOMAKE(rssyl, $PLUGINVERSION, no-define)

AC_PROG_CC
AC_ISC_POSIX
AM_PROG_CC_STDC
AC_PROG_INSTALL
AC_PROG_CPP
AC_PROG_LIBTOOL

#
# Find pkg-config
#
AC_PATH_PROG(PKG_CONFIG, pkg-config, no)
if test x$PKG_CONFIG = xno ; then
  AC_MSG_ERROR([*** pkg-config not found. See http://www.freedesktop.org/software/pkgconfig/])
fi

#
# Check for sylpheed-claws
#
PKG_CHECK_MODULES(SYLPHEED_CLAWS, sylpheed-claws >= 1.9.15)
AC_SUBST(SYLPHEED_CLAWS_CFLAGS)
AC_SUBST(SYLPHEED_CLAWS_LIBS)
SYLPHEED_CLAWS_PLUGINDIR=$( $PKG_CONFIG --variable=plugindir sylpheed-claws )
AC_SUBST(SYLPHEED_CLAWS_PLUGINDIR)

#
# Check for GLib
#
AM_PATH_GLIB_2_0(2.4.0,,
        AC_MSG_ERROR(Test for GLIB failed. See the file 'INSTALL' for help.),
        gmodule gobject gthread)

AM_PATH_GTK_2_0(2.4.0,,
        AC_MSG_ERROR(Test for GTK failed. See the file 'INSTALL' for help.))

# Find curl-config
AC_PATH_PROG(CURL_CONFIG, curl-config, no)
if test x$CURL_CONFIG = xno ; then
	AC_MSG_ERROR([*** curl-config not found. Please install curl along with its development files.])
fi
CURL_CFLAGS="`$CURL_CONFIG --cflags`"

CFLAGS="$CFLAGS $CURL_CFLAGS"

CPPFLAGS="$CPPFLAGS $CURL_CFLAGS"
AC_CHECK_HEADER(curl/curl.h, [], [AC_MSG_ERROR(libcurl headers are missing)])

CURL_LIBS="`$CURL_CONFIG --libs`"

LIBS="$LIBS $CURL_LIBS"
AC_CHECK_LIB(curl,curl_global_init,,[AC_MSG_ERROR(curl libs are missing)])

PKG_CHECK_MODULES(libxml, "libxml-2.0",,
									[AC_MSG_ERROR(libxml library is missing)])

CFLAGS="$CFLAGS `pkg-config --cflags libxml-2.0`"
LIBS="$LIBS `pkg-config --libs libxml-2.0`"

AC_OUTPUT([
Makefile
src/Makefile
])
