AC_PREREQ(2.50)
AC_INIT(src/plugin.c)
AC_CONFIG_AUX_DIR(config)
AM_MAINTAINER_MODE

PLUGINVERSION=0.6
AC_DEFINE_UNQUOTED(PLUGINVERSION, "$PLUGINVERSION", [plugin version])
AM_INIT_AUTOMAKE(vcalendar, $PLUGINVERSION, no-define)

AM_CONFIG_HEADER(pluginconfig.h)

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
PKG_CHECK_MODULES(SYLPHEED_CLAWS, sylpheed-claws >= 0.9.12.80)
AC_SUBST(SYLPHEED_CLAWS_CFLAGS)
AC_SUBST(SYLPHEED_CLAWS_LIBS)
SYLPHEED_CLAWS_PLUGINDIR=$( $PKG_CONFIG --variable=plugindir sylpheed-claws )
AC_SUBST(SYLPHEED_CLAWS_PLUGINDIR)

#
# Check for GLib
#
AM_PATH_GLIB(1.2.6,,
        AC_MSG_ERROR(Test for GLIB failed. See the file 'INSTALL' for help.),
        gmodule gobject gthread)

AM_PATH_GTK(1.2.6,,
        AC_MSG_ERROR(Test for GTK failed. See the file 'INSTALL' for help.))

#
# Check for libical
#
AC_CHECK_HEADERS([ical.h])
AC_CHECK_HEADERS([icalss.h], [], [],
[#if HAVE_ICAL_H
# include <ical.h>
# endif
])AC_CHECK_LIB(ical,icaltime_compare,,AC_MSG_ERROR("Cannot find libical. Please install it or set CPPFLAGS=-I<path-to-libical>. See http://softwarestudio.org/libical/"), $GTK_LIBS)
AC_CHECK_LIB(icalss,icalfileset_new,,AC_MSG_ERROR("Cannot find libical. Please install it or set CPPFLAGS=-I<path-to-libical>. See http://softwarestudio.org/libical/"), $GTK_LIBS)
# Libical v 0.23 is missing icaltime_as_local
AC_REPLACE_FUNCS([icaltime_as_local])


AC_OUTPUT([
Makefile
config/Makefile
src/Makefile
])
