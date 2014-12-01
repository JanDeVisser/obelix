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
# AM_CFLAGS   = -Wall
LDADD       = -ldl -lm

lib_LTLIBRARIES = libobelix.la
libobelix_la_SOURCES = \
		core.c \
		list.c \
		array.c \
		dict.c \
		set.c \
		tree.c \
		data.c \
		expr.c \
		file.c \
		str.c \
		resolve.c \
		lexer.c \
		grammar.c \
		parser.c \
		instruction.c \
		script.c

bin_PROGRAMS = obelix
obelix_SOURCES = obelix.c
obelix_LDADD = libobelix.la -ldl -lm
