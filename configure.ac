AC_INIT([hdfmonkey], [0.4], [matt@west.co.tt])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_CANONICAL_HOST
AC_PROG_CC

case "$host_os" in
  mingw32*)
    AC_DEFINE([COMPAT_WIN32], 1, [Defined if needs Windows compatibility])
    ;;
esac

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([
	Makefile
	src/Makefile
])
AC_OUTPUT
