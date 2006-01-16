AC_INIT(src/plugin.c)
AM_MAINTAINER_MODE

PLUGINVERSION=1.10
AC_DEFINE_UNQUOTED(PLUGINVERSION, "$PLUGINVERSION", [plugin version])
AM_INIT_AUTOMAKE(mailmbox, $PLUGINVERSION, no-define)
AC_SUBST(PLUGINVERSION)

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
PKG_CHECK_MODULES(SYLPHEED_CLAWS, sylpheed-claws >= 1.9.100.160)
SYLPHEED_CLAWS_PLUGINDIR=$( $PKG_CONFIG --variable=plugindir sylpheed-claws )
AC_SUBST(SYLPHEED_CLAWS_CFLAGS)
AC_SUBST(SYLPHEED_CLAWS_LIBS)
AC_SUBST(SYLPHEED_CLAWS_PLUGINDIR)

AM_PATH_GLIB_2_0(2.2.0,,
        AC_MSG_ERROR(Test for GLIB failed. See the file 'INSTALL' for help.),
        gthread)

AM_PATH_GTK_2_0(2.4.0,,
	AC_MSG_ERROR(Test for GTK failed. See the file 'INSTALL' for help.))

AC_OUTPUT([
Makefile
src/plugin_version.h
src/Makefile
])
