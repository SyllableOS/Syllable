
INCLUDES = -I$(srcdir)/../../dropin $(QT_INCLUDES)

noinst_LTLIBRARIES = libkdecore.la

libkdecore_la_SOURCES = kurl.cpp krfcdate.cpp kcharsets.cpp \
			kentities.c ktempfile.cpp
libkdecore_la_LIBADD = ../../dropin/libdropin.la
libkdecore_la_LDFLAGS = $(all_libraries)
libkdecore_la_METASOURCES = AUTO

charsets_DATA = charsets
charsetsdir = $(prefix)/share/config

KDE_OPTIONS = qtonly
