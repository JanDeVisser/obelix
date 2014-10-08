TESTS = list
check_PROGRAMS = list

list_SOURCES = list.c $(top_builddir)/src/list.h $(top_builddir)/src/core.h
list_CFLAGS = @CHECK_CFLAGS@
list_LDFLAGS = -pthread
list_LDADD = $(top_builddir)/src/libobelix.la @CHECK_LIBS@ -lm -lrt
