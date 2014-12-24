AC_INIT(obelix, 0.1, jan@finiandarcy.com)
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([-Wall])
LT_INIT
AM_PATH_CHECK([0.9.0],[],[AC_MSG_ERROR([check is missing])])

AC_PROG_CC

AC_CONFIG_SRCDIR(src/bin/obelix.c)
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile src/lib/Makefile src/types/Makefile src/parser/Makefile src/bin/Makefile test/Makefile])
AC_OUTPUT
