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

AM_CPPFLAGS = -DOBELIX_DATADIR=\"$(datadir)\" -I$(top_srcdir)/include
AM_LDFLAGS  = -rdynamic -pthread
# AM_CFLAGS   = -Werror
LDADD       = ../parser/liboblparser.la ../lib/liboblcore.la 

lib_LTLIBRARIES = liboblparser.la
bin_PROGRAMS = panoramix

liboblparser_la_SOURCES = \
		token.c \
		lexer.c \
		grammar.c \
		grammarparser.c \
		parser.c \
		script.c \
		closure.c \
		bytecode.c \
		boundmethod.c \
		namespace.c \
		instruction.c \
		object.c \
		stacktrace.c \
		vm.c

panoramix_SOURCES = panoramix.c

# SUBDIRS = . test
