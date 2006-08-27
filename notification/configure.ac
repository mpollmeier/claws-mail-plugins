AC_INIT(src/notification_plugin.c)
AM_MAINTAINER_MODE

PACKAGE=notification_plugin

dnl plugin version
MAJOR_VERSION=0
MINOR_VERSION=3
MICRO_VERSION=0
EXTRA_VERSION=5

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

dnl Check for sylpheed-claws
PKG_CHECK_MODULES(SYLPHEED_CLAWS, sylpheed-claws >= 1.9.15.94)
if test -z $prefix || test "${prefix}" == "NONE" ; then
  prefix=$( $PKG_CONFIG --variable=prefix sylpheed-claws )
  SYLPHEED_CLAWS_PLUGINDIR=$( $PKG_CONFIG --variable=plugindir sylpheed-claws )
else
  SYLPHEED_CLAWS_PLUGINDIR='${libdir}/sylpheed-claws/plugins'
fi
AC_SUBST(SYLPHEED_CLAWS_CFLAGS)
AC_SUBST(SYLPHEED_CLAWS_LIBS)
AC_SUBST(SYLPHEED_CLAWS_PLUGINDIR)

dnl Check for GLib
AM_PATH_GLIB_2_0(2.6.0,,
        AC_MSG_ERROR(Test for GLIB failed.),
        gmodule gobject gthread)

AM_PATH_GTK_2_0(2.6.0,,
        AC_MSG_ERROR(Test for GTK failed.))

dnl Popup for new email
AC_ARG_ENABLE(popup,
	[  --disable-popup         do not include popup support],
 	[ac_cv_enable_popup=$enableval], [ac_cv_enable_popup=yes])
AC_MSG_CHECKING([whether to include notification popup support])
if test x"$ac_cv_enable_popup" = xyes; then
   AC_DEFINE(NOTIFICATION_POPUP, 1, Activate notification popup)
fi
AC_MSG_RESULT($ac_cv_enable_popup)

dnl Banner
AC_ARG_ENABLE(banner,
	[  --disable-banner        do not include banner support],
 	[ac_cv_enable_banner=$enableval], [ac_cv_enable_banner=yes])
AC_MSG_CHECKING([whether to include notification banner support])
if test x"$ac_cv_enable_banner" = xyes; then
   AC_DEFINE(NOTIFICATION_BANNER, 1, Activate notification banner)
fi
AC_MSG_RESULT($ac_cv_enable_banner)

dnl Command
AC_ARG_ENABLE(command,
	[  --disable-command        do not include shell command support],
 	[ac_cv_enable_command=$enableval], [ac_cv_enable_command=yes])
AC_MSG_CHECKING([whether to include notification shell command support])
if test x"$ac_cv_enable_command" = xyes; then
   AC_DEFINE(NOTIFICATION_COMMAND, 1, Activate notification shell command)
fi
AC_MSG_RESULT($ac_cv_enable_command)

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
echo "Configuration Summary"
echo ""
echo "Popup   : $ac_cv_enable_popup"
echo "Banner  : $ac_cv_enable_banner"
echo "Command : $ac_cv_enable_command"
if test x"$ac_cv_enable_popup" = xno \
     -a x"$ac_cv_enable_banner" = xno \
     -a x"$ac_cv_enable_command" = xno; then
      echo ""
      echo "No features selected. The plugin will be useless!"
fi
echo ""
