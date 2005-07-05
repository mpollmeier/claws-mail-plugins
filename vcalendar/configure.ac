AC_PREREQ(2.50)
AC_INIT(src/plugin.c)
AC_CONFIG_AUX_DIR(config)
AM_MAINTAINER_MODE

PLUGINVERSION=1.4
AC_DEFINE_UNQUOTED(PLUGINVERSION, "$PLUGINVERSION", [plugin version])
AC_DEFINE_UNQUOTED(CALVERSION, "$PLUGINVERSION", [plugin version])
AC_DEFINE_UNQUOTED(CALPACKAGE, "Sylpheed-Claws vCalendar", [package])
AM_INIT_AUTOMAKE(vcalendar, $PLUGINVERSION, no-define)

AM_CONFIG_HEADER(pluginconfig.h)

AC_PROG_CC
AC_ISC_POSIX
AM_PROG_CC_STDC
AC_PROG_INSTALL
AC_PROG_CPP
AC_PROG_LIBTOOL
AM_PROG_LEX
AC_PROG_YACC

AC_CHECK_PROGS(PERL, perl5 perl)

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
AM_PATH_GLIB_2_0(2.4.0,,
        AC_MSG_ERROR(Test for GLIB failed. See the file 'INSTALL' for help.),
        gmodule gobject gthread)

AM_PATH_GTK_2_0(2.4.0,,
        AC_MSG_ERROR(Test for GTK failed. See the file 'INSTALL' for help.))

AC_DEFINE(ICAL_ERRORS_ARE_FATAL, 1, [fatal errors])
AC_DEFINE(ICAL_SAFESAVES, 1, [safesaves])
AC_DEFINE(ICAL_UNIX_NEWLINE, 1, [unix newline])

AC_CHECK_HEADERS(time.h sys/types.h assert.h)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T
AC_STRUCT_TM

dnl Checks for library functions.
AC_CHECK_FUNCS(strdup)

AC_CONFIG_HEADERS(config.h)

AC_OUTPUT([
Makefile
config/Makefile
src/Makefile
libical/Makefile
libical/libical/icalversion.h
libical/libical/Makefile
libical/design-data/Makefile
libical/scripts/Makefile
])
