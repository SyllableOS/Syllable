
INCLUDES = -I$(srcdir)/.. -I$(srcdir)/../../kdesrc/kdecore -I$(srcdir)/../../kdesrc \
	   -I$(srcdir)/../../kdesrc/kio/http -I$(srcdir)/../../kdesrc/kio -I../../kdesrc/kio/http \
	   -I$(srcdir)/../../kdesrc/kssl $(QT_INCLUDES)

noinst_LTLIBRARIES = libkiodropin.la

libkiodropin_la_SOURCES = job.cpp jobclasses.cpp ktrader.cpp krun.cpp kprotocolmanager.cpp

libkiodropin_la_LIBADD = ../libdropin.la
libkiodropin_la_LDFLAGS = $(all_libraries)
libkiodropin_la_METASOURCES = AUTO


KDE_OPTIONS = qtonly
