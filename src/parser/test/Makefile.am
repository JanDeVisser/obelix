TESTS   = \
	tlexer \
	tgrammar

check_PROGRAMS   = \
	tlexer \
	tgrammar

AM_CPPFLAGS =   -I$(top_srcdir)/include
AM_CFLAGS =	@CHECK_CFLAGS@
AM_LDFLAGS =	-pthread
LDADD   =       $(top_srcdir)/src/lib/liboblcore.la \
		$(top_srcdir)/src/types/libobltypes.la \
		$(top_srcdir)/src/parser/liboblparser.la \
		@CHECK_LIBS@ -lm -lrt -ldl

tlexer_SOURCES    = tlexer.c
tgrammar_SOURCES  = tgrammar.c
