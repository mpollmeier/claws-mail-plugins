AC_PREREQ(2.60)
AC_INIT(src/plugin.c)
AC_CONFIG_AUX_DIR(config)
AM_MAINTAINER_MODE

PACKAGE=vcalendar

dnl plugin version
MAJOR_VERSION=1
MINOR_VERSION=98
MICRO_VERSION=0
EXTRA_VERSION=3

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

AC_DEFINE_UNQUOTED(CALVERSION, "$PLUGINVERSION", [plugin version])
AC_DEFINE_UNQUOTED(CALPACKAGE, "Claws Mail vCalendar", [package])

AM_INIT_AUTOMAKE($PACKAGE, $VERSION, no-define)
AC_DEFINE_UNQUOTED(PLUGINVERSION, "$VERSION", [plugin version])

AC_PROG_CC
AC_ISC_POSIX
AM_PROG_CC_STDC
AC_PROG_INSTALL
AC_PROG_CPP
AC_PROG_LIBTOOL
AM_PROG_LEX
AC_PROG_YACC

case "$host" in
  *-sun-solaris2.8)
    CPPFLAGS="$CPPFLAGS -D_POSIX_PTHREAD_SEMANTICS"
    CFLAGS="$CFLAGS -DSOLARIS"
    ;;
esac
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
PKG_CHECK_MODULES(CLAWS_MAIL, claws-mail >= 2.10.0.177)
AC_SUBST(CLAWS_MAIL_CFLAGS)
AC_SUBST(CLAWS_MAIL_LIBS)
if test -z $prefix || test "${prefix}" = "NONE" ; then
  prefix=$( $PKG_CONFIG --variable=prefix claws-mail )
  CLAWS_MAIL_PLUGINDIR=$( $PKG_CONFIG --variable=plugindir claws-mail )
else
  CLAWS_MAIL_PLUGINDIR='${libdir}/claws-mail/plugins'
fi
AC_SUBST(CLAWS_MAIL_PLUGINDIR)

ALL_LINGUAS="ca de es fi fr hu it pl pt_BR sk sr zh_CN"
AC_DEFINE_UNQUOTED(TEXTDOMAIN, "$PACKAGE", [Gettext textdomain])
AM_GNU_GETTEXT_VERSION([0.15])
AM_GNU_GETTEXT([external])

AC_CHECK_PROGS(PERL, perl5 perl)

dnl Check for GLib
PKG_CHECK_MODULES(GLIB, glib-2.0 >= 2.6 gmodule-2.0 >= 2.6 gobject-2.0 >= 2.6 gthread-2.0 >= 2.6)
      
GLIB_GENMARSHAL=`pkg-config --variable=glib_genmarshal glib-2.0`
AC_SUBST(GLIB_GENMARSHAL)

AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)

dnl Check for GTK+
PKG_CHECK_MODULES(GTK, gtk+-2.0 >= 2.6)
	
dnl MY_CURL
dnl -------
dnl set my_cv_curl_vers to the version of libcurl or NONE
dnl if libcurl is not found or is too old

AC_DEFUN([MY_CURL],[
 AC_CACHE_VAL(my_cv_curl_vers,[
 my_cv_curl_vers=NONE
 check="7.9.7"
 check_hex="070907"

 AC_MSG_CHECKING([for curl >= $check])

 if eval curl-config --version 2>/dev/null >/dev/null; then
   ver=`curl-config --version | sed -e "s/libcurl //g"`
   hex_ver=`curl-config --vernum | tr 'a-f' 'A-F'`
   bc_exists=`which bc`
   if test x$bc_exists != x; then
    ok=`echo "ibase=16; if($hex_ver>=$check_hex) $hex_ver else 0" | bc`
   else
    ok=1
   fi
   if test x$ok != x0; then
     my_cv_curl_vers="$ver"
     AC_MSG_RESULT([$my_cv_curl_vers])
   else
     AC_MSG_RESULT(FAILED)
     AC_MSG_WARN([$ver is too old. Need version $check or higher.])
   fi
 else
   AC_MSG_RESULT(FAILED)
   AC_MSG_WARN([curl-config was not found])
 fi
 ])
])
MY_CURL
if test $my_cv_curl_vers != NONE; then
  CURL_CFLAGS=`curl-config --cflags`
  CURL_LIBS=`curl-config --libs`
  if test "x$build_vendor" = "xapple" ; then
    if test -f /usr/bin/libcurl.a ; then
      # prefer to link static
      CURL_LIBS=/usr/bin/libcurl.a
    else
      # mac os x curl-config outputs erroneous -arch i386 in libs
      CURL_LIBS="`echo $CURL_LIBS | sed 's/-arch i386//'`"
    fi
  fi
else
  AC_MSG_ERROR([libcurl is required. You must install it. \
See http://curl.haxx.se/])
fi
AC_SUBST(CURL_CFLAGS)

LIBS="$CURL_LIBS $LIBS"

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

AC_DEFINE(ICAL_ERRORS_ARE_FATAL, 1, [fatal errors])
AC_DEFINE(ICAL_SAFESAVES, 1, [safesaves])
AC_DEFINE(ICAL_UNIX_NEWLINE, 1, [unix newline])

AC_CHECK_HEADERS(time.h sys/types.h assert.h wchar.h)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T
AC_STRUCT_TM

dnl Checks for library functions.
AC_CHECK_FUNCS(strdup)

AC_CONFIG_HEADERS(config.h)

AC_SUBST(VERSION)
AC_SUBST(PLUGINVERSION)
AC_SUBST(MAJOR_VERSION)
AC_SUBST(MINOR_VERSION)
AC_SUBST(MICRO_VERSION)
AC_SUBST(EXTRA_VERSION)

AC_OUTPUT([ m4/Makefile
Makefile
config/Makefile
po/Makefile.in
src/Makefile
libical/Makefile
libical/libical/icalversion.h
libical/libical/Makefile
libical/design-data/Makefile
libical/scripts/Makefile
])
