
INCLUDES = -I$(srcdir)/../../dropin -I$(srcdir)/.. -I$(srcdir)/../kdecore \
           -I$(srcdir)/../../dropin/kparts $(QT_INCLUDES)

noinst_LTLIBRARIES = libkparts.la

libkparts_la_SOURCES = event.cpp partmanager.cpp browserextension.cpp factory.cpp historyprovider.cpp
libkparts_la_LIBADD = ../../dropin/kparts/libkpartsdropin.la ../../dropin/atheos/libatheosdropin.la

libkparts_la_LDFLAGS = $(all_libraries)
#(removed by KHS) -no-undefined
libkparts_la_METASOURCES = AUTO


KDE_OPTIONS = qtonly
