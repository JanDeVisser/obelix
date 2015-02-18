AC_INIT(obelix, 0.1, jan@finiandarcy.com)
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_MACRO_DIR([m4])
AC_USE_SYSTEM_EXTENSIONS

AM_INIT_AUTOMAKE([-Wall])
LT_INIT
AM_PATH_CHECK([0.9.0],[],[AC_MSG_ERROR([check is missing])])

AC_PROG_CC

AC_CONFIG_SRCDIR(src/bin/obelix.c)
AC_CONFIG_HEADERS([config.h])
AC_CHECK_FUNCS([asprintf])
AC_CHECK_FUNCS([vasprintf])
AC_CONFIG_FILES([ \
  Makefile \
  src/lib/Makefile \
  src/lib/test/Makefile \
  src/types/Makefile \
  src/types/test/Makefile \
  src/parser/Makefile \
  src/parser/test/Makefile \
  src/bin/Makefile \
  share/Makefile \
  test/Makefile \
  test/script/Makefile])
AC_OUTPUT
