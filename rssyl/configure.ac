AC_PREREQ(2.50)
AC_INIT(src/plugin.c)
AC_CONFIG_AUX_DIR(config)
AM_MAINTAINER_MODE
AC_CONFIG_HEADERS(config.h)

PLUGINVERSION=0.4pre2
AM_INIT_AUTOMAKE(rssyl, $PLUGINVERSION, no-define)
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
ALL_LINGUAS="es fr sk sv zh_CN"
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
PKG_CHECK_MODULES(SYLPHEED_CLAWS, sylpheed-claws >= 1.9.100.0)
AC_SUBST(SYLPHEED_CLAWS_CFLAGS)
AC_SUBST(SYLPHEED_CLAWS_LIBS)
SYLPHEED_CLAWS_PLUGINDIR=$( $PKG_CONFIG --variable=plugindir sylpheed-claws )
AC_SUBST(SYLPHEED_CLAWS_PLUGINDIR)

dnl Check for GLib
AM_PATH_GLIB_2_0(2.4.0,,
        AC_MSG_ERROR(Test for GLIB failed. See the file 'INSTALL' for help.),
        gmodule gobject gthread)

AM_PATH_GTK_2_0(2.4.0,,
        AC_MSG_ERROR(Test for GTK failed. See the file 'INSTALL' for help.))

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

PKG_CHECK_MODULES(libxml, "libxml-2.0",,
									[AC_MSG_ERROR(libxml library is missing)])

CFLAGS="$CFLAGS `pkg-config --cflags libxml-2.0`"
LIBS="$LIBS `pkg-config --libs libxml-2.0`"

AC_OUTPUT([
Makefile
config/Makefile
po/Makefile.in
src/plugin_version.h
src/Makefile
])
