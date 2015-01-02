#
# Makefile.am - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
#
# This file is part of Obelix.
#
# Obelix is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Obelix is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Obelix.  If not, see <http://www.gnu.org/licenses/>.

AM_CPPFLAGS = -I$(top_srcdir)/include
AM_LDFLAGS  = -rdynamic
# AM_CFLAGS   = -Werror
LDADD       = -ldl -lm -lcheck

lib_LTLIBRARIES = liboblparser.la
liboblparser_la_SOURCES = \
		datastack.c \
		lexer.c \
		grammar.c \
		grammarparser.c \
		parser.c \
		script.c \
		instruction.c \
		scriptparse.c \
		object.c \
		namespace.c
		
liboblparser_la_LIBADD = \
		$(top_srcdir)/src/lib/liboblcore.la \
		$(top_srcdir)/src/types/libobltypes.la
		