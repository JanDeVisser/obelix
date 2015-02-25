TESTS   = corelib

check_PROGRAMS   = corelib

AM_CPPFLAGS =   -I$(top_srcdir)/include
AM_CFLAGS =	@CHECK_CFLAGS@
AM_LDFLAGS =	-pthread
LDADD   =       $(top_srcdir)/src/lib/liboblcore.la \
		libtest.la \
		@CHECK_LIBS@ -lm -lrt -ldl

lib_LTLIBRARIES    = libtest.la libtestlib.la
libtest_la_SOURCES = testsuite.c
libtestlib_la_SOURCES = testlib.c

corelib_SOURCES  = tcore.c tlist.c tarray.c tdict.c tset.c tstr.c \
                   tbuffer.c tresolve.c
