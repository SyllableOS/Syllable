# Configuration-derived make variables for inetutils
#
# Copyright (C) 1995, 1996, 1997 Free Software Foundation, Inc.
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

SHELL = /bin/sh


prefix = /source/Syllable/0.3.9/system/dist-root/atheos/usr
exec_prefix = ${prefix}
bindir = ${exec_prefix}/bin
includedir = ${prefix}/include
libdir = ${exec_prefix}/lib
sbindir = ${exec_prefix}/sbin
libexecdir = ${exec_prefix}/libexec
sysconfdir = ${prefix}/etc
localstatedir = ${prefix}/var
sharedstatedir = ${prefix}/com
mandir = ${prefix}/man
man1dir = $(mandir)/man1
man5dir = $(mandir)/man5
man8dir = $(mandir)/man8

INSTALL = /usr/local/bin/install -c
INSTALL_DATA = ${INSTALL} -m 644
INSTALL_PROGRAM = ${INSTALL}
AR = ar
RANLIB = ranlib
CC = gcc
YACC = bison -y
DEFS = -DHAVE_CONFIG_H
LIBS = -lresolv -lnsl 
CFLAGS = -g -O2
LDFLAGS = 
RM = /usr/local/bin/rm
MKINSTDIRS = $(SHELL) $(top_srcdir)/mkinstalldirs

LIBCURSES = -lncurses
LIBAUTH = 
LIBTERMCAP = -lncurses
LIBCRYPT = -lcrypt
LIBUTIL = -lutil
LIBGLOB = 

NCURSES_INCLUDE = 

CPPFLAGS += $(DEFS) -I$(srcdir) -I../include $(CPPFLAGS-$(<F))
LDLIBS += -L../libinetutils -linetutils $(LIBS)

RULES = $(top_srcdir)/rules.make

# Link or copy the file $$DISTFILE (a shell variable) into $(DISTDIR)/$$FILE
define _LINK_DISTFILE
  if test -d $$DISTFILE; then					 \
    echo mkdir $(DISTDIR)/$$FILE; mkdir $(DISTDIR)/$$FILE;	 \
  else								 \
    { ln 2>/dev/null $$DISTFILE $(DISTDIR)/$$FILE		 \
      && echo ln $$DISTFILE $(DISTDIR)/$$FILE; }		 \
    || { echo cp -p $$DISTFILE $(DISTDIR)/$$FILE		 \
        ; cp -p $$DISTFILE $(DISTDIR)/$$FILE; };		 \
  fi
endef

# Link or copy every file in $(DISTFILES) + Makefile.in from $(srcdir)
# and any file in $(OPT_DISTFILE) that exists in the current directory,
# into $(DISTDIR)
define LINK_DISTFILES
  @for FILE in Makefile.in $(DISTFILES); do \
    DISTFILE=$(srcdir)/$$FILE; $(_LINK_DISTFILE); \
  done
  @for FILE in $(OPT_DISTFILES) ""; do \
    test "$$FILE" -a -e "$$FILE" && { DISTFILE="$$FILE"; $(_LINK_DISTFILE); } || :; \
  done
endef


PATHDEF_CP = -DPATH_CP=\"$(bindir)/cp\"
PATHDEF_FTPLOGINMESG = -DPATH_FTPLOGINMESG=\"/etc/motd\"
PATHDEF_FTPUSERS = -DPATH_FTPUSERS=\"$(sysconfdir)/ftpusers\"
PATHDEF_FTPWELCOME = -DPATH_FTPWELCOME=\"$(sysconfdir)/ftpwelcome\"
PATHDEF_INETDCONF = -DPATH_INETDCONF=\"$(sysconfdir)/inetd.conf\"
PATHDEF_LOGCONF = -DPATH_LOGCONF=\"$(sysconfdir)/syslog.conf\"
PATHDEF_LOGIN = -DPATH_LOGIN=\"/usr/inetutils/bin/login\"
PATHDEF_LOGPID = -DPATH_LOGPID=\"$(localstatedir)/run/syslog.pid\"
PATHDEF_RLOGIN = -DPATH_RLOGIN=\"$(bindir)/rlogin\"
PATHDEF_RSH = -DPATH_RSH=\"$(bindir)/rsh\"
PATHDEF_UUCICO = -DPATH_UUCICO=\"$(libexecdir)/uucp/uucico\"
PATHDEF_HEQUIV = -DPATH_HEQUIV=\"/etc/hosts.equiv\"
