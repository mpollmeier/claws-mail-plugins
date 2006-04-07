AC_PREREQ(2.50)
AC_INIT(src/gtkhtml2_viewer.c)
AC_CONFIG_AUX_DIR(config)
AM_MAINTAINER_MODE
AC_CONFIG_HEADERS(pluginconfig.h)

PLUGINVERSION=0.9
AM_INIT_AUTOMAKE(gtkhtml2_viewer, $PLUGINVERSION, no-define)
AC_SUBST(PLUGINVERSION)

AC_PROG_CC
AC_ISC_POSIX
AM_PROG_CC_STDC
AC_PROG_INSTALL
AC_PROG_CPP
AC_PROG_LIBTOOL

dnl Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS(fcntl.h sys/file.h unistd.h paths.h \
		 sys/param.h sys/utsname.h sys/select.h \
		 wchar.h wctype.h locale.h netdb.h)

AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.12.1])
ALL_LINGUAS="es pt_BR"
AC_DEFINE_UNQUOTED(TEXTDOMAIN, "$PACKAGE", [Gettext textdomain])

dnl Find pkg-config
AC_PATH_PROG(PKG_CONFIG, pkg-config, no)
if test x$PKG_CONFIG = xno ; then
  AC_MSG_ERROR([*** pkg-config not found. See http://www.freedesktop.org/software/pkgconfig/])
fi

prefix=$( $PKG_CONFIG --variable=prefix sylpheed-claws )
localedir="${prefix}/share/locale"
AC_ARG_WITH(localedir,
	[  --with-localedir=DIR    Locale directory],
	[localedir="$withval"])
AC_SUBST(localedir)
AC_DEFINE_UNQUOTED(LOCALEDIR, "${localedir}", [Locale directory])

dnl Check for sylpheed-claws
PKG_CHECK_MODULES(SYLPHEED_CLAWS, sylpheed-claws >= 2.0.0.141)
AC_SUBST(SYLPHEED_CLAWS_CFLAGS)
AC_SUBST(SYLPHEED_CLAWS_LIBS)
if test -z $prefix ; then
  SYLPHEED_CLAWS_PLUGINDIR=$( $PKG_CONFIG --variable=plugindir sylpheed-claws )
else
  SYLPHEED_CLAWS_PLUGINDIR=${prefix}/lib/sylpheed-claws/plugins
fi
AC_SUBST(SYLPHEED_CLAWS_PLUGINDIR)

dnl Check for GLib
AM_PATH_GLIB_2_0(2.4.0,,
        AC_MSG_ERROR(Test for GLIB failed. See the file 'INSTALL' for help.),
        gmodule gobject gthread)

AM_PATH_GTK_2_0(2.4.0,,
        AC_MSG_ERROR(Test for GTK failed. See the file 'INSTALL' for help.))

PKG_CHECK_MODULES(GTKHTML2, libgtkhtml-2.0, ,
		  AC_MSG_ERROR([Can't find libgtkhtml2]))
AC_SUBST(GTKHTML2_LIBS)
AC_SUBST(GTKHTML2_CFLAGS)

dnl Find curl-config
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

AC_OUTPUT([
Makefile
config/Makefile
po/Makefile.in
src/plugin_version.h
src/Makefile
])
