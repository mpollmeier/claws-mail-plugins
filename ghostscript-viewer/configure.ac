AC_INIT(src/plugin.c)
AC_CONFIG_AUX_DIR(config)
AM_MAINTAINER_MODE

PLUGINVERSION=0.7
AC_DEFINE_UNQUOTED(PLUGINVERSION, "$PLUGINVERSION", [plugin version])
AM_INIT_AUTOMAKE(ghostscript-viewer, $PLUGINVERSION, no-define)

AM_CONFIG_HEADER(pluginconfig.h)

AC_PROG_CC
AC_ISC_POSIX
AM_PROG_CC_STDC
AC_PROG_INSTALL
AC_PROG_CPP
AM_PROG_LIBTOOL

ALL_LINGUAS="de es sk"
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION(0.12.1)
AC_DEFINE_UNQUOTED(TEXTDOMAIN, "$PACKAGE", [Gettext textdomain])

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
PKG_CHECK_MODULES(SYLPHEED_CLAWS, sylpheed-claws >= 0.9.4claws1)
AC_SUBST(SYLPHEED_CLAWS_CFLAGS)
AC_SUBST(SYLPHEED_CLAWS_LIBS)
SYLPHEED_CLAWS_PLUGINDIR=$( $PKG_CONFIG --variable=plugindir sylpheed-claws )
AC_SUBST(SYLPHEED_CLAWS_PLUGINDIR)

#
# Check for GLib
#
AM_PATH_GLIB(1.2.6,,
        AC_MSG_ERROR(Test for GLIB failed. See the file 'INSTALL' for help.),
        gthread)

#
# Check for GTK
#
AM_PATH_GTK(1.2.6,,
	AC_MSG_ERROR(Test for GTK failed. See the file 'INSTALL' for help.))

AC_CHECK_HEADER(paths.h)
AC_PATH_PROG(GS_PROG, gs)
AC_DEFINE_UNQUOTED(GS_PATH, "$GS_PROG", [Path to ghostscript])

dnl check for GS version and define appropriate alpha parameters
AC_MSG_CHECKING(for Ghostscript version...)
GS_VERSION=`$GS_PROG --version | head -c 1`
AC_MSG_RESULT(found $GS_VERSION)

AA_PARMS="-sDEVICE=x11alpha"
AC_DEFINE_UNQUOTED(ALPHA_PARAMS, "$AA_PARMS", [Ghostscript antialiasing parameters])
AC_MSG_RESULT(Antialiasing parameters for Ghostscript: $AA_PARMS)

AC_OUTPUT([
po/Makefile.in
m4/Makefile 
Makefile
config/Makefile
src/Makefile
src/libgtkgs/Makefile
])
