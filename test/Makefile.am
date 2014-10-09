TESTS = collections
check_PROGRAMS = collections

collections_SOURCES = collections.c \
list.c \
array.c \
$(top_builddir)/src/list.h \
$(top_builddir)/src/array.h \
$(top_builddir)/src/core.h
collections_CFLAGS = @CHECK_CFLAGS@
collections_LDFLAGS = -pthread
collections_LDADD = $(top_builddir)/src/libobelix.la @CHECK_LIBS@ -lm -lrt
