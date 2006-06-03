AC_INIT(src/fetchinfo_plugin.c)
AC_CONFIG_AUX_DIR(config)
AM_MAINTAINER_MODE

dnl plugin version
MAJOR_VERSION=0
MINOR_VERSION=4
MICRO_VERSION=14
EXTRA_VERSION=1

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
AC_DEFINE_UNQUOTED(PLUGINVERSION, "$VERSION", [plugin version])

AM_INIT_AUTOMAKE(fetchinfo-plugin, $PLUGINVERSION, no-define)
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
PKG_CHECK_MODULES(SYLPHEED_CLAWS, sylpheed-claws >= 1.9.100.93)
if test -z $prefix || test "${prefix}" == "NONE" ; then
  prefix=$( $PKG_CONFIG --variable=prefix sylpheed-claws )
  SYLPHEED_CLAWS_PLUGINDIR=$( $PKG_CONFIG --variable=plugindir sylpheed-claws )
else
  SYLPHEED_CLAWS_PLUGINDIR='${libdir}/sylpheed-claws/plugins'
fi
AC_SUBST(SYLPHEED_CLAWS_CFLAGS)
AC_SUBST(SYLPHEED_CLAWS_LIBS)
AC_SUBST(SYLPHEED_CLAWS_PLUGINDIR)

#
# Check for GLib
#
AM_PATH_GLIB_2_0(2.4.0,,
        AC_MSG_ERROR(Test for GLIB failed. See the file 'INSTALL' for help.),
        gmodule gobject gthread)

#
# Check for GTK
#
AM_PATH_GTK_2_0(2.4.0,,
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
src/plugin_version.h
src/Makefile
])
