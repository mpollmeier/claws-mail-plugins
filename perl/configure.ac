AC_INIT(src/perl_plugin.c)
AM_MAINTAINER_MODE

PACKAGE=perl_plugin

dnl plugin version
MAJOR_VERSION=0
MINOR_VERSION=2
MICRO_VERSION=0
EXTRA_VERSION=13

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

dnl check for sylpheed-claws
PKG_CHECK_MODULES(SYLPHEED_CLAWS, sylpheed-claws >= 1.0.3.4)
SYLPHEED_CLAWS_PLUGINDIR=$( $PKG_CONFIG --variable=plugindir sylpheed-claws )
AC_SUBST(SYLPHEED_CLAWS_CFLAGS)
AC_SUBST(SYLPHEED_CLAWS_LIBS)
AC_SUBST(SYLPHEED_CLAWS_PLUGINDIR)

dnl check for GLib
AM_PATH_GLIB(1.2.6,,
  AC_MSG_ERROR(Test for GLIB failed.),
  gthread)

dnl check for GTK
AM_PATH_GTK(1.2.6,,
  AC_MSG_ERROR(Test for GTK failed.))

dnl check for Sed
AC_PATH_PROG(sedpath, sed, no)
if test x$sedpath = xno ; then
  AC_MSG_ERROR(Test for sed failed.)
fi

dnl Perl
AC_PATH_PROG(PERL_PATH, perl, no)
if test x$PERL_PATH = xno ; then
  AC_MSG_ERROR(Test for Perl failed)
fi
AC_MSG_CHECKING(for perl >= 5.8.0)
PERL_VER=`$PERL_PATH -e 'print $] > 5.0079999?"yes":"no"'`
if test "$PERL_VER" = "yes"; then
  AC_MSG_RESULT(yes)
else
  AC_MSG_RESULT(no)
  AC_MSG_ERROR(Your Perl version is too old.)
fi
AC_MSG_CHECKING(for Perl compile flags)
PERL_CFLAGS=`$PERL_PATH -MExtUtils::Embed -e ccopts`
PERL_CFLAGS=`echo $PERL_CFLAGS | $sedpath 's/-D_FILE_OFFSET_BITS=[[0-9]]*//'`
PERL_LDFLAGS=`$PERL_PATH -MExtUtils::Embed -e ldopts |$sedpath 's/-lgdbm//'`
PERL_LDFLAGS=`echo $PERL_LDFLAGS |$sedpath 's/-ldb//'`
PERL_LDFLAGS=`echo $PERL_LDFLAGS |$sedpath 's/-lndbm//'`
PERL_LDFLAGS=`echo $PERL_LDFLAGS |$sedpath 's/-lc//'`
AC_MSG_RESULT(ok)
AC_SUBST(PERL_CFLAGS)
AC_SUBST(PERL_LDFLAGS)

AC_SUBST(VERSION)
AC_SUBST(MAJOR_VERSION)
AC_SUBST(MINOR_VERSION)
AC_SUBST(MICRO_VERSION)
AC_SUBST(EXTRA_VERSION)

AC_OUTPUT([
Makefile
src/Makefile
tools/Makefile
])
