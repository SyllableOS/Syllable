
INCLUDES = -I$(srcdir)/../dropin -I$(srcdir)/../kdesrc/kdecore \
	   -I$(srcdir)/../kdesrc/khtml -I$(srcdir)/../kdesrc \
	   $(QT_INCLUDES)

bin_PROGRAMS = ABrowse
bindir = $(prefix)

ABrowse_SOURCES = main.cpp mainwindow.cpp toolbar.cpp utils.cpp \
		  downloaddialog.cpp searchdialog.cpp statusbar.cpp

ABrowse_LDADD = ../kdesrc/kparts/libkparts.la \
	     ../kdesrc/khtml/libkhtml.la \
	     ../dropin/kparts/libkpartsdropin.la \
	     ../kdesrc/khtml/ecma/kjs_html.la
ABrowse_LDFLAGS = $(QT_LDFLAGS) -lqt
ABrowse_METASOURCES = AUTO

ABrowse$(EXEEXT): $(ABrowse_OBJECTS) $(ABrowse_DEPENDENCIES)
	@rm -f ABrowse$(EXEEXT)
	$(CXXLINK) $(ABrowse_LDFLAGS) $(ABrowse_OBJECTS) $(ABrowse_LDADD) $(LIBS)
	rescopy -r $@ bitmaps/*.png ../kdesrc/khtml/css/html4.css

KDE_OPTIONS = qtonly

