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

AM_CPPFLAGS = -I$(top_srcdir)/include -D_GNU_SOURCE
AM_LDFLAGS  = -rdynamic -pthread

lib_LTLIBRARIES = liboblcore.la

liboblcore_la_SOURCES = core.c list.c array.c dict.c set.c file.c str.c \
		        resolve.c user.c logging.c socket.c thread.c \
                        typedescr.c data.c any.c wrapper.c name.c exception.c

# Disabled for now. Don't need them and expr needs to be moved to types.		
#		tree.c 
#		expr.c 

liboblcore_la_LIBADD = -lpthread -ldl -lm

SUBDIRS = . test
