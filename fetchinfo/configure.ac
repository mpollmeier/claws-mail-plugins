AC_INIT(src/fetchinfo_plugin.c)
AC_CONFIG_AUX_DIR(config)
AM_MAINTAINER_MODE

#
# plugin version
#
MAJOR_VERSION=0
MINOR_VERSION=4
MICRO_VERSION=4
EXTRA_VERSION=0
VERSION=${MAJOR_VERSION}.${MINOR_VERSION}.${MICRO_VERSION}.${EXTRA_VERSION}

#
# extract sylpheed version
#
SYLPHEED_VER=`pkg-config --modversion sylpheed-claws`
REST=${SYLPHEED_VER//./ }
REST=${REST//claws/ }
SYLPHEED_MAJOR=${REST%% *}
REST=${REST#* }
SYLPHEED_MINOR=${REST%% *}
REST=${REST#* }
SYLPHEED_MICRO=${REST%% *}
REST=${REST#* }
SYLPHEED_EXTRA=${REST#* }

AM_INIT_AUTOMAKE(fetchinfo-plugin, $VERSION, no-define)

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
PKG_CHECK_MODULES(SYLPHEED_CLAWS, sylpheed-claws >= 0.9.4claws)
SYLPHEED_CLAWS_PLUGINDIR=$( $PKG_CONFIG --variable=plugindir sylpheed-claws )
AC_SUBST(SYLPHEED_CLAWS_CFLAGS)
AC_SUBST(SYLPHEED_CLAWS_LIBS)
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

AC_ARG_WITH(sylpheed-dir, [  --with-sylpheed-dir=DIR     Directory of the sylpheed source directory],
            sylpheed_dir="$withval", sylpheed_dir="/usr/src/sylpheed")
AC_SUBST(sylpheed_dir)

AC_SUBST(VERSION)
AC_SUBST(MAJOR_VERSION)
AC_SUBST(MINOR_VERSION)
AC_SUBST(MICRO_VERSION)
AC_SUBST(EXTRA_VERSION)
AC_SUBST(SYLPHEED_MAJOR)
AC_SUBST(SYLPHEED_MINOR)
AC_SUBST(SYLPHEED_MICRO)
AC_SUBST(SYLPHEED_EXTRA)

AC_OUTPUT([
Makefile
src/Makefile
win32/version.rc
])
