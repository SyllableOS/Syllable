# Generated automatically from Makefile.in by configure.
# Makefile for ftp
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

SRCS = cmds.c cmdtab.c ftp.c main.c ruserpass.c domacro.c
HDRS = ftp_var.h extern.h
DISTFILES = $(SRCS) $(HDRS) ftp.1

INSTALL_TARGETS = $(bindir)/ftp $(man1dir)/ftp.1
OBJS = $(SRCS:.c=.o)
LDLIBS += $(LIBGLOB) $(LIBUTIL)
CLEAN = ftp $(OBJS)

CPPFLAGS += $(PATHDEF_TMP) $(PATHDEF_BSHELL)

all: ftp
ftp: $(OBJS)

include $(RULES)
