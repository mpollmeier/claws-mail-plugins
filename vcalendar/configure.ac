AC_PREREQ(2.50)
AC_INIT(src/plugin.c)
AC_CONFIG_AUX_DIR(config)
AM_MAINTAINER_MODE

PLUGINVERSION=1.3
AC_DEFINE_UNQUOTED(PLUGINVERSION, "$PLUGINVERSION", [plugin version])
AM_INIT_AUTOMAKE(vcalendar, $PLUGINVERSION, no-define)

AM_CONFIG_HEADER(pluginconfig.h)

AC_PROG_CC
AC_ISC_POSIX
AM_PROG_CC_STDC
AC_PROG_INSTALL
AC_PROG_CPP
AC_PROG_LIBTOOL

#
# Find pkg-config
#
AC_PATH_PROG(PKG_CONFIG, pkg-config, no)
if test x$PKG_CONFIG = xno ; then
  AC_MSG_ERROR([*** pkg-config not found. See http://www.freedesktop.org/software/pkgconfig/])
fi

#
# Check for sylpheed-claws
#
PKG_CHECK_MODULES(SYLPHEED_CLAWS, sylpheed-claws >= 0.9.12.80)
AC_SUBST(SYLPHEED_CLAWS_CFLAGS)
AC_SUBST(SYLPHEED_CLAWS_LIBS)
SYLPHEED_CLAWS_PLUGINDIR=$( $PKG_CONFIG --variable=plugindir sylpheed-claws )
AC_SUBST(SYLPHEED_CLAWS_PLUGINDIR)

#
# Check for GLib
#
AM_PATH_GLIB_2_0(2.4.0,,
        AC_MSG_ERROR(Test for GLIB failed. See the file 'INSTALL' for help.),
        gmodule gobject gthread)

AM_PATH_GTK_2_0(2.4.0,,
        AC_MSG_ERROR(Test for GTK failed. See the file 'INSTALL' for help.))

AC_DEFUN(AC_CHECK_ICAL, [
    echo -n "Checking for libical version >= " $1 "... "
    AC_TRY_RUN([
        #include <stdio.h>
        #include <ical.h>
        int main()
        {
            int rmaj = 0, rmin = 0, amaj = 0, amin = 0;
            sscanf("$1", "%d.%d", &rmaj, &rmin);
            sscanf(ICAL_VERSION, "%d.%d", &amaj, &amin);
            printf("(found %s) ", ICAL_VERSION);
            return ((rmaj * 1000 + rmin) > (amaj * 1000 + amin)) ? 1 : 0;
        }
        ],
        [AC_MSG_RESULT(yes)],
        [
            AC_MSG_RESULT(no)
            echo "*** You need at least version " $1 " of libical"
            echo "*** See http://sourceforge.net/projects/freeassociation/"
            exit 1
        ])
    ])

AC_CHECK_ICAL(0.24)
AC_OUTPUT([
Makefile
config/Makefile
src/Makefile
])
