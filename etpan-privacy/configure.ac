AC_INIT(src/plugin.c)
AM_MAINTAINER_MODE

PLUGINVERSION=0.8
AC_DEFINE_UNQUOTED(PLUGINVERSION, "$PLUGINVERSION", [plugin version])
AM_INIT_AUTOMAKE(etpan-privacy, $PLUGINVERSION, no-define)

AM_CONFIG_HEADER(pluginconfig.h)

AC_PROG_CC
AC_ISC_POSIX
AM_PROG_CC_STDC
AC_PROG_INSTALL
AC_PROG_CPP
AM_PROG_LIBTOOL

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
PKG_CHECK_MODULES(SYLPHEED_CLAWS, sylpheed-claws >= 0.9.12.76)
SYLPHEED_CLAWS_PLUGINDIR=$( $PKG_CONFIG --variable=plugindir sylpheed-claws )
AC_SUBST(SYLPHEED_CLAWS_CFLAGS)
AC_SUBST(SYLPHEED_CLAWS_LIBS)
AC_SUBST(SYLPHEED_CLAWS_PLUGINDIR)

AM_PATH_GLIB(1.2.6,,
        AC_MSG_ERROR(Test for GLIB failed. See the file 'INSTALL' for help.),
        gthread)

AM_PATH_GTK(1.2.6,,
	AC_MSG_ERROR(Test for GTK failed. See the file 'INSTALL' for help.))

libetpan=no
AC_PATH_PROG(libetpanconfig, [libetpan-config])
if test "x$libetpanconfig" != "x"; then
  CPPFLAGS="$CPPFLAGS `$libetpanconfig --cflags 2>/dev/null`"
  AC_CHECK_HEADER(libetpan/libetpan.h, [libetpan=yes])
  if test "x$libetpan" = "xyes"; then
    AC_MSG_CHECKING([whether libetpan-config hints compiles and links fine])
    LDFLAGS="$LDFLAGS `$libetpanconfig --libs 2>/dev/null`"
    AC_TRY_LINK([], [mailmessage_check();], [libetpan=yes], [libetpan=no])
    AC_MSG_RESULT([$libetpan])
  fi
fi
test "x$libetpan" = "xno" && AC_MSG_ERROR([libetpan support required])

AC_OUTPUT([
Makefile
config/Makefile
src/Makefile
])
