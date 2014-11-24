TESTS = 	 core list array dict set str buffer resolve lexer
check_PROGRAMS = core list array dict set str buffer resolve lexer

AM_CPPFLAGS =   -I$(top_srcdir)/include
AM_CFLAGS =	@CHECK_CFLAGS@
AM_LDFLAGS =	-pthread
LDADD =		$(top_builddir)/src/libobelix.la libtest.la @CHECK_LIBS@ -lm -lrt -ldl

lib_LTLIBRARIES    = libtest.la libtestlib.la
libtest_la_SOURCES = collections.c
libtestlib_la_SOURCES = testlib.c

core_SOURCES    = core.c
list_SOURCES    = list.c
array_SOURCES   = array.c
dict_SOURCES    = dict.c
set_SOURCES     = set.c
str_SOURCES     = str.c
buffer_SOURCES  = buffer.c
resolve_SOURCES = resolve.c
lexer_SOURCES   = lexer.c
