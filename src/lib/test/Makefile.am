TESTS   = corelib

check_PROGRAMS   = corelib

AM_CPPFLAGS =   -I$(top_srcdir)/include
AM_CFLAGS =	@CHECK_CFLAGS@
AM_DFLAGS =	-pthread
LDADD   =       $(top_srcdir)/src/lib/liboblcore.la \
		$(top_srcdir)/src/test/libtest.la \
		$(top_srcdir)/src/types/libobltypes.la \
		$(top_srcdir)/src/parser/liboblparser.la \
		@CHECK_LIBS@ -lm -lrt -ldl

lib_LTLIBRARIES    = libtestlib.la
libtestlib_la_SOURCES = testlib.c

corelib_SOURCES  = tcore.c tlist.c tarray.c tdict.c tset.c tstr.c \
                   tbuffer.c tresolve.c tsocket.c
