# Generated automatically from Makefile.in by configure.
# Makefile for rcp
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

SRCS = rcp.c util.c
DISTFILES = $(SRCS) extern.h rcp.1

INSTALL_TARGETS = $(bindir)/rcp $(man1dir)/rcp.1
INST_PROG_FLAGS=-o root -m 04755
OBJS = $(SRCS:.c=.o)
CLEAN = rcp $(OBJS)

CPPFLAGS += $(PATHDEF_CP) $(PATHDEF_RSH) $(PATHDEF_BSHELL)

all: rcp
rcp: $(OBJS)

include $(RULES)
