# Generated automatically from Makefile.in by configure.
# Makefile for rlogind
#
# Copyright (C) 1995, 1996 Free Software Foundation, Inc.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#

srcdir = .
top_srcdir = ..
include ../config.make

DISTFILES = rlogind.c rlogind.8

INSTALL_TARGETS = $(libexecdir)/in.rlogind $(man8dir)/rlogind.8
LDLIBS += $(LIBUTIL)
CLEAN = rlogind rlogind.o

CPPFLAGS += $(PATHDEF_LOGIN) $(PATHDEF_DEV)

all: rlogind

include $(RULES)
