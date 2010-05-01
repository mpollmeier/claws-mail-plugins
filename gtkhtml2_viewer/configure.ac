AC_PREREQ(2.60)
AC_INIT(src/gtkhtml2_viewer.c)
AC_CONFIG_AUX_DIR(config)
AM_MAINTAINER_MODE

PACKAGE=gtkhtml2_viewer

dnl plugin version
MAJOR_VERSION=0
MINOR_VERSION=27
MICRO_VERSION=0
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

AC_CANONICAL_SYSTEM

AM_INIT_AUTOMAKE($PACKAGE, $VERSION, no-define)
AC_CONFIG_HEADERS(config.h)
AC_DEFINE_UNQUOTED(PLUGINVERSION, "$VERSION", [plugin version])

AC_PROG_CC
AC_ISC_POSIX
AM_PROG_CC_STDC
AC_PROG_INSTALL
AC_PROG_CPP
AC_PROG_MAKE_SET
AC_LIBTOOL_WIN32_DLL
LT_AC_PROG_RC
AC_LIBTOOL_RC
AC_LIB_PREFIX
AC_PROG_LIBTOOL
AC_SYS_LARGEFILE


dnl Copied from the official gtk+-2 configure.in
AC_MSG_CHECKING([for some Win32 platform])
case "$host" in
  *-*-mingw*|*-*-cygwin*)
    platform_win32=yes
    LDFLAGS="$LDFLAGS -mwindows"
    ;;
  *)
    platform_win32=no
    ;;
esac
AC_MSG_RESULT([$platform_win32])
AM_CONDITIONAL(PLATFORM_WIN32, test x"$platform_win32" = x"yes")

AC_MSG_CHECKING([for native Win32])
case "$host" in
  *-*-mingw*)
    os_win32=yes
    ;;
  *)
    os_win32=no
    ;;
esac
AC_MSG_RESULT([$os_win32])
AM_CONDITIONAL(OS_WIN32, test x"$os_win32" = x"yes")

AC_MSG_CHECKING([for Cygwin])
case "$host" in
  *-*-cygwin*)
    env_cygwin=yes
    ;;
  *)
    env_cygwin=no
    ;;
esac
AC_MSG_RESULT([$env_cygwin])
AM_CONDITIONAL(CYGWIN, test x"$env_cygwin" = x"yes")

if test "$GCC" = "yes"
then
        CFLAGS="$CFLAGS -Wno-unused-function"
fi

case "$target" in
*-darwin*)
        CFLAGS="$CFLAGS -no-cpp-precomp -fno-common"
        ;;
*-*-mingw*)
        # Note that we need to link to pthreadGC2 in all cases. This
        # is because some locking is used even when pthread support is
        # disabled.
        pthread_name=pthreadGC2
        CFLAGS="$CFLAGS -mms-bitfields"
        LIBS="$LIBS -l${pthread_name} -lws2_32 -lregex"
        ;;
*-*-solaris*)
        CPPFLAGS="$CPPFLAGS -D_POSIX_PTHREAD_SEMANTICS"
        CFLAGS="$CFLAGS -DSOLARIS"
        ;;
esac


dnl Checks for iconv
AM_ICONV

dnl Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS(fcntl.h sys/file.h unistd.h paths.h \
		 sys/param.h sys/utsname.h sys/select.h \
		 wchar.h wctype.h locale.h netdb.h)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_STRUCT_TM

ALL_LINGUAS="ca cs de es fi fr hu id it ja nl pl pt_BR ru sk sr zh_CN"
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
PKG_CHECK_MODULES(CLAWS_MAIL, claws-mail >= 3.4.0.1)
AC_SUBST(CLAWS_MAIL_CFLAGS)
AC_SUBST(CLAWS_MAIL_LIBS)
AC_SUBST(CLAWS_MAIL_PLUGINDIR)

dnl Check for GLib
PKG_CHECK_MODULES(GLIB, glib-2.0 >= 2.6 gmodule-2.0 >= 2.6 gobject-2.0 >= 2.6 gthread-2.0 >= 2.6)
    
if test x"$os_win32" = xno; then
	PKG_CHECK_MODULES(GTKPRINTUNIX, gtk+-unix-print-2.0, ac_cv_enable_gtkprint=yes, ac_cv_enable_gtkprint=no)        
	if test x$ac_cv_enable_gtkprint = xyes; then
		AC_DEFINE(USE_PRINTUNIX, 1, Define if you want gtk+-unix-print support)
	else
		AC_MSG_RESULT(not found)
	fi
fi
AC_SUBST(GTKPRINTUNIX_CFLAGS)
AC_SUBST(GTKPRINTUNIX_LIBS)
  
GLIB_GENMARSHAL=`pkg-config --variable=glib_genmarshal glib-2.0`
AC_SUBST(GLIB_GENMARSHAL)

AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)

dnl Check for GTK+
PKG_CHECK_MODULES(GTK, gtk+-2.0 >= 2.6)

dnl Find curl-config
AC_PATH_PROG(CURL_CONFIG, curl-config, no)
if test x$CURL_CONFIG = xno ; then
	AC_MSG_ERROR([*** curl-config not found. Please install curl along with its development files.])
fi
CURL_CFLAGS="`$CURL_CONFIG --cflags`"

AC_MSG_CHECKING([if gcc supports -Wno-pointer-sign])
_gcc_cflags_save=$CFLAGS
CFLAGS="-Wno-pointer-sign"
AC_COMPILE_IFELSE(AC_LANG_PROGRAM([]),_gcc_psign=yes,_gcc_psign=no)
AC_MSG_RESULT($_gcc_psign)
CFLAGS=$_gcc_cflags_save;
if test x"$_gcc_psign" = xyes ; then
	CFLAGS="$CFLAGS -Wno-pointer-sign"
fi

CFLAGS="$CFLAGS $CURL_CFLAGS -Wall"
if test $USE_MAINTAINER_MODE = yes; then
	CFLAGS="$CFLAGS -DGTK_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED -DG_DISABLE_DEPRECATED"
fi

CPPFLAGS="$CPPFLAGS $CURL_CFLAGS"
AC_CHECK_HEADER(curl/curl.h, [
	AC_CHECK_LIB(curl,curl_global_init,,[AC_MSG_ERROR(curl libs are missing)])],
	[AC_MSG_ERROR(libcurl headers are missing)])

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

PKG_CHECK_MODULES(GTKHTML2, gtk+-2.0 >= 2.4.0 libxml-2.0 >= 2.4.16)


AC_ARG_ENABLE(accessibility,
              [  --enable-accessibility  enable accessibility support (needs gail) [default=no]], ,
              enable_accessibility=no)

if test x$enable_accessibility = xyes; then
  PKG_CHECK_MODULES(GAILUTIL, gail >= 1.8, ,
	            AC_MSG_ERROR([
*** gail is needed for accessibility. Either install gail or turn off 
*** accessibility support using the --disable-accessibility option.]))

  AC_DEFINE(ENABLE_ACCESSIBILITY, 1,
	    [Define to 1 to enable accessibility support.])
else
  AC_MSG_WARN([*** GtkHtml2 will be built without accessibility support.])
fi

AM_CONDITIONAL(ENABLE_ACCESSIBILITY, test x$enable_accessibility = xyes)

AC_SUBST(GTKHTML2_CFLAGS)
AC_SUBST(GTKHTML2_LIBS)
AC_SUBST(GAILUTIL_CFLAGS)
AC_SUBST(GAILUTIL_LIBS)
AC_SUBST(A11Y)

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
libgtkhtml/Makefile
libgtkhtml/css/Makefile
libgtkhtml/document/Makefile
libgtkhtml/dom/Makefile
libgtkhtml/dom/core/Makefile
libgtkhtml/dom/html/Makefile
libgtkhtml/dom/events/Makefile
libgtkhtml/dom/views/Makefile
libgtkhtml/dom/traversal/Makefile
libgtkhtml/graphics/Makefile
libgtkhtml/layout/Makefile
libgtkhtml/layout/html/Makefile
libgtkhtml/util/Makefile
libgtkhtml/view/Makefile
libgtkhtml/a11y/Makefile
src/Makefile
])
