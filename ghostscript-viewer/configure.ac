AC_INIT(src/plugin.c)
AC_CONFIG_AUX_DIR(config)
AM_MAINTAINER_MODE

AM_INIT_AUTOMAKE(ghostscript-viewer, 0.3, no-define)

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

AC_CHECK_HEADER(paths.h)
AC_PATH_PROG(GS_PROG, gs)
AC_DEFINE_UNQUOTED(GS_PATH, "$GS_PROG", [Path to ghostscript])

dnl check for GS version and define appropriate alpha parameters
AC_MSG_CHECKING(for Ghostscript version...)
GS_VERSION=`$GS_PROG --version | head -c 1`
AC_MSG_RESULT(found $GS_VERSION)

AA_PARMS="-sDEVICE=x11alpha"
AC_DEFINE_UNQUOTED(ALPHA_PARAMS, "$AA_PARMS", [Ghostscript antialiasing parameters])
AC_MSG_RESULT(Antialiasing parameters for Ghostscript: $AA_PARMS)

AC_OUTPUT([
Makefile
config/Makefile
src/Makefile
src/libgtkgs/Makefile
])
