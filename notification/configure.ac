AC_INIT(src/notification_plugin.c)
AM_MAINTAINER_MODE

PACKAGE=notification_plugin

dnl plugin version
MAJOR_VERSION=0
MINOR_VERSION=6
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

AM_INIT_AUTOMAKE($PACKAGE, $VERSION, no-define)
AM_CONFIG_HEADER(pluginconfig.h)
AC_DEFINE_UNQUOTED(PLUGINVERSION, "$VERSION", [plugin version])

AC_PROG_CC
AC_ISC_POSIX
AM_PROG_CC_STDC
AC_PROG_INSTALL
AC_PROG_CPP
AC_PROG_LIBTOOL

dnl Find pkg-config
AC_PATH_PROG(PKG_CONFIG, pkg-config, no)
if test x$PKG_CONFIG = xno ; then
  AC_MSG_ERROR([*** pkg-config not found. See http://www.freedesktop.org/software/pkgconfig/])
fi

dnl Check for claws-mail
PKG_CHECK_MODULES(CLAWS_MAIL, claws-mail >= 2.6.0.64)
if test -z $prefix || test "${prefix}" == "NONE" ; then
  prefix=$( $PKG_CONFIG --variable=prefix claws-mail )
  CLAWS_MAIL_PLUGINDIR=$( $PKG_CONFIG --variable=plugindir claws-mail )
else
  CLAWS_MAIL_PLUGINDIR='${libdir}/claws-mail/plugins'
fi
AC_SUBST(CLAWS_MAIL_CFLAGS)
AC_SUBST(CLAWS_MAIL_LIBS)
AC_SUBST(CLAWS_MAIL_PLUGINDIR)

dnl Check for GLib
AM_PATH_GLIB_2_0(2.6.0,,
        AC_MSG_ERROR(Test for GLIB failed.),
        gmodule gobject gthread)

AM_PATH_GTK_2_0(2.6.0,,
        AC_MSG_ERROR(Test for GTK failed.))

dnl Check for libnotify support
LIBNOTIFY_REQUIRED=9.3.2
dnl LIBNOTIFY_REQUIRED=0.4.3
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

AC_SUBST(VERSION)
AC_SUBST(PLUGINVERSION)
AC_SUBST(MAJOR_VERSION)
AC_SUBST(MINOR_VERSION)
AC_SUBST(MICRO_VERSION)
AC_SUBST(EXTRA_VERSION)

AC_OUTPUT([
Makefile
src/Makefile
])

dnl Output configuration
echo ""
echo "-----------------------------------------------------------"
echo ""
echo "Configuration Summary:"
echo ""
echo "Popup   : $enable_popup"
echo "Banner  : $enable_banner"
echo "Command : $enable_command"
if test x"$enable_popup" = xno \
     -a x"$enable_banner" = xno \
     -a x"$enable_command" = xno; then
      echo ""
      echo "No features selected. The plugin will be useless!"
fi
echo ""
echo ""
echo "External libraries usage:"
echo ""
echo "libnotify : $enable_libnotify"
echo ""
