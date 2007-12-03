AC_PREREQ(2.60)
AC_INIT([src/poppler_viewer.c])
AC_CONFIG_AUX_DIR(config)
AM_MAINTAINER_MODE

PACKAGE=pdf_viewer

dnl plugin version
MAJOR_VERSION=0
MINOR_VERSION=7
MICRO_VERSION=0
EXTRA_VERSION=4

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
AC_CONFIG_HEADERS(config.h)
AC_DEFINE_UNQUOTED(PLUGINVERSION, "$VERSION", [plugin version])

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
AC_CHECK_HEADERS(dlfcn.h fcntl.h sys/file.h unistd.h paths.h \
		 sys/param.h sys/utsname.h sys/select.h \
		 wchar.h wctype.h locale.h libintl.h)

AC_CHECK_FUNCS(memset strchr strrchr strstr)

AC_C_CONST
AC_PROG_LIBTOOL
ALL_LINGUAS="ca de es fr fi hu it pl pt_BR sk sr"
AC_DEFINE_UNQUOTED(TEXTDOMAIN, "$PACKAGE", [Gettext textdomain])

AM_GNU_GETTEXT_VERSION([0.15])
AM_GNU_GETTEXT([external])

dnl Find pkg-config
AC_PATH_PROG(PKG_CONFIG, pkg-config, no)
if test x$PKG_CONFIG = xno ; then
  AC_MSG_ERROR([*** pkg-config not found. See http://www.freedesktop.org/software/pkgconfig/])
fi

if test -z $prefix || test "${prefix}" = "NONE" ; then
  prefix=$( $PKG_CONFIG --variable=prefix claws-mail )
  CLAWS_MAIL_PLUGINDIR=$( $PKG_CONFIG --variable=plugindir claws-mail )
else
  CLAWS_MAIL_PLUGINDIR='${libdir}/claws-mail/plugins'
fi

dnl Check for claws-mail
PKG_CHECK_MODULES(CLAWS_MAIL, claws-mail >= 3.1.0.0)
AC_SUBST(CLAWS_MAIL_CFLAGS)
AC_SUBST(CLAWS_MAIL_LIBS)
AC_SUBST(CLAWS_MAIL_PLUGINDIR)

dnl Check for GLib
PKG_CHECK_MODULES(GLIB, glib-2.0 >= 2.6 gmodule-2.0 >= 2.6 gobject-2.0 >= 2.6 gthread-2.0 >= 2.6)
      
GLIB_GENMARSHAL=`pkg-config --variable=glib_genmarshal glib-2.0`
AC_SUBST(GLIB_GENMARSHAL)

AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)

dnl Check for GTK+
PKG_CHECK_MODULES(GTK, gtk+-2.0 >= 2.6)

PKG_CHECK_MODULES(POPPLER, poppler-glib >= 0.4.2, ,
				  AC_MSG_ERROR([Can't find Poppler >= 0.4.2 Glib wrapper]))
AC_SUBST(POPPLER_LIBS)
AC_SUBST(POPPLER_CFLAGS)

OLD_CFLAGS=$CFLAGS
CFLAGS="$POPPLER_CFLAGS $GTK_CFLAGS $GLIB_CFLAGS"
AC_CHECK_DECL(POPPLER_DEST_NAMED,
 	[AC_DEFINE([HAVE_POPPLER_DEST_NAMED], [], [Description])],
 	,[#include <poppler-action.h>])
AC_CHECK_DECL(POPPLER_DEST_XYZ,
 	[AC_DEFINE([HAVE_POPPLER_DEST_XYZ], [], [Description])],
 	,[#include <poppler-action.h>])
CFLAGS=$OLD_CFLAGS

dnl Find gs
AC_PATH_PROG(GS_EXEC, gs, no)
if test x$GS_EXEC = xno ; then
	AC_MSG_ERROR([*** gs not found. Please install gs.])
fi

dnl check for pthread support
AC_ARG_ENABLE(pthread,
	[  --disable-pthread           Disable pthread support],
	[ac_cv_enable_pthread=$enableval], [ac_cv_enable_pthread=yes])
AC_MSG_CHECKING([whether to use pthread])
if test x$ac_cv_enable_pthread = xno; then
	AC_MSG_RESULT(no)
else
	AC_MSG_RESULT(yes)

        # For W32 we need to use a special ptrhead lib. In this case we can't
        # use AC_CHECK_LIB because it has no means of checking for a
        # library installed under a different name.  Checking for the
        # header is okay.
        if test -n "${pthread_name}" ; then
           ac_cv_enable_pthread=yes
        else
	AC_CHECK_LIB(pthread, pthread_create, :, ac_cv_enable_pthread=no)
        fi
	AC_CHECK_HEADERS(pthread.h, :, ac_cv_enable_pthread=no)

	if test x$ac_cv_enable_pthread = xyes; then
		AC_DEFINE(USE_PTHREAD, 1, Define if you have pthread)
                if test -z "${pthread_name}" ; then
		PTHREAD_LIBS="-lpthread"
	fi
	fi

fi
AC_SUBST(PTHREAD_LIBS)

AC_SUBST(VERSION)
AC_SUBST(PLUGINVERSION)
AC_SUBST(MAJOR_VERSION)
AC_SUBST(MINOR_VERSION)
AC_SUBST(MICRO_VERSION)
AC_SUBST(EXTRA_VERSION)

AC_OUTPUT([
Makefile
config/Makefile
po/Makefile.in
src/Makefile
])
