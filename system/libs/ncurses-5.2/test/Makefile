# Generated automatically from Makefile.in by configure.
# $Id$
##############################################################################
# Copyright (c) 1998,1999,2000 Free Software Foundation, Inc.                #
#                                                                            #
# Permission is hereby granted, free of charge, to any person obtaining a    #
# copy of this software and associated documentation files (the "Software"), #
# to deal in the Software without restriction, including without limitation  #
# the rights to use, copy, modify, merge, publish, distribute, distribute    #
# with modifications, sublicense, and/or sell copies of the Software, and to #
# permit persons to whom the Software is furnished to do so, subject to the  #
# following conditions:                                                      #
#                                                                            #
# The above copyright notice and this permission notice shall be included in #
# all copies or substantial portions of the Software.                        #
#                                                                            #
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR #
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   #
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    #
# THE ABOVE COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER      #
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    #
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        #
# DEALINGS IN THE SOFTWARE.                                                  #
#                                                                            #
# Except as contained in this notice, the name(s) of the above copyright     #
# holders shall not be used in advertising or otherwise to promote the sale, #
# use or other dealings in this Software without prior written               #
# authorization.                                                             #
##############################################################################
#
# Author: Thomas E. Dickey <dickey@clark.net> 1996,1997,1998
#
# Makefile for ncurses tests.

# turn off _all_ suffix rules; we'll generate our own
.SUFFIXES:

SHELL		= /bin/sh

x		= 

MODEL		= ../objects
srcdir		= .
prefix		= /source/Syllable/0.3.9/system/dist-root/atheos/usr
exec_prefix	= ${prefix}
libdir		= ${exec_prefix}/lib
includedir	= ${prefix}/include

LIBTOOL		= 

CC		= gcc
CPP		= gcc -E

CFLAGS		= -O2 
CPPFLAGS	=  -I../test -I$(srcdir)  -DNDEBUG -I. -I../include -I$(includedir) -DHAVE_CONFIG_H

CCFLAGS		= $(CPPFLAGS) $(CFLAGS)

CFLAGS_LIBTOOL	= $(CCFLAGS)
CFLAGS_NORMAL	= $(CCFLAGS)
CFLAGS_DEBUG	= $(CCFLAGS) -g -DTRACE
CFLAGS_PROFILE	= $(CCFLAGS) -pg
CFLAGS_SHARED	= $(CCFLAGS) unknown

CFLAGS_DEFAULT	= $(CFLAGS_NORMAL)

REL_VERSION	= 5.2
ABI_VERSION	= 5
LOCAL_LIBS	= ../lib/libform.a ../lib/libmenu.a ../lib/libpanel.a ../lib/libncurses.a 
MATH_LIB	= 

LD		= ld
LINK		=  $(LIBTOOL) $(CC)

LDFLAGS		=  -L../lib -L$(libdir) -lform -lmenu -lpanel -lncurses     

LDFLAGS_LIBTOOL	= $(LDFLAGS)
LDFLAGS_NORMAL	= $(LDFLAGS)
LDFLAGS_DEBUG	= $(LDFLAGS) -g
LDFLAGS_PROFILE	= $(LDFLAGS) -pg
LDFLAGS_SHARED	= $(LDFLAGS) 

LDFLAGS_DEFAULT	= $(LDFLAGS_NORMAL)

LINT		= 
LINT_OPTS	= 
LINT_LIBS	= -lform -lmenu -lpanel -lncurses 

TESTS = \
	blue$x \
	bs$x \
	cardfile$x \
	ditto$x \
	dots$x \
	filter$x \
	firework$x \
	firstlast$x \
	gdc$x \
	hanoi$x \
	hashtest$x \
	keynames$x \
	knight$x \
	lrtest$x \
	ncurses$x \
	newdemo$x \
	rain$x \
	railroad$x \
	tclock$x \
	testaddch$x \
	testcurs$x \
	testscanw$x \
	view$x \
	worm$x \
	xmas$x

all: $(TESTS)

sources:

blue$x: $(MODEL)/blue.o $(LOCAL_LIBS)
	 $(LINK) -o $@ $(MODEL)/blue.o $(LDFLAGS_DEFAULT)

bs$x: $(MODEL)/bs.o $(LOCAL_LIBS)
	 $(LINK) -o $@ $(MODEL)/bs.o $(LDFLAGS_DEFAULT)

cardfile$x: $(MODEL)/cardfile.o $(LOCAL_LIBS)
	 $(LINK) -o $@ $(MODEL)/cardfile.o $(LDFLAGS_DEFAULT)

ditto$x: $(MODEL)/ditto.o $(LOCAL_LIBS)
	 $(LINK) -o $@ $(MODEL)/ditto.o $(LDFLAGS_DEFAULT)

dots$x: $(MODEL)/dots.o $(LOCAL_LIBS)
	 $(LINK) -o $@ $(MODEL)/dots.o $(LDFLAGS_DEFAULT)

filter$x: $(MODEL)/filter.o $(LOCAL_LIBS)
	 $(LINK) -o $@ $(MODEL)/filter.o $(LDFLAGS_DEFAULT)

firework$x: $(MODEL)/firework.o $(LOCAL_LIBS)
	 $(LINK) -o $@ $(MODEL)/firework.o $(LDFLAGS_DEFAULT)

firstlast$x: $(MODEL)/firstlast.o $(LOCAL_LIBS)
	 $(LINK) -o $@ $(MODEL)/firstlast.o $(LDFLAGS_DEFAULT)

gdc$x: $(MODEL)/gdc.o $(LOCAL_LIBS)
	 $(LINK) -o $@ $(MODEL)/gdc.o $(LDFLAGS_DEFAULT)

hanoi$x: $(MODEL)/hanoi.o $(LOCAL_LIBS)
	 $(LINK) -o $@ $(MODEL)/hanoi.o $(LDFLAGS_DEFAULT)

hashtest$x: $(MODEL)/hashtest.o $(LOCAL_LIBS)
	 $(LINK) -o $@ $(MODEL)/hashtest.o $(LDFLAGS_DEFAULT)

keynames$x: $(MODEL)/keynames.o $(LOCAL_LIBS)
	 $(LINK) -o $@ $(MODEL)/keynames.o $(LDFLAGS_DEFAULT)

knight$x: $(MODEL)/knight.o $(LOCAL_LIBS)
	 $(LINK) -o $@ $(MODEL)/knight.o $(LDFLAGS_DEFAULT)

lrtest$x: $(MODEL)/lrtest.o $(LOCAL_LIBS)
	 $(LINK) -o $@ $(MODEL)/lrtest.o $(LDFLAGS_DEFAULT)

ncurses$x: $(MODEL)/ncurses.o $(LOCAL_LIBS)
	 $(LINK) -o $@ $(MODEL)/ncurses.o $(LDFLAGS_DEFAULT)

newdemo$x: $(MODEL)/newdemo.o $(LOCAL_LIBS)
	 $(LINK) -o $@ $(MODEL)/newdemo.o $(LDFLAGS_DEFAULT)

rain$x: $(MODEL)/rain.o $(LOCAL_LIBS)
	 $(LINK) -o $@ $(MODEL)/rain.o $(LDFLAGS_DEFAULT)

railroad$x: $(MODEL)/railroad.o $(LOCAL_LIBS)
	 $(LINK) -o $@ $(MODEL)/railroad.o $(LDFLAGS_DEFAULT)

tclock$x: $(MODEL)/tclock.o $(LOCAL_LIBS)
	 $(LINK) -o $@ $(MODEL)/tclock.o $(LDFLAGS_DEFAULT) $(MATH_LIB)

testaddch$x: $(MODEL)/testaddch.o $(LOCAL_LIBS)
	 $(LINK) -o $@ $(MODEL)/testaddch.o $(LDFLAGS_DEFAULT)

testcurs$x: $(MODEL)/testcurs.o $(LOCAL_LIBS)
	 $(LINK) -o $@ $(MODEL)/testcurs.o $(LDFLAGS_DEFAULT)

testscanw$x: $(MODEL)/testscanw.o $(LOCAL_LIBS)
	 $(LINK) -o $@ $(MODEL)/testscanw.o $(LDFLAGS_DEFAULT)

view$x: $(MODEL)/view.o $(LOCAL_LIBS)
	 $(LINK) -o $@ $(MODEL)/view.o $(LDFLAGS_DEFAULT)

worm$x: $(MODEL)/worm.o $(LOCAL_LIBS)
	 $(LINK) -o $@ $(MODEL)/worm.o $(LDFLAGS_DEFAULT)

xmas$x: $(MODEL)/xmas.o $(LOCAL_LIBS)
	 $(LINK) -o $@ $(MODEL)/xmas.o $(LDFLAGS_DEFAULT)

libs \
install \
install.libs \
install.test:

uninstall:
uninstall.libs:
uninstall.test:

mostlyclean ::
	-rm -f core tags TAGS *~ *.ln *.atac trace

clean :: mostlyclean
	-rm -rf *.o screendump *.lis $(TESTS) .libs

distclean :: clean
	-rm -f Makefile ncurses_cfg.h config.*

realclean :: distclean

lint:
	sh -c 'for N in $(TESTS); do echo LINT:$$N; $(LINT) $(LINT_OPTS) $(CPPFLAGS) $(srcdir)/$$N.c $(LINT_LIBS); done'

# Use this to get a list of test-programs for the standalone configure script.
echo_tests :
	@echo "$(TESTS)"
echo_suffix :
	@echo "$x"

# generated by mk-0th.awk

.SUFFIXES: .c .h .i
.c.i :
	$(CPP) $(CPPFLAGS) $< >$@
.h.i :
	$(CPP) $(CPPFLAGS) $< >$@

C_SRC = \
	$(srcdir)/blue.c \
	$(srcdir)/bs.c \
	$(srcdir)/cardfile.c \
	$(srcdir)/ditto.c \
	$(srcdir)/dots.c \
	$(srcdir)/filter.c \
	$(srcdir)/firework.c \
	$(srcdir)/firstlast.c \
	$(srcdir)/gdc.c \
	$(srcdir)/hanoi.c \
	$(srcdir)/hashtest.c \
	$(srcdir)/keynames.c \
	$(srcdir)/knight.c \
	$(srcdir)/lrtest.c \
	$(srcdir)/ncurses.c \
	$(srcdir)/newdemo.c \
	$(srcdir)/rain.c \
	$(srcdir)/railroad.c \
	$(srcdir)/tclock.c \
	$(srcdir)/testaddch.c \
	$(srcdir)/testcurs.c \
	$(srcdir)/testscanw.c \
	$(srcdir)/view.c \
	$(srcdir)/worm.c \
	$(srcdir)/xmas.c

lintlib :
	@echo no action needed

# generated by mk-1st.awk

NORMAL_OBJS = \
	../objects/blue.o \
	../objects/bs.o \
	../objects/cardfile.o \
	../objects/ditto.o \
	../objects/dots.o \
	../objects/filter.o \
	../objects/firework.o \
	../objects/firstlast.o \
	../objects/gdc.o \
	../objects/hanoi.o \
	../objects/hashtest.o \
	../objects/keynames.o \
	../objects/knight.o \
	../objects/lrtest.o \
	../objects/ncurses.o \
	../objects/newdemo.o \
	../objects/rain.o \
	../objects/railroad.o \
	../objects/tclock.o \
	../objects/testaddch.o \
	../objects/testcurs.o \
	../objects/testscanw.o \
	../objects/view.o \
	../objects/worm.o \
	../objects/xmas.o

$(NORMAL_OBJS) : ../include/ncurses_cfg.h ./test.priv.h

mostlyclean::
	-rm -f $(NORMAL_OBJS)

clean ::
	-rm -f $(NORMAL_OBJS)

# generated by mk-2nd.awk


../objects/blue.o :	$(srcdir)/blue.c \
			../include/term.h
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../test/blue.c

../objects/bs.o :	$(srcdir)/bs.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../test/bs.c

../objects/cardfile.o :	$(srcdir)/cardfile.c \
			../include/panel.h \
			../include/menu.h \
			../include/form.h
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../test/cardfile.c

../objects/ditto.o :	$(srcdir)/ditto.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../test/ditto.c

../objects/dots.o :	$(srcdir)/dots.c \
			../include/term.h
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../test/dots.c

../objects/filter.o :	$(srcdir)/filter.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../test/filter.c

../objects/firework.o :	$(srcdir)/firework.c \
			../include/term.h
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../test/firework.c

../objects/firstlast.o :	$(srcdir)/firstlast.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../test/firstlast.c

../objects/gdc.o :	$(srcdir)/gdc.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../test/gdc.c

../objects/hanoi.o :	$(srcdir)/hanoi.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../test/hanoi.c

../objects/hashtest.o :	$(srcdir)/hashtest.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../test/hashtest.c

../objects/keynames.o :	$(srcdir)/keynames.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../test/keynames.c

../objects/knight.o :	$(srcdir)/knight.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../test/knight.c

../objects/lrtest.o :	$(srcdir)/lrtest.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../test/lrtest.c

../objects/ncurses.o :	$(srcdir)/ncurses.c \
			../include/panel.h \
			../include/menu.h \
			../include/form.h
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../test/ncurses.c

../objects/newdemo.o :	$(srcdir)/newdemo.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../test/newdemo.c

../objects/rain.o :	$(srcdir)/rain.c \
			../include/term.h
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../test/rain.c

../objects/railroad.o :	$(srcdir)/railroad.c \
			../include/termcap.h
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../test/railroad.c

../objects/tclock.o :	$(srcdir)/tclock.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../test/tclock.c

../objects/testaddch.o :	$(srcdir)/testaddch.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../test/testaddch.c

../objects/testcurs.o :	$(srcdir)/testcurs.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../test/testcurs.c

../objects/testscanw.o :	$(srcdir)/testscanw.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../test/testscanw.c

../objects/view.o :	$(srcdir)/view.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../test/view.c

../objects/worm.o :	$(srcdir)/worm.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../test/worm.c

../objects/xmas.o :	$(srcdir)/xmas.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../test/xmas.c


# generated by mk-1st.awk

DEBUG_OBJS = \
	../obj_g/blue.o \
	../obj_g/bs.o \
	../obj_g/cardfile.o \
	../obj_g/ditto.o \
	../obj_g/dots.o \
	../obj_g/filter.o \
	../obj_g/firework.o \
	../obj_g/firstlast.o \
	../obj_g/gdc.o \
	../obj_g/hanoi.o \
	../obj_g/hashtest.o \
	../obj_g/keynames.o \
	../obj_g/knight.o \
	../obj_g/lrtest.o \
	../obj_g/ncurses.o \
	../obj_g/newdemo.o \
	../obj_g/rain.o \
	../obj_g/railroad.o \
	../obj_g/tclock.o \
	../obj_g/testaddch.o \
	../obj_g/testcurs.o \
	../obj_g/testscanw.o \
	../obj_g/view.o \
	../obj_g/worm.o \
	../obj_g/xmas.o

$(DEBUG_OBJS) : ../include/ncurses_cfg.h ./test.priv.h

mostlyclean::
	-rm -f $(DEBUG_OBJS)

clean ::
	-rm -f $(DEBUG_OBJS)

# generated by mk-2nd.awk


../obj_g/blue.o :	$(srcdir)/blue.c \
			../include/term.h
	cd ../obj_g; $(LIBTOOL) $(CC) $(CFLAGS_DEBUG) -c ../test/blue.c

../obj_g/bs.o :	$(srcdir)/bs.c
	cd ../obj_g; $(LIBTOOL) $(CC) $(CFLAGS_DEBUG) -c ../test/bs.c

../obj_g/cardfile.o :	$(srcdir)/cardfile.c \
			../include/panel.h \
			../include/menu.h \
			../include/form.h
	cd ../obj_g; $(LIBTOOL) $(CC) $(CFLAGS_DEBUG) -c ../test/cardfile.c

../obj_g/ditto.o :	$(srcdir)/ditto.c
	cd ../obj_g; $(LIBTOOL) $(CC) $(CFLAGS_DEBUG) -c ../test/ditto.c

../obj_g/dots.o :	$(srcdir)/dots.c \
			../include/term.h
	cd ../obj_g; $(LIBTOOL) $(CC) $(CFLAGS_DEBUG) -c ../test/dots.c

../obj_g/filter.o :	$(srcdir)/filter.c
	cd ../obj_g; $(LIBTOOL) $(CC) $(CFLAGS_DEBUG) -c ../test/filter.c

../obj_g/firework.o :	$(srcdir)/firework.c \
			../include/term.h
	cd ../obj_g; $(LIBTOOL) $(CC) $(CFLAGS_DEBUG) -c ../test/firework.c

../obj_g/firstlast.o :	$(srcdir)/firstlast.c
	cd ../obj_g; $(LIBTOOL) $(CC) $(CFLAGS_DEBUG) -c ../test/firstlast.c

../obj_g/gdc.o :	$(srcdir)/gdc.c
	cd ../obj_g; $(LIBTOOL) $(CC) $(CFLAGS_DEBUG) -c ../test/gdc.c

../obj_g/hanoi.o :	$(srcdir)/hanoi.c
	cd ../obj_g; $(LIBTOOL) $(CC) $(CFLAGS_DEBUG) -c ../test/hanoi.c

../obj_g/hashtest.o :	$(srcdir)/hashtest.c
	cd ../obj_g; $(LIBTOOL) $(CC) $(CFLAGS_DEBUG) -c ../test/hashtest.c

../obj_g/keynames.o :	$(srcdir)/keynames.c
	cd ../obj_g; $(LIBTOOL) $(CC) $(CFLAGS_DEBUG) -c ../test/keynames.c

../obj_g/knight.o :	$(srcdir)/knight.c
	cd ../obj_g; $(LIBTOOL) $(CC) $(CFLAGS_DEBUG) -c ../test/knight.c

../obj_g/lrtest.o :	$(srcdir)/lrtest.c
	cd ../obj_g; $(LIBTOOL) $(CC) $(CFLAGS_DEBUG) -c ../test/lrtest.c

../obj_g/ncurses.o :	$(srcdir)/ncurses.c \
			../include/panel.h \
			../include/menu.h \
			../include/form.h
	cd ../obj_g; $(LIBTOOL) $(CC) $(CFLAGS_DEBUG) -c ../test/ncurses.c

../obj_g/newdemo.o :	$(srcdir)/newdemo.c
	cd ../obj_g; $(LIBTOOL) $(CC) $(CFLAGS_DEBUG) -c ../test/newdemo.c

../obj_g/rain.o :	$(srcdir)/rain.c \
			../include/term.h
	cd ../obj_g; $(LIBTOOL) $(CC) $(CFLAGS_DEBUG) -c ../test/rain.c

../obj_g/railroad.o :	$(srcdir)/railroad.c \
			../include/termcap.h
	cd ../obj_g; $(LIBTOOL) $(CC) $(CFLAGS_DEBUG) -c ../test/railroad.c

../obj_g/tclock.o :	$(srcdir)/tclock.c
	cd ../obj_g; $(LIBTOOL) $(CC) $(CFLAGS_DEBUG) -c ../test/tclock.c

../obj_g/testaddch.o :	$(srcdir)/testaddch.c
	cd ../obj_g; $(LIBTOOL) $(CC) $(CFLAGS_DEBUG) -c ../test/testaddch.c

../obj_g/testcurs.o :	$(srcdir)/testcurs.c
	cd ../obj_g; $(LIBTOOL) $(CC) $(CFLAGS_DEBUG) -c ../test/testcurs.c

../obj_g/testscanw.o :	$(srcdir)/testscanw.c
	cd ../obj_g; $(LIBTOOL) $(CC) $(CFLAGS_DEBUG) -c ../test/testscanw.c

../obj_g/view.o :	$(srcdir)/view.c
	cd ../obj_g; $(LIBTOOL) $(CC) $(CFLAGS_DEBUG) -c ../test/view.c

../obj_g/worm.o :	$(srcdir)/worm.c
	cd ../obj_g; $(LIBTOOL) $(CC) $(CFLAGS_DEBUG) -c ../test/worm.c

../obj_g/xmas.o :	$(srcdir)/xmas.c
	cd ../obj_g; $(LIBTOOL) $(CC) $(CFLAGS_DEBUG) -c ../test/xmas.c

