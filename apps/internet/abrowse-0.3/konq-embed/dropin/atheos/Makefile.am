
INCLUDES = -I$(srcdir)/.. -I$(srcdir)/../../kdesrc/kdecore -I$(srcdir)/../../kdesrc \
	   -I$(srcdir)/../../kdesrc/kio/http -I$(srcdir)/../../kdesrc/kio -I../../kdesrc/kio/http \
	   -I$(srcdir)/../../kdesrc/kssl $(QT_INCLUDES)

noinst_LTLIBRARIES = libatheosdropin.la

libatheosdropin_la_SOURCES =	abrush.cpp acursor.cpp afont.cpp afontdatabase.cpp afontinfo.cpp afontmetrics.cpp aframe.cpp	\
				apainter.cpp apaintdevice.cpp apen.cpp apixmap.cpp awidget.cpp ascrollview.cpp aregion.cpp	\
				connector.cpp apushbutton.cpp alineedit.cpp amultilineedit.cpp alistbox.cpp			\
				acombobox.cpp acheckbox.cpp aradiobutton.cpp avariant.cpp amutex.cpp atimer.cpp
libatheosdropin_la_LIBADD = ../libdropin.la
libatheosdropin_la_LDFLAGS = $(all_libraries)
libatheosdropin_la_METASOURCES = AUTO


KDE_OPTIONS = qtonly
