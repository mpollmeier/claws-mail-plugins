dnl $Id: librapi2.m4,v 1.1.2.1 2007-02-09 18:49:20 colinler Exp $ vim: syntax=config
dnl Check for librapi2

AC_DEFUN([AM_PATH_LIBRAPI2], [

  AC_ARG_WITH(librapi2,
    AC_HELP_STRING(
      [--with-librapi2[=DIR]],
      [Search for librapi2 in DIR/include and DIR/lib]),
      [
				CPPFLAGS="$CPPFLAGS -I${withval}/include"
				LDFLAGS="$LDFLAGS -L${withval}/lib"
			]
    )

  AC_ARG_WITH(librapi2-include,
    AC_HELP_STRING(
      [--with-librapi2-include[=DIR]],
      [Search for librapi2 header files in DIR]),
      [
				CPPFLAGS="$CPPFLAGS -I${withval}"
			]
    )

  AC_ARG_WITH(librapi2-lib,
    AC_HELP_STRING(
      [--with-librapi2-lib[=DIR]],
      [Search for librapi2 library files in DIR]),
      [
				LDFLAGS="$LDFLAGS -L${withval}"
			]
    )


	AC_CHECK_LIB(rapi,CeRapiInit,,[
		AC_MSG_ERROR([Can't find RAPI library])
		])
	AC_CHECK_HEADERS(rapi.h,,[
		AC_MSG_ERROR([Can't find rapi.h])
		])

])
