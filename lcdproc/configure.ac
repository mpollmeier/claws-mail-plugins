AC_INIT(src/plugin.c)
AM_MAINTAINER_MODE

AM_INIT_AUTOMAKE(sylpheed, 0.1, no-define)

AM_CONFIG_HEADER(pluginconfig.h)

AC_PROG_CC
AC_ISC_POSIX
AM_PROG_CC_STDC
AC_PROG_INSTALL
AC_PROG_CPP
AM_PROG_LIBTOOL

AM_PATH_GLIB(1.2.6,,
        AC_MSG_ERROR(Test for GLIB failed. See the file 'INSTALL' for help.),
        gthread)

AM_PATH_GTK(1.2.6,,
        AC_MSG_ERROR(Test for GTK failed. See the file 'INSTALL' for help.))

AC_ARG_WITH(sylpheed-dir, [  --with-sylpheed-dir=DIR     Directory of the sylpheed source directory],
            sylpheed_dir="$withval", sylpheed_dir="/usr/src/sylpheed")
AC_SUBST(sylpheed_dir)

AC_OUTPUT([
Makefile
src/Makefile
])
