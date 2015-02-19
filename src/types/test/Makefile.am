TESTS   = \
	tdata

check_PROGRAMS   = \
	tdata

AM_CPPFLAGS =   -I$(top_srcdir)/include
AM_CFLAGS =	@CHECK_CFLAGS@
AM_LDFLAGS =	-pthread
LDADD   =       $(top_srcdir)/src/lib/liboblcore.la \
		$(top_srcdir)/src/lib/test/libtest.la \
		$(top_srcdir)/src/types/libobltypes.la \
		@CHECK_LIBS@ -lm -lrt -ldl

tdata_SOURCES    = tdata.c
