
INCLUDES = -I$(srcdir)/.. -I$(srcdir)/../../kdesrc/kdecore -I$(srcdir)/../../kdesrc \
	   $(QT_INCLUDES)

noinst_LTLIBRARIES = libkpartsdropin.la

libkpartsdropin_la_SOURCES = part.cpp
libkpartsdropin_la_LIBADD = ../libdropin.la
libkpartsdropin_la_LDFLAGS = $(all_libraries)
libkpartsdropin_la_METASOURCES = AUTO


KDE_OPTIONS = qtonly
