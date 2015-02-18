TESTS   = \
	tcore \
	tlist \
	tarray \
	tdict \
	tset \
	tstr \
	tbuffer \
	tresolve 

check_PROGRAMS   = \
	tcore \
	tlist \
	tarray \
	tdict \
	tset \
	tstr \
	tbuffer \
	tresolve 

AM_CPPFLAGS =   -I$(top_srcdir)/include
AM_CFLAGS =	@CHECK_CFLAGS@
AM_LDFLAGS =	-pthread
LDADD   =       $(top_srcdir)/src/lib/liboblcore.la \
		libtest.la \
		@CHECK_LIBS@ -lm -lrt -ldl

lib_LTLIBRARIES    = libtest.la libtestlib.la
libtest_la_SOURCES = collections.c
libtestlib_la_SOURCES = testlib.c

tcore_SOURCES    = tcore.c
tlist_SOURCES    = tlist.c
tarray_SOURCES   = tarray.c
tdict_SOURCES    = tdict.c
tset_SOURCES     = tset.c
tstr_SOURCES     = tstr.c
tbuffer_SOURCES  = tbuffer.c
tresolve_SOURCES = tresolve.c
