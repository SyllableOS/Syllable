# Generated automatically from Makefile.in by configure.
# $Id$
##############################################################################
# Copyright (c) 1998,1999 Free Software Foundation, Inc.                     #
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
# Author: Thomas E. Dickey <dickey@clark.net> 1996,1997
#
#  Simple makefile for c++ window class demo

# turn off _all_ suffix rules; we'll generate our own
.SUFFIXES:

SHELL		= /bin/sh

CF_MFLAGS 	= 

x		= 

MODEL		= ../objects
DESTDIR		= 
srcdir		= .
prefix		= /source/Syllable/0.3.9/system/dist-root/atheos/usr
exec_prefix	= ${prefix}
libdir		= ${exec_prefix}/lib
includedir	= ${prefix}/include

LIBTOOL		= 

INSTALL		= /usr/local/bin/install -c
INSTALL_DATA	= ${INSTALL} -m 644

AR		= ar
AR_OPTS		= rv
RANLIB		= ranlib

CXX		= c++
CXXFLAGS	= -O2
CXXLIBS		=  -lstdc++

INCDIR		= ../include
CPPFLAGS	= -I../c++ -I$(INCDIR) -I$(srcdir) -DHAVE_CONFIG_H  -DNDEBUG -I. -I../include -I$(includedir)

CCFLAGS		= $(CPPFLAGS) $(CXXFLAGS)

CFLAGS_LIBTOOL	= $(CCFLAGS)
CFLAGS_NORMAL	= $(CCFLAGS)
CFLAGS_DEBUG	= $(CCFLAGS) -g -DTRACE
CFLAGS_PROFILE	= $(CCFLAGS) -pg
CFLAGS_SHARED	= $(CCFLAGS) unknown

CFLAGS_DEFAULT	= $(CFLAGS_NORMAL)

NCURSES_MAJOR	= 5
NCURSES_MINOR	= 2
REL_VERSION     = 5.2
ABI_VERSION	= 5

LINK		=  $(LIBTOOL) $(CXX) 

LIBROOT		= ncurses++

LIBNAME_LIBTOOL	= lib$(LIBROOT).la
LIBNAME_NORMAL	= lib$(LIBROOT).a
LIBNAME		= lib$(LIBROOT).a

LINK_FLAGS	=  -L../lib -L$(libdir) -l$(LIBROOT)

LINK_LIBTOOL	=  -L../lib -L$(libdir) ../lib/$(LIBNAME)
LINK_NORMAL	= $(LINK_FLAGS)
LINK_DEBUG	= $(LINK_FLAGS)
LINK_PROFILE	= $(LINK_FLAGS)
LINK_SHARED	= $(LINK_FLAGS)

LDFLAGS		= -L../lib -L$(libdir) -lform -lmenu -lpanel -lncurses   \
	    $(CXXLIBS)

LDFLAGS_LIBTOOL	= $(LDFLAGS)
LDFLAGS_NORMAL	= $(LDFLAGS)
LDFLAGS_DEBUG	= $(LDFLAGS) -g
LDFLAGS_PROFILE	= $(LDFLAGS) -pg
LDFLAGS_SHARED	= $(LDFLAGS) 

LDFLAGS_DEFAULT	= $(LINK_NORMAL) $(LDFLAGS_NORMAL)

AUTO_SRC	= \
		etip.h

all \
libs ::		$(AUTO_SRC) ../lib/$(LIBNAME)

all ::		demo$x

sources :	$(AUTO_SRC)

# Build a conventional library for installing, since a shared library would
# pull in all of the ncurses libraries (panel, menu, form, ncurses) as direct
# dependencies.
LIB_OBJS = \
	$(MODEL)/cursesf.o \
	$(MODEL)/cursesm.o \
	$(MODEL)/cursesw.o \
	$(MODEL)/cursespad.o \
	$(MODEL)/cursesp.o \
	$(MODEL)/cursslk.o \
	$(MODEL)/cursesapp.o \
	$(MODEL)/cursesmain.o

../lib/$(LIBNAME_NORMAL) : $(LIB_OBJS)
	$(AR) $(AR_OPTS) $@ $?
	$(RANLIB) $@

../lib/$(LIBNAME_LIBTOOL) : $(LIB_OBJS)
	cd ../lib && $(LIBTOOL) $(CXX) -o $(LIBNAME) $(LIB_OBJS:.o=.lo) \
		-rpath $(INSTALL_PREFIX)$(libdir) \
		-version-info $(NCURSES_MAJOR):$(NCURSES_MINOR)

OBJS_DEMO = $(MODEL)/demo.o

$(MODEL)/demo.o : $(srcdir)/demo.cc \
	$(cursesf_h) $(cursesm_h) $(cursesapp_h)

demo$x:	$(OBJS_DEMO) \
	../lib/$(LIBNAME)  \
	../lib/libform.a ../lib/libmenu.a ../lib/libpanel.a ../lib/libncurses.a 
	 $(LINK) -o $@ $(OBJS_DEMO) $(LDFLAGS_DEFAULT)

etip.h:	$(srcdir)/etip.h.in $(srcdir)/edit_cfg.sh
	cp $(srcdir)/etip.h.in $@
	sh $(srcdir)/edit_cfg.sh ../include/ncurses_cfg.h $@

$(DESTDIR)$(libdir) :
	$(srcdir)/../mkinstalldirs $@

install \
install.libs:: ../lib/$(LIBNAME) $(DESTDIR)$(libdir)
	$(LIBTOOL) $(INSTALL) ../lib/$(LIBNAME) $(DESTDIR)$(libdir)/$(LIBNAME)

uninstall \
uninstall.libs::
	-$(LIBTOOL) rm -f $(DESTDIR)$(libdir)/$(LIBNAME)

mostlyclean ::
	-rm -f core tags TAGS *~ *.ln *.atac trace

clean :: mostlyclean
	-$(LIBTOOL) rm -f demo$x $(AUTO_SRC) ../lib/$(LIBNAME) $(LIB_OBJS) $(LIB_OBJS:.o=.lo) $(OBJS_DEMO)
	-rm -rf .libs

distclean :: clean
	-rm -f Makefile

realclean :: distclean

###############################################################################

cursesw_h	= $(srcdir)/cursesw.h \
		  etip.h \
		  $(INCDIR)/curses.h

cursesp_h	= $(srcdir)/cursesp.h \
		  $(cursesw_h) \
		  $(INCDIR)/panel.h

cursesf_h	= $(srcdir)/cursesf.h \
		  $(cursesp_h) \
		  $(INCDIR)/form.h

cursesm_h	= $(srcdir)/cursesm.h \
		  $(cursesp_h) \
		  $(INCDIR)/menu.h

cursslk_h	= $(srcdir)/cursslk.h \
		  $(cursesw_h)

cursesapp_h	= $(srcdir)/cursesapp.h \
		  $(cursslk_h)

$(INCDIR)/form.h :
	cd ../form && $(MAKE) $@

$(INCDIR)/menu.h :
	cd ../menu && $(MAKE) $@

$(INCDIR)/panel.h :
	cd ../panel && $(MAKE) $@

###############################################################################
# The remainder of this file is automatically generated during configuration
###############################################################################

# generated by mk-0th.awk

.SUFFIXES: .c .h .i
.c.i :
	$(CPP) $(CPPFLAGS) $< >$@
.h.i :
	$(CPP) $(CPPFLAGS) $< >$@

C_SRC = \
	$(srcdir)/cursesf.c \
	$(srcdir)/cursesm.c \
	$(srcdir)/cursesp.c \
	$(srcdir)/cursesw.c \
	$(srcdir)/cursespad.c \
	$(srcdir)/cursslk.c \
	$(srcdir)/cursesapp.c \
	$(srcdir)/cursesmain.c \
	$(srcdir)/demo.c

lintlib :
	@echo no action needed

# generated by mk-1st.awk

NORMAL_OBJS = \
	../objects/cursesf.o \
	../objects/cursesm.o \
	../objects/cursesp.o \
	../objects/cursesw.o \
	../objects/cursespad.o \
	../objects/cursslk.o \
	../objects/cursesapp.o \
	../objects/cursesmain.o \
	../objects/demo.o

$(NORMAL_OBJS) : ../include/ncurses_cfg.h

mostlyclean::
	-rm -f $(NORMAL_OBJS)

clean ::
	-rm -f $(NORMAL_OBJS)

# generated by mk-2nd.awk


../objects/cursesf.o :	$(srcdir)/cursesf.cc \
			$(cursesf_h) \
			$(cursesapp_h)
	cd ../objects; $(LIBTOOL) $(CXX) $(CFLAGS_NORMAL) -c ../c++/cursesf.cc

../objects/cursesm.o :	$(srcdir)/cursesm.cc \
			$(cursesm_h) \
			$(cursesapp_h)
	cd ../objects; $(LIBTOOL) $(CXX) $(CFLAGS_NORMAL) -c ../c++/cursesm.cc

../objects/cursesp.o :	$(srcdir)/cursesp.cc \
			$(cursesp_h)
	cd ../objects; $(LIBTOOL) $(CXX) $(CFLAGS_NORMAL) -c ../c++/cursesp.cc

../objects/cursesw.o :	$(srcdir)/cursesw.cc \
			$(cursesw_h)
	cd ../objects; $(LIBTOOL) $(CXX) $(CFLAGS_NORMAL) -c ../c++/cursesw.cc

../objects/cursespad.o :	$(srcdir)/cursespad.cc \
			$(cursesw_h)
	cd ../objects; $(LIBTOOL) $(CXX) $(CFLAGS_NORMAL) -c ../c++/cursespad.cc

../objects/cursslk.o :	$(srcdir)/cursslk.cc \
			$(cursslk_h) \
			$(cursesapp_h)
	cd ../objects; $(LIBTOOL) $(CXX) $(CFLAGS_NORMAL) -c ../c++/cursslk.cc

../objects/cursesapp.o :	$(srcdir)/cursesapp.cc \
			$(cursesapp_h)
	cd ../objects; $(LIBTOOL) $(CXX) $(CFLAGS_NORMAL) -c ../c++/cursesapp.cc

../objects/cursesmain.o :	$(srcdir)/cursesmain.cc \
			$(cursesapp_h)
	cd ../objects; $(LIBTOOL) $(CXX) $(CFLAGS_NORMAL) -c ../c++/cursesmain.cc

../objects/demo.o :	$(srcdir)/demo.cc \
			$(cursesf_h) \
			$(cursesm_h) \
			$(cursesapp_h)
	cd ../objects; $(LIBTOOL) $(CXX) $(CFLAGS_NORMAL) -c ../c++/demo.cc


# generated by mk-1st.awk

DEBUG_OBJS = \
	../obj_g/cursesf.o \
	../obj_g/cursesm.o \
	../obj_g/cursesp.o \
	../obj_g/cursesw.o \
	../obj_g/cursespad.o \
	../obj_g/cursslk.o \
	../obj_g/cursesapp.o \
	../obj_g/cursesmain.o \
	../obj_g/demo.o

$(DEBUG_OBJS) : ../include/ncurses_cfg.h

mostlyclean::
	-rm -f $(DEBUG_OBJS)

clean ::
	-rm -f $(DEBUG_OBJS)

# generated by mk-2nd.awk


../obj_g/cursesf.o :	$(srcdir)/cursesf.cc \
			$(cursesf_h) \
			$(cursesapp_h)
	cd ../obj_g; $(LIBTOOL) $(CXX) $(CFLAGS_DEBUG) -c ../c++/cursesf.cc

../obj_g/cursesm.o :	$(srcdir)/cursesm.cc \
			$(cursesm_h) \
			$(cursesapp_h)
	cd ../obj_g; $(LIBTOOL) $(CXX) $(CFLAGS_DEBUG) -c ../c++/cursesm.cc

../obj_g/cursesp.o :	$(srcdir)/cursesp.cc \
			$(cursesp_h)
	cd ../obj_g; $(LIBTOOL) $(CXX) $(CFLAGS_DEBUG) -c ../c++/cursesp.cc

../obj_g/cursesw.o :	$(srcdir)/cursesw.cc \
			$(cursesw_h)
	cd ../obj_g; $(LIBTOOL) $(CXX) $(CFLAGS_DEBUG) -c ../c++/cursesw.cc

../obj_g/cursespad.o :	$(srcdir)/cursespad.cc \
			$(cursesw_h)
	cd ../obj_g; $(LIBTOOL) $(CXX) $(CFLAGS_DEBUG) -c ../c++/cursespad.cc

../obj_g/cursslk.o :	$(srcdir)/cursslk.cc \
			$(cursslk_h) \
			$(cursesapp_h)
	cd ../obj_g; $(LIBTOOL) $(CXX) $(CFLAGS_DEBUG) -c ../c++/cursslk.cc

../obj_g/cursesapp.o :	$(srcdir)/cursesapp.cc \
			$(cursesapp_h)
	cd ../obj_g; $(LIBTOOL) $(CXX) $(CFLAGS_DEBUG) -c ../c++/cursesapp.cc

../obj_g/cursesmain.o :	$(srcdir)/cursesmain.cc \
			$(cursesapp_h)
	cd ../obj_g; $(LIBTOOL) $(CXX) $(CFLAGS_DEBUG) -c ../c++/cursesmain.cc

../obj_g/demo.o :	$(srcdir)/demo.cc \
			$(cursesf_h) \
			$(cursesm_h) \
			$(cursesapp_h)
	cd ../obj_g; $(LIBTOOL) $(CXX) $(CFLAGS_DEBUG) -c ../c++/demo.cc

$(DESTDIR)$(includedir) :
	$(srcdir)/../mkinstalldirs $@

install \
install.libs \
install.includes :: $(AUTO_SRC) $(DESTDIR)$(includedir) \
		$(srcdir)/cursesapp.h \
		$(srcdir)/cursesf.h \
		$(srcdir)/cursesm.h \
		$(srcdir)/cursesp.h \
		$(srcdir)/cursesw.h \
		$(srcdir)/cursslk.h \
		etip.h
	@ (cd $(DESTDIR)$(includedir) && rm -f cursesapp.h) ; ../headers.sh $(INSTALL_DATA) $(DESTDIR)$(includedir) $(srcdir) $(srcdir)/cursesapp.h
	@ (cd $(DESTDIR)$(includedir) && rm -f cursesf.h) ; ../headers.sh $(INSTALL_DATA) $(DESTDIR)$(includedir) $(srcdir) $(srcdir)/cursesf.h
	@ (cd $(DESTDIR)$(includedir) && rm -f cursesm.h) ; ../headers.sh $(INSTALL_DATA) $(DESTDIR)$(includedir) $(srcdir) $(srcdir)/cursesm.h
	@ (cd $(DESTDIR)$(includedir) && rm -f cursesp.h) ; ../headers.sh $(INSTALL_DATA) $(DESTDIR)$(includedir) $(srcdir) $(srcdir)/cursesp.h
	@ (cd $(DESTDIR)$(includedir) && rm -f cursesw.h) ; ../headers.sh $(INSTALL_DATA) $(DESTDIR)$(includedir) $(srcdir) $(srcdir)/cursesw.h
	@ (cd $(DESTDIR)$(includedir) && rm -f cursslk.h) ; ../headers.sh $(INSTALL_DATA) $(DESTDIR)$(includedir) $(srcdir) $(srcdir)/cursslk.h
	@ (cd $(DESTDIR)$(includedir) && rm -f etip.h) ; ../headers.sh $(INSTALL_DATA) $(DESTDIR)$(includedir) $(srcdir) etip.h

uninstall \
uninstall.libs \
uninstall.includes ::
	-@ (cd $(DESTDIR)$(includedir) && rm -f cursesapp.h)
	-@ (cd $(DESTDIR)$(includedir) && rm -f cursesf.h)
	-@ (cd $(DESTDIR)$(includedir) && rm -f cursesm.h)
	-@ (cd $(DESTDIR)$(includedir) && rm -f cursesp.h)
	-@ (cd $(DESTDIR)$(includedir) && rm -f cursesw.h)
	-@ (cd $(DESTDIR)$(includedir) && rm -f cursslk.h)
	-@ (cd $(DESTDIR)$(includedir) && rm -f etip.h)
