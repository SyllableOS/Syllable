#    This file is part of the KDE libraries
#    Copyright (C) 1997 Martin Jones (mjones@kde.org)
#              (C) 1997 Torben Weis (weis@kde.org)

#    This library is free software; you can redistribute it and/or
#    modify it under the terms of the GNU Library General Public
#    License as published by the Free Software Foundation; either
#    version 2 of the License, or (at your option) any later version.

#    This library is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#    Library General Public License for more details.

#    You should have received a copy of the GNU Library General Public License
#    along with this library; see the file COPYING.LIB.  If not, write to
#    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
#    Boston, MA 02111-1307, USA.

KDE_CXXFLAGS = $(USE_EXCEPTIONS)

AM_CPPFLAGS = -DQT_NO_COMPAT

#AM_CXXFLAGS += -DTABLE_DEBUG
#AM_CXXFLAGS += -DDEBUG_LAYOUT
#AM_CXXFLAGS += -DDEBUG_EVENTS
#AM_CXXFLAGS += -DTOKEN_DEBUG
#AM_CXXFLAGS += -DPARSER_DEBUG
#AM_CXXFLAGS += -DCACHE_DEBUG
#AM_CXXFLAGS += -DDEBUG=1

# for profiling.
# command line for line by line profiling (into seperate files):
# gprof -l -y
# AM_CXXFLAGS += -pg

SUBDIRS = misc dom css xml html rendering . ecma

noinst_LTLIBRARIES = libkhtml.la
libkhtml_la_SOURCES = khtmlview.cpp khtml_part.cpp khtml_run.cpp khtml_factory.cpp khtml_settings_atheos.cc \
		      khtml_events.cpp khtml_pagecache.cpp

libkhtml_la_METASOURCES = AUTO

noinst_HEADERS = khtmlview.h khtml_part.h khtml_events.h khtml_settings_atheos.h khtmldefaults.h

noinst_HEADERS = design.h testkhtml.h testrender.h khtml_pagecache.h

libkhtml_la_LDFLAGS = -version-info 3:0 -module -no-undefined
libkhtml_la_LIBADD = ./xml/libkhtmlxml.la ./html/libkhtmlhtml.la ./rendering/libkhtmlrender.la ./css/libkhtmlcss.la \
		     ./misc/libkhtmlmisc.la ./dom/libkhtmldom.la \
		     ../kparts/libkparts.la ../kdecore/libkdecore.la \
		     ../../dropin/khtml/libkhtmldropin.la ../../dropin/libdropin.la ../../dropin/kio/libkiodropin.la
		     

INCLUDES = -I$(srcdir)/../kjs -I$(srcdir)/../kio \
	-I$(srcdir)/java -I$(srcdir)/misc \
	-I$(srcdir)/dom -I$(srcdir)/xml -I$(srcdir)/html -I$(srcdir)/css \
	-I$(srcdir)/../../dropin \
        -I$(srcdir)/../kdecore -I$(srcdir)/../../dropin/khtml \
	-I$(srcdir)/../../dropin/kio \
        -I$(srcdir)/.. $(QT_INCLUDES)

#servicedir = $(kde_servicesdir)
#service_DATA = khtml.desktop

#rcdir = $(kde_datadir)/khtml
#rc_DATA = khtml.rc khtml_browser.rc khtml_popupmenu.rc

SRCDOC_DEST=$(kde_htmldir)/en/kdelibs/khtml

## generate lib documentation
srcdoc:
	$(mkinstalldirs) $(SRCDOC_DEST)
	kdoc -H -d $(SRCDOC_DEST) kdecore \
	         $(include_HEADERS) -lqt

khtmltags_DIST = khtmltags.gperf khtmltags.in khtmltags.h khtmltags.c
kcssprop_DIST = kcssprop.c kcssprop.h kcssprop.in

EXTRA_DIST = $(khtmltags_DIST) makeprop maketags test \
	CHANGES DESIGN README.HTMLWidget README.tags $(kcssprop_DIST)

parser:
	cd $(srcdir) && \
	perl scripts/makeattrs && \
	bash scripts/maketags && \
	bash scripts/makeprop

## test program
check_PROGRAMS 	= testkhtml testcss testrender testkhtml_static
testkhtml_SOURCES = testkhtml.cpp
testkhtml_LDADD = libkhtml.la
testkhtml_static_SOURCES = testkhtml.cpp $(libkhtml_la_SOURCES)
testkhtml_static_LDADD = $(LIB_KDEUI) $(libkhtml_la_LIBADD)
testcss_SOURCES = testcss.cpp
testcss_LDADD = libkhtml.la
testrender_SOURCES = testrender.cpp
testrender_LDADD = libkhtml.la


KDE_OPTIONS = qtonly
