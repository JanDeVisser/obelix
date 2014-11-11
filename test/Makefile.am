TESTS = 	 core list array dict set buffer
check_PROGRAMS = core list array dict set buffer

AM_CPPFLAGS =   -I$(top_srcdir)/include
AM_CFLAGS =	@CHECK_CFLAGS@
AM_LDFLAGS =	-pthread
LDADD =		$(top_builddir)/src/libobelix.la libtest.la @CHECK_LIBS@ -lm -lrt

lib_LTLIBRARIES    = libtest.la
libtest_la_SOURCES = collections.c

core_SOURCES =	 core.c
list_SOURCES =	 list.c
array_SOURCES =	 array.c
dict_SOURCES =	 dict.c
set_SOURCES =    set.c
buffer_SOURCES = buffer.c
