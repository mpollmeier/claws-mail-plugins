AC_INIT(src/plugin.c)
AC_CONFIG_AUX_DIR(config)
AM_MAINTAINER_MODE

PLUGINVERSION=0.1
AC_DEFINE_UNQUOTED(PLUGINVERSION, "$PLUGINVERSION", [plugin version])
AM_INIT_AUTOMAKE(pgpinline, $PLUGINVERSION, no-define)

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
# Check for GLib
#
AM_PATH_GLIB_2_0(2.4.0,,
        AC_MSG_ERROR(Test for GLIB failed. See the file 'INSTALL' for help.),
        gmodule gobject gthread)

AM_PATH_GTK_2_0(2.4.0,,
        AC_MSG_ERROR(Test for GTK failed. See the file 'INSTALL' for help.))
#
# Check for gpgme
#
AM_PATH_GPGME(0.3.10,,
	AC_MSG_ERROR(Test for gpgme failed. See the file 'INSTALL' for help.),)

AC_ARG_WITH(sylpheed-dir, [  --with-sylpheed-dir=DIR     Directory of the sylpheed source directory],
            sylpheed_dir="$withval", sylpheed_dir="/usr/src/sylpheed")
AC_SUBST(sylpheed_dir)

AC_OUTPUT([
Makefile
config/Makefile
src/Makefile
])
