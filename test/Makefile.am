TESTS = list array dict
check_PROGRAMS = list array dict

list_SOURCES = collections.c \
list.c \
$(top_builddir)/src/list.h \
$(top_builddir)/src/core.h
list_CFLAGS = @CHECK_CFLAGS@
list_LDFLAGS = -pthread
list_LDADD = $(top_builddir)/src/libobelix.la @CHECK_LIBS@ -lm -lrt

array_SOURCES = collections.c \
array.c \
$(top_builddir)/src/array.h \
$(top_builddir)/src/list.h \
$(top_builddir)/src/core.h
array_CFLAGS = @CHECK_CFLAGS@
array_LDFLAGS = -pthread
array_LDADD = $(top_builddir)/src/libobelix.la @CHECK_LIBS@ -lm -lrt

dict_SOURCES = collections.c \
dict.c \
$(top_builddir)/src/dict.h \
$(top_builddir)/src/array.h \
$(top_builddir)/src/list.h \
$(top_builddir)/src/core.h
dict_CFLAGS = @CHECK_CFLAGS@
dict_LDFLAGS = -pthread
dict_LDADD = $(top_builddir)/src/libobelix.la @CHECK_LIBS@ -lm -lrt
