INCLUDES = -I$(srcdir)/.. -I$(srcdir)/../../kdesrc/kdecore -I$(srcdir)/../../kdesrc \
	   -I$(srcdir)/../../kdesrc/khtml -I$(srcdir)/../../kdesrc/khtml/xml \
	   -I$(srcdir)/../../kdesrc/khtml/dom -I$(srcdir)/../../dropin/kio $(QT_INCLUDES)

SUBDIRS = java

noinst_LTLIBRARIES = libkhtmldropin.la

libkhtmldropin_la_SOURCES = khtml_ext.cpp
libkhtmldropin_la_LIBADD = ../libdropin.la
libkhtmldropin_la_LDFLAGS = $(all_libraries)
libkhtmldropin_la_METASOURCES = AUTO


KDE_OPTIONS = qtonly
