AC_PREREQ(2.60)
AC_INIT(src/notification_plugin.c)
AC_CONFIG_AUX_DIR(config)
AM_MAINTAINER_MODE

PACKAGE=notification_plugin

dnl plugin version
MAJOR_VERSION=0
MINOR_VERSION=22
MICRO_VERSION=0
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

AC_CANONICAL_SYSTEM

AM_INIT_AUTOMAKE($PACKAGE, $VERSION, no-define)
AM_CONFIG_HEADER(pluginconfig.h)
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

CFLAGS="$CFLAGS -Wall"
if test $USE_MAINTAINER_MODE = yes; then
	CFLAGS="$CFLAGS -DGTK_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED -DG_DISABLE_DEPRECATED"
fi

dnl Checks for iconv
AM_ICONV

dnl Find pkg-config
AC_PATH_PROG(PKG_CONFIG, pkg-config, no)
if test x$PKG_CONFIG = xno ; then
  AC_MSG_ERROR([*** pkg-config not found. See http://www.freedesktop.org/software/pkgconfig/])
fi

dnl Check for claws-mail
PKG_CHECK_MODULES(CLAWS_MAIL, claws-mail >= 3.7.1.60)
if test -z $prefix || test "${prefix}" = "NONE" ; then
  prefix=$( $PKG_CONFIG --variable=prefix claws-mail )
  CLAWS_MAIL_PLUGINDIR=$( $PKG_CONFIG --variable=plugindir claws-mail )
else
  CLAWS_MAIL_PLUGINDIR='${libdir}/claws-mail/plugins'
fi
AC_SUBST(CLAWS_MAIL_CFLAGS)
AC_SUBST(CLAWS_MAIL_LIBS)
AC_SUBST(CLAWS_MAIL_PLUGINDIR)

ALL_LINGUAS="ca cs es fi fr hu id it ja pt_BR ru zh_CN"
AC_DEFINE_UNQUOTED(TEXTDOMAIN, "$PACKAGE", [Gettext textdomain])
AM_GNU_GETTEXT_VERSION([0.15])
AM_GNU_GETTEXT([external])

dnl Check for GLib
PKG_CHECK_MODULES(GLIB, glib-2.0 >= 2.6 gmodule-2.0 >= 2.6 gobject-2.0 >= 2.6 gthread-2.0 >= 2.6)
      
GLIB_GENMARSHAL=`pkg-config --variable=glib_genmarshal glib-2.0`
AC_SUBST(GLIB_GENMARSHAL)

AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)

dnl Check for GTK+
PKG_CHECK_MODULES(GTK, gtk+-2.0 >= 2.6)

dnl Check for libnotify support
LIBNOTIFY_REQUIRED=0.4.3
AC_ARG_ENABLE(libnotify,
	[  --disable-libnotify     do not include libnotify support],
 	[enable_libnotify=$enableval], [enable_libnotify=auto])
if test "x$enable_libnotify" != "xno"; then
  PKG_CHECK_MODULES(libnotify, libnotify >= $LIBNOTIFY_REQUIRED,
    have_libnotify=yes, have_libnotify=no)
  if test "x$have_libnotify" = "xno" -a "x$enable_libnotify" = "xyes"; then
    AC_MSG_ERROR([libnotify support explicitly requested, but libnotify couldn't be found or is too old])
    have_libnotify=yes
  fi
  if test "x$have_libnotify" = "xyes"; then
    enable_libnotify=yes
  else
    AC_MSG_RESULT(no)
    enable_libnotify=no
  fi
fi
if test "x$enable_libnotify" = "xyes"; then
  AC_DEFINE(HAVE_LIBNOTIFY, 1, [Define if libnotify support is enabled])
else
  libnotify_CFLAGS=""
  libnotify_LIBS=""
fi
AC_SUBST(libnotify_CFLAGS)
AC_SUBST(libnotify_LIBS)

dnl Check for libcanberra-gtk
LIBCANBERRA_GTK_REQUIRED=0.6
AC_ARG_ENABLE(libcanberra-gtk,
	[  --disable-libcanberra-gtk  do not include libcanberra-gtk support],
	[enable_libcanberra_gtk=$enableval], [enable_libcanberra_gtk=auto])
if test "x$enable_libcanberra_gtk" != "xno"; then
	PKG_CHECK_MODULES(libcanberra_gtk, libcanberra-gtk >= $LIBCANBERRA_GTK_REQUIRED,
	  have_libcanberra_gtk=yes, have_libcanberra_gtk=no)
	if test "x$have_libcanberra_gtk" = "xno" -a "x$enable_libcanberra_gtk" = "xyes"; then
	  AC_MSG_ERROR([libcanberra-gtk support explicitely requested, but libcanberra-gtk couldn't be found or is too old])
	  have_libcanberra_gtk=no
	fi
	if test "x$have_libcanberra_gtk" = "xyes"; then
	  enable_libcanberra_gtk=yes
	else
	  enable_libcanberra_gtk=no
	fi
fi
if test "x$enable_libcanberra_gtk" = "xyes"; then
  AC_DEFINE(HAVE_LIBCANBERRA_GTK, 1, [Define if libcanberra-gtk support is enabled])
else
  libcanberra_gtk_CFLAGS=""
  libcanberra_gtk_LIBS=""
fi
AC_SUBST(libcanberra_gtk_CFLAGS)
AC_SUBST(libcanberra_gtk_LIBS)

dnl Popup for new email
AC_ARG_ENABLE(popup,
	[  --disable-popup         do not include popup support],
 	[enable_popup=$enableval], [enable_popup=yes])
AC_MSG_CHECKING([whether to include notification popup support])
if test x"$enable_popup" = xyes; then
   AC_DEFINE(NOTIFICATION_POPUP, 1, Activate notification popup)
fi
AC_MSG_RESULT($enable_popup)

dnl Banner
AC_ARG_ENABLE(banner,
	[  --disable-banner        do not include banner support],
 	[enable_banner=$enableval], [enable_banner=yes])
AC_MSG_CHECKING([whether to include notification banner support])
if test x"$enable_banner" = xyes; then
   AC_DEFINE(NOTIFICATION_BANNER, 1, Activate notification banner)
fi
AC_MSG_RESULT($enable_banner)

dnl Command
AC_ARG_ENABLE(command,
	[  --disable-command       do not include shell command support],
 	[enable_command=$enableval], [enable_command=yes])
AC_MSG_CHECKING([whether to include notification shell command support])
if test x"$enable_command" = xyes; then
   AC_DEFINE(NOTIFICATION_COMMAND, 1, Activate notification shell command)
fi
AC_MSG_RESULT($enable_command)

dnl Trayicon
AC_ARG_ENABLE(trayicon,
	[  --disable-trayicon     do not include trayicon support],
	[enable_trayicon=$enableval], [enable_trayicon=auto])
if test "x$enable_trayicon" != "xno"; then
  dnl Need GTK+ >= 2.10
  PKG_CHECK_MODULES(GTK_2_10, gtk+-2.0 >= 2.10.0,
   have_gtk_2_10=yes, have_gtk_2_10=no)
  if test "x$have_gtk_2_10" = "xno" -a "x$enable_trayicon" = "xyes"; then
    AC_MSG_ERROR([Trayicon module explicitly requested, but GTK+ >= 2.10 couldn't be found])
  fi
  if test "x$have_gtk_2_10" = "xyes"; then
    enable_trayicon=yes
    AC_DEFINE(NOTIFICATION_TRAYICON, 1, Activate notification trayicon)
  else
    enable_trayicon=no
  fi
fi
AC_MSG_CHECKING([whether to include trayicon support])
AC_MSG_RESULT($enable_trayicon)

dnl lcdproc
AC_ARG_ENABLE(lcdproc,
	[  --disable-lcdproc       do not include lcdproc support],
 	[enable_lcdproc=$enableval], [enable_lcdproc=yes])
AC_MSG_CHECKING([whether to include lcdproc support])
if test x"$enable_lcdproc" = xyes; then
   AC_DEFINE(NOTIFICATION_LCDPROC, 1, Activate lcdproc support)
fi
AC_MSG_RESULT($enable_lcdproc)

AC_SUBST(VERSION)
AC_SUBST(PLUGINVERSION)
AC_SUBST(MAJOR_VERSION)
AC_SUBST(MINOR_VERSION)
AC_SUBST(MICRO_VERSION)
AC_SUBST(EXTRA_VERSION)

AC_OUTPUT([
po/Makefile.in
m4/Makefile
config/Makefile
Makefile
src/Makefile
])

dnl Output configuration
echo ""
echo "-----------------------------------------------------------"
echo ""
echo "Configuration Summary:"
echo ""
echo "Popup    : $enable_popup"
echo "Banner   : $enable_banner"
echo "Command  : $enable_command"
echo "LCD      : $enable_lcdproc"
echo "Trayicon : $enable_trayicon"
if test x"$enable_popup" = xno \
     -a x"$enable_banner" = xno \
     -a x"$enable_trayicon" = xno \
     -a x"$enable_lcdproc" = xno \
     -a x"$enable_command" = xno; then
      echo ""
      echo "No features selected. The plugin will be useless!"
fi
echo ""
echo ""
echo "External libraries usage:"
echo ""
echo "libnotify       : $enable_libnotify"
echo "libcanberra-gtk : $enable_libcanberra_gtk"
echo ""
