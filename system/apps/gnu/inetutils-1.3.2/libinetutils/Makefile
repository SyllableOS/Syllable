# Generated automatically from Makefile.in by configure.
# Makefile for libinetutils
#
# Copyright (C) 1996, 1997 Free Software Foundation, Inc.
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

SRCS = \
__progname.c alloca.c cleansess.c daemon.c err.c forkpty.c getopt.c \
getopt1.c getusershell.c herror.c hstrerror.c iruserok.c localhost.c \
login.c login_tty.c logout.c logwtmp.c logwtmpko.c memmove.c memset.c \
openpty.c revoke.c setenv.c setsig.c snprintf.c strdup.c strerror.c \
stub_tgetent.c ttymsg.c version.c xmalloc.c xstrdup.c syslog.c

DISTFILES = $(SRCS) snprintf.h

OBJS = setsig.o ttymsg.o version.o localhost.o cleansess.o logwtmpko.o  \
        xstrdup.o revoke.o  xmalloc.o
CLEAN = libinetutils.a $(OBJS)

CPPFLAGS-logout.c = $(PATHDEF_UTMP)
CPPFLAGS-logwtmp.c = $(PATHDEF_WTMP)
CPPFLAGS-logwtmpko.c = $(PATHDEF_WTMP)
CPPFLAGS-ttymsg.c = $(PATHDEF_TTY_PFX)
CPPFLAGS-cleansess.c = $(PATHDEF_TTY_PFX)
CPPFLAGS-daemon.c = $(PATHDEF_DEVNULL)
CPPFLAGS-iruserok.c = $(PATHDEF_HEQUIV)

all: libinetutils.a
libinetutils.a: $(OBJS)
	$(AR) rcv $@ $?
	$(RANLIB) $@

include $(RULES)
