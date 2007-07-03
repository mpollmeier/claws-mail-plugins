AC_INIT(src/synce_plugin.c)
AM_MAINTAINER_MODE

PACKAGE=synce_plugin

dnl plugin version
MAJOR_VERSION=0
MINOR_VERSION=7
MICRO_VERSION=1
EXTRA_VERSION=0

if test \( $EXTRA_VERSION -eq 0 \); then
    if test \( $MICRO_VERSION -eq 0 \); then
        VERSION=${MAJOR_VERSION}.${MINOR_VERSION} 
    else
	VERSION=${MAJOR_VERSION}.${MINOR_VERSION}.${MICRO_VERSION}
    fi
else
    if test \( $MICRO_VERSION -eq 0 \); then
        VERSION=${MAJOR_VERSION}.${MINOR_VERSION}cvs${EXTRA_VERSION}
    else
	VERSION=${MAJOR_VERSION}.${MINOR_VERSION}.${MICRO_VERSION}cvs${EXTRA_VERSION}
    fi
fi

AM_INIT_AUTOMAKE($PACKAGE, $VERSION, no-define)
AM_CONFIG_HEADER(pluginconfig.h)
AC_DEFINE_UNQUOTED(PLUGINVERSION, "$VERSION", [plugin version])

AC_PROG_CC
AC_ISC_POSIX
AM_PROG_CC_STDC
AC_PROG_INSTALL
AC_PROG_CPP
AM_PROG_LIBTOOL

dnl find pkg-config
AC_PATH_PROG(PKG_CONFIG, pkg-config, no)
if test x$PKG_CONFIG = xno ; then
  AC_MSG_ERROR(pkg-config not found. See http://www.freedesktop.org/software/pkgconfig/)
fi

dnl check for claws-mail
PKG_CHECK_MODULES(CLAWS_MAIL, claws-mail >= 2.6.1.41)
if test -z $prefix || test "${prefix}" = "NONE" ; then
  prefix=$( $PKG_CONFIG --variable=prefix claws-mail )
  CLAWS_MAIL_PLUGINDIR=$( $PKG_CONFIG --variable=plugindir claws-mail )
else
  CLAWS_MAIL_PLUGINDIR='${libdir}/claws-mail/plugins'
fi
AC_SUBST(CLAWS_MAIL_CFLAGS)
AC_SUBST(CLAWS_MAIL_LIBS)
AC_SUBST(CLAWS_MAIL_PLUGINDIR)

#
# Check for GLib
#
AM_PATH_GLIB_2_0(2.6.0,,
        AC_MSG_ERROR(Test for GLIB failed. See the file 'INSTALL' for help.),
        gmodule gobject gthread)

#
# Check for GTK
#
AM_PATH_GTK_2_0(2.6.0,,
        AC_MSG_ERROR(Test for GTK failed. See the file 'INSTALL' for help.))

dnl Check for libsynce
PKG_CHECK_MODULES(LIBSYNCE, libsynce >= 0.9.1, synce_pkg=yes, synce_pkg=no)
if test x"$synce_pkg" = xyes; then
	CFLAGS="$CFLAGS `pkg-config --cflags libsynce`"
	LIBS="$LIBS `pkg-config --libs libsynce`"
else
	AM_PATH_LIBSYNCE
fi

dnl Check for librapi2
PKG_CHECK_MODULES(LIBRAPI2, librapi2 >= 0.9.1, rapi_pkg=yes, rapi_pkg=no)
if test x"$rapi_pkg" = xyes; then
	CFLAGS="$CFLAGS `pkg-config --cflags librapi2`"
	LIBS="$LIBS `pkg-config --libs librapi2`"
else
	AM_PATH_LIBRAPI2
fi

AC_SUBST(VERSION)
AC_SUBST(PLUGINVERSION)
AC_SUBST(MAJOR_VERSION)
AC_SUBST(MINOR_VERSION)
AC_SUBST(MICRO_VERSION)
AC_SUBST(EXTRA_VERSION)

AC_OUTPUT([
Makefile
src/Makefile
m4/Makefile
])
