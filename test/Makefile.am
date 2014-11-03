TESTS = 	 core list array dict set
check_PROGRAMS = core list array dict set

AM_CFLAGS =	@CHECK_CFLAGS@
AM_LDFLAGS =	-pthread
LDADD =		$(top_builddir)/src/libobelix.la libtest.la @CHECK_LIBS@ -lm -lrt

lib_LTLIBRARIES    = libtest.la
libtest_la_SOURCES = collections.c

core_SOURCES =	core.c \
		$(top_builddir)/src/core.h

list_SOURCES =	list.c \
		$(top_builddir)/src/list.h \
		$(top_builddir)/src/core.h

array_SOURCES =	array.c \
		$(top_builddir)/src/array.h \
		$(top_builddir)/src/list.h \
		$(top_builddir)/src/core.h

dict_SOURCES =	dict.c \
		$(top_builddir)/src/dict.h \
		$(top_builddir)/src/array.h \
		$(top_builddir)/src/list.h \
		$(top_builddir)/src/core.h

set_SOURCES =	set.c \
		$(top_builddir)/src/set.h \
		$(top_builddir)/src/dict.h \
		$(top_builddir)/src/array.h \
		$(top_builddir)/src/list.h \
		$(top_builddir)/src/core.h
