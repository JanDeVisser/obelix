TESTS          = parser
check_PROGRAMS = parser

AM_CPPFLAGS =   -I$(top_srcdir)/include
AM_CFLAGS =	@CHECK_CFLAGS@
AM_LDFLAGS =	-pthread
LDADD   =       $(top_srcdir)/src/lib/liboblcore.la \
		$(top_srcdir)/src/test/libtest.la \
		$(top_srcdir)/src/types/libobltypes.la \
		$(top_srcdir)/src/parser/liboblparser.la \
		@CHECK_LIBS@ -lm -lrt -ldl

parser_SOURCES = tlexer.c tgrammar.c
