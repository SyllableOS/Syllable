# Copyright (C) 1997, 1998 Free Software Foundation, Inc.
# This file is part of the GNU C Library.

# The GNU C Library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Library General Public License as
# published by the Free Software Foundation; either version 2 of the
# License, or (at your option) any later version.

# The GNU C Library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Library General Public License for more details.

# You should have received a copy of the GNU Library General Public
# License along with the GNU C Library; see the file COPYING.LIB.  If
# not, write to the Free Software Foundation, Inc.,
# 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

#
#	Makefile for iconv.
#
subdir	:= iconv

include ../Makeconfig

headers		= iconv.h gconv.h
routines	= iconv_open iconv iconv_close \
		  gconv_open gconv gconv_close gconv_db gconv_conf \
		  gconv_builtin gconv_simple
ifeq ($(elf),yes)
routines	+= gconv_dl
else
CFLAGS-gconv_db.c = -DSTATIC_GCONV
CFLAGS-gconv_simple.c = -DSTATIC_GCONV
endif

distribute	= gconv_builtin.h gconv_int.h loop.c skeleton.c

others		= iconv_prog
install-others	= $(inst_bindir)/iconv

CFLAGS-gconv_conf.c = -DGCONV_PATH='"$(gconvdir)"'

include ../Rules

$(inst_bindir)/iconv: $(objpfx)iconv_prog $(+force)
	$(do-install-program)
