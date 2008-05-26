#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.60)
AC_INIT(src/archiver.c)
AC_CONFIG_AUX_DIR(config)
AM_MAINTAINER_MODE

PACKAGE=archive

dnl plugin version

MAJOR_VERSION=0
MINOR_VERSION=5
MICRO_VERSION=0
EXTRA_VERSION=19

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

AC_DEFINE_UNQUOTED(ARCHIVEVERSION, "$PLUGINVERSION", [plugin version])
AC_DEFINE_UNQUOTED(ARCHIVEPACKAGE, "Claws Mail Archiver", [package])

AM_INIT_AUTOMAKE($PACKAGE, $VERSION, no-define)
AC_DEFINE_UNQUOTED(PLUGINVERSION, "$VERSION", [plugin version])

AC_CONFIG_HEADERS([config.h])

if test $USE_MAINTAINER_MODE = yes; then
		CFLAGS="-g -Wall"
else
		CFLAGS="$CFLAGS -O2 -Wall"
fi

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_STDC
AC_LANG_C
AC_ISC_POSIX
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LIBTOOL

# Checks for libraries.
#
# Find pkg-config
#
AC_PATH_PROG(PKG_CONFIG, pkg-config, no)
if test x$PKG_CONFIG = xno ; then
  AC_MSG_ERROR([*** pkg-config not found. See http://www.freedesktop.org/software/pkgconfig/])
fi

#
# Check for claws-mail
#
PKG_CHECK_MODULES(CLAWS_MAIL, claws-mail >= 3.4.0.65)
AC_SUBST(CLAWS_MAIL_CFLAGS)
AC_SUBST(CLAWS_MAIL_LIBS)
if test -z $prefix || test "${prefix}" = "NONE" ; then
  prefix=$( $PKG_CONFIG --variable=prefix claws-mail )
  CLAWS_MAIL_PLUGINDIR=$( $PKG_CONFIG --variable=plugindir claws-mail )
else
  CLAWS_MAIL_PLUGINDIR='${libdir}/claws-mail/plugins'
fi
AC_SUBST(CLAWS_MAIL_PLUGINDIR)

ALL_LINGUAS="fr"
AC_DEFINE_UNQUOTED(TEXTDOMAIN, "$PACKAGE", [Gettext textdomain])
AM_GNU_GETTEXT_VERSION([0.15])
AM_GNU_GETTEXT([external])

PKG_CHECK_MODULES(GLIB, [glib-2.0 >= 2.6])

GLIB_GENMARSHAL=`pkg-config --variable=glib_genmarshal glib-2.0`
AC_SUBST(GLIB_GENMARSHAL)

AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)

PKG_CHECK_MODULES(GTK, gtk+-2.0 >= 2.6)

AC_SEARCH_LIBS([archive_read_new], [archive],
	ARCHIVE_LIBS=-larchive
    AC_SUBST(ARCHIVE_LIBS),
    echo "You need to install libarchive"
    exit 1
    )

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([archive.h archive_entry.h sys/stat.h fcntl.h dirent.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T

# Checks for library functions.
AC_SYS_LARGEFILE

AC_SUBST(VERSION)
AC_SUBST(PLUGINVERSION)
AC_SUBST(MAJOR_VERSION)
AC_SUBST(MINOR_VERSION)
AC_SUBST(MICRO_VERSION)
AC_SUBST(EXTRA_VERSION)

AC_CONFIG_COMMANDS(
    summary,
    [echo ""
    echo "archive will be compiled with these settings:"
    echo ""
    echo -e "Now run make to build archive"
    echo ""
    echo -e "Please send bugs or feature requests to the maintainer(s)."
    echo -e "Email addresses can be found in the AUTHORS file."
    echo ""],
    []
)

AC_OUTPUT([
    Makefile
    config/Makefile
    po/Makefile.in
    src/Makefile
])
