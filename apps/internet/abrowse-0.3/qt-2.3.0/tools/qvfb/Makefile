#############################################################################
# Automatically generated from /atheos/src/apps/abrowse/qt-2.3.0/./tools/qvfb/Makefile.in
# Build options from 
#############################################################################

QT_CXXFLAGS_OPT = -DQT_BUILTIN_GIF_READER=0 -DQT_NO_IMAGEIO_JPEG -DQT_NO_IMAGEIO_MNG -DQT_NO_SM_SUPPORT  -I$(QTDIR)/src/3rdparty/zlib -I$(QTDIR)/src/3rdparty/libpng
QT_CFLAGS_OPT   =  -I$(QTDIR)/src/3rdparty/zlib -I$(QTDIR)/src/3rdparty/libpng
QT_LIBS_OPT	=   
QT_MODULE_OBJ      = $(PNG_OBJECTS) $(ZLIB_OBJECTS) \
	    $(OBJECTS_tools) \
	    $(OBJECTS_kernel) \
	    $(OBJECTS_widgets) \
	    $(OBJECTS_xml) 

# Compiling
INTERFACE_DECL_PATH 	= .
SYSCONF_CXX		= g++
SYSCONF_CC		= gcc
DASHCROSS		= 

# Compiling with support libraries
SYSCONF_CXXFLAGS_X11	= 
SYSCONF_CXXFLAGS_QT	= -I$(QTDIR)/include -DQT_NO_PROPERTIES
SYSCONF_CXXFLAGS_OPENGL	= 

# Compiling YACC output
SYSCONF_CXXFLAGS_YACC     = -Wno-unused -Wno-parentheses

# Linking with support libraries
SYSCONF_RPATH_X11	= 
SYSCONF_RPATH_QT	= 
SYSCONF_RPATH_OPENGL	= 

# Linking with support libraries
# X11
SYSCONF_LFLAGS_X11	= 
SYSCONF_LIBS_X11	= -lm
# Qt, Qt+OpenGL
SYSCONF_LFLAGS_QT	= -L$(QTDIR)/lib
SYSCONF_LIBS_QT		= -lqt$(QT_THREAD_SUFFIX)
SYSCONF_LIBS_QT_OPENGL	= 
# OpenGL
SYSCONF_LFLAGS_OPENGL	= 
SYSCONF_LIBS_OPENGL =
# Yacc
SYSCONF_LIBS_YACC	= 

# Linking applications
SYSCONF_LINK		= g++
SYSCONF_LFLAGS		= 
SYSCONF_LIBS		= 

# Link flags for shared objects
SYSCONF_LFLAGS_SHOBJ	= -shared

# Flags for threading
SYSCONF_CFLAGS_THREAD	= -D_REENTRANT
SYSCONF_CXXFLAGS_THREAD	= -D_REENTRANT
SYSCONF_LFLAGS_THREAD	= 
SYSCONF_LIBS_THREAD	= 

# Meta-object compiler
SYSCONF_MOC		= $(QTDIR)/bin/moc

# UI compiler
SYSCONF_UIC		= $(QTDIR)/bin/uic

# Linking shared libraries
#   - Build the $(TARGET) library, eg. lib$(TARGET).so.2.2.2
#   - Place target in $(DESTDIR) - which has a trailing /
#   - Usually needs to incorporate $(VER_MAJ), $(VER_MIN) and $(VER_PATCH)
#
SYSCONF_LINK_SHLIB	= g++
SYSCONF_LINK_TARGET_SHARED	= lib$(TARGET).so.$(VER_MAJ).$(VER_MIN).$(VER_PATCH)
SYSCONF_LINK_LIB_SHARED	=  $(SYSCONF_LINK_SHLIB) -shared -Wl,-soname,lib$(TARGET).so.$(VER_MAJ) \
				     $(LFLAGS) -o $(SYSCONF_LINK_TARGET_SHARED) \
				     $(OBJECTS) $(OBJMOC) $(LIBS) && \
				 mv $(SYSCONF_LINK_TARGET_SHARED) $(DESTDIR); \
				 cd $(DESTDIR) && \
				 rm -f lib$(TARGET).so lib$(TARGET).so.$(VER_MAJ) lib$(TARGET).so.$(VER_MAJ).$(VER_MIN); \
				 ln -s $(SYSCONF_LINK_TARGET_SHARED) lib$(TARGET).so; \
				 ln -s $(SYSCONF_LINK_TARGET_SHARED) lib$(TARGET).so.$(VER_MAJ); \
				 ln -s $(SYSCONF_LINK_TARGET_SHARED) lib$(TARGET).so.$(VER_MAJ).$(VER_MIN)

# Linking static libraries
#   - Build the $(TARGET) library, eg. lib$(TARGET).a
#   - Place target in $(DESTDIR) - which has a trailing /
#
SYSCONF_AR		= ar cqs
SYSCONF_LINK_TARGET_STATIC = lib$(TARGET).a
SYSCONF_LINK_LIB_STATIC	= rm -f $(DESTDIR)$(SYSCONF_LINK_TARGET_STATIC) ; \
				 $(SYSCONF_AR) $(DESTDIR)$(SYSCONF_LINK_TARGET_STATIC) $(OBJECTS) $(OBJMOC) 
# Compiling application source
SYSCONF_CXXFLAGS	= -pipe -O2
SYSCONF_CFLAGS		= -pipe -O2
SYSCONF_LINK_LIB	= $(SYSCONF_LINK_LIB_STATIC)
SYSCONF_LINK_TARGET	= $(SYSCONF_LINK_TARGET_STATIC)
# Compiling library source
SYSCONF_CXXFLAGS_LIB	=
SYSCONF_CFLAGS_LIB	=
# Compiling shared-object source
SYSCONF_CXXFLAGS_SHOBJ	= -fPIC
SYSCONF_CFLAGS_SHOBJ	= -fPIC
# Linking Qt
SYSCONF_LIBS_QTLIB	= $(SYSCONF_CXXFLAGS_X11) $(QT_LIBS_MT) $(QT_LIBS_OPT)
# Linking Qt applications
SYSCONF_LIBS_QTAPP	= $(SYSCONF_LFLAGS_X11) $(SYSCONF_CXXFLAGS_X11) $(QT_LIBS_MT) $(QT_LIBS_OPT) $(SYSCONF_LIBS_X11)
#############################################################################

####### Compiler, tools and options

CXX	=	$(SYSCONF_CXX) $(QT_CXX_MT)
CXXFLAGS=	$(SYSCONF_CXXFLAGS_QT) $(SYSCONF_CXXFLAGS)
CC	=	$(SYSCONF_CC) $(QT_C_MT)
CFLAGS	=	$(SYSCONF_CFLAGS)
INCPATH =	-I$(QTDIR)/src/3rdparty/libpng
LFLAGS	=	$(SYSCONF_LFLAGS_QT) $(SYSCONF_RPATH_QT) $(SYSCONF_LFLAGS) $(QT_LFLAGS_MT)
LIBS	=	$(SUBLIBS) $(SYSCONF_LIBS_QT) $(SYSCONF_LIBS) $(SYSCONF_LIBS_QTAPP)
MOC	=	$(SYSCONF_MOC)
UIC	=	$(SYSCONF_UIC)

####### Target

DESTDIR = ./
VER_MAJ = 1
VER_MIN = 0
VER_PATCH = 0
TARGET	= qvfb
TARGET1 = lib$(TARGET).so.$(VER_MAJ)

####### Files

HEADERS =	qvfb.h \
		qvfbview.h \
		qvfbhdr.h \
		qvfbratedlg.h \
		qanimationwriter.h
SOURCES =	qvfb.cpp \
		qvfbview.cpp \
		qvfbratedlg.cpp \
		main.cpp \
		qanimationwriter.cpp
OBJECTS =	qvfb.o \
		qvfbview.o \
		qvfbratedlg.o \
		main.o \
		qanimationwriter.o \
		config.o
INTERFACES =    config.ui
UICDECLS =      config.h
UICIMPLS =      config.cpp
SRCMOC	=	moc_qvfb.cpp \
		moc_qvfbview.cpp \
		moc_qvfbratedlg.cpp \
		moc_config.cpp
OBJMOC	=	moc_qvfb.o \
		moc_qvfbview.o \
		moc_qvfbratedlg.o \
		moc_config.o


####### Implicit rules

.SUFFIXES: .cpp .cxx .cc .C .c

.cpp.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ $<

.cxx.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ $<

.cc.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ $<

.C.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ $<

.c.o:
	$(CC) -c $(CFLAGS) $(INCPATH) -o $@ $<

####### Build rules


all: $(DESTDIR)$(TARGET)

$(DESTDIR)$(TARGET): $(UICDECLS) $(OBJECTS) $(OBJMOC) $(SUBLIBS)
	$(SYSCONF_LINK) $(LFLAGS) -o $(DESTDIR)$(TARGET) $(OBJECTS) $(OBJMOC) $(LIBS)

moc: $(SRCMOC)

tmake:
	tmake qvfb.pro

clean:
	-rm -f $(OBJECTS) $(OBJMOC) $(SRCMOC) $(UICIMPLS) $(UICDECLS)
	-rm -f *~ core
	-rm -f allmoc.cpp

####### Extension Modules

listpromodules:
	@echo 

listallmodules:
	@echo 

listaddonpromodules:
	@echo 

listaddonentmodules:
	@echo 


REQUIRES=

####### Sub-libraries


###### Combined headers



####### Compile

qvfb.o: qvfb.cpp \
		../../include/qmenubar.h \
		../../include/qpopupmenu.h \
		../../include/qframe.h \
		../../include/qwidget.h \
		../../include/qwindowdefs.h \
		../../include/qobjectdefs.h \
		../../include/qglobal.h \
		../../include/qfeatures.h \
		../../include/qconfig.h \
		../../include/qstring.h \
		../../include/qcstring.h \
		../../include/qarray.h \
		../../include/qgarray.h \
		../../include/qshared.h \
		../../include/qnamespace.h \
		../../include/qobject.h \
		../../include/qevent.h \
		../../include/qregion.h \
		../../include/qrect.h \
		../../include/qsize.h \
		../../include/qpoint.h \
		../../include/qmime.h \
		../../include/qlist.h \
		../../include/qglist.h \
		../../include/qcollection.h \
		../../include/qstringlist.h \
		../../include/qvaluelist.h \
		../../include/qdatastream.h \
		../../include/qiodevice.h \
		../../include/qregexp.h \
		../../include/qpaintdevice.h \
		../../include/qpalette.h \
		../../include/qcolor.h \
		../../include/qbrush.h \
		../../include/qcursor.h \
		../../include/qfont.h \
		../../include/qfontmetrics.h \
		../../include/qfontinfo.h \
		../../include/qstyle.h \
		../../include/qsizepolicy.h \
		../../include/qmenudata.h \
		../../include/qiconset.h \
		../../include/qpixmap.h \
		../../include/qsignal.h \
		../../include/qapplication.h \
		../../include/qasciidict.h \
		../../include/qgdict.h \
		../../include/qtranslator.h \
		../../include/qintdict.h \
		../../include/qmessagebox.h \
		../../include/qdialog.h \
		../../include/qfiledialog.h \
		../../include/qdir.h \
		../../include/qstrlist.h \
		../../include/qfileinfo.h \
		../../include/qfile.h \
		../../include/qdatetime.h \
		../../include/qlistbox.h \
		../../include/qscrollview.h \
		../../include/qscrollbar.h \
		../../include/qrangecontrol.h \
		../../include/qdrawutil.h \
		../../include/qpainter.h \
		../../include/qpen.h \
		../../include/qpointarray.h \
		../../include/qwmatrix.h \
		../../include/qtimer.h \
		../../include/qlineedit.h \
		../../include/qlistview.h \
		../../include/qurloperator.h \
		../../include/qurl.h \
		../../include/qnetworkprotocol.h \
		../../include/qurlinfo.h \
		../../include/qdict.h \
		../../include/qqueue.h \
		../../include/qspinbox.h \
		../../include/qradiobutton.h \
		../../include/qbutton.h \
		../../include/qimage.h \
		qvfb.h \
		../../include/qmainwindow.h \
		qvfbview.h \
		qvfbratedlg.h \
		config.h

qvfbview.o: qvfbview.cpp \
		qvfbview.h \
		../../include/qscrollview.h \
		../../include/qframe.h \
		../../include/qwidget.h \
		../../include/qwindowdefs.h \
		../../include/qobjectdefs.h \
		../../include/qglobal.h \
		../../include/qfeatures.h \
		../../include/qconfig.h \
		../../include/qstring.h \
		../../include/qcstring.h \
		../../include/qarray.h \
		../../include/qgarray.h \
		../../include/qshared.h \
		../../include/qnamespace.h \
		../../include/qobject.h \
		../../include/qevent.h \
		../../include/qregion.h \
		../../include/qrect.h \
		../../include/qsize.h \
		../../include/qpoint.h \
		../../include/qmime.h \
		../../include/qlist.h \
		../../include/qglist.h \
		../../include/qcollection.h \
		../../include/qstringlist.h \
		../../include/qvaluelist.h \
		../../include/qdatastream.h \
		../../include/qiodevice.h \
		../../include/qregexp.h \
		../../include/qpaintdevice.h \
		../../include/qpalette.h \
		../../include/qcolor.h \
		../../include/qbrush.h \
		../../include/qcursor.h \
		../../include/qfont.h \
		../../include/qfontmetrics.h \
		../../include/qfontinfo.h \
		../../include/qstyle.h \
		../../include/qsizepolicy.h \
		../../include/qscrollbar.h \
		../../include/qrangecontrol.h \
		../../include/qdrawutil.h \
		../../include/qpainter.h \
		../../include/qpen.h \
		../../include/qpointarray.h \
		../../include/qwmatrix.h \
		../../include/qvfbhdr.h \
		../../include/qapplication.h \
		../../include/qasciidict.h \
		../../include/qgdict.h \
		../../include/qtranslator.h \
		../../include/qintdict.h \
		../../include/qimage.h \
		../../include/qpixmap.h \
		../../include/qstrlist.h \
		../../include/qbitmap.h \
		../../include/qtimer.h \
		qanimationwriter.h

qvfbratedlg.o: qvfbratedlg.cpp \
		../../include/qlayout.h \
		../../include/qabstractlayout.h \
		../../include/qobject.h \
		../../include/qobjectdefs.h \
		../../include/qglobal.h \
		../../include/qfeatures.h \
		../../include/qconfig.h \
		../../include/qwindowdefs.h \
		../../include/qstring.h \
		../../include/qcstring.h \
		../../include/qarray.h \
		../../include/qgarray.h \
		../../include/qshared.h \
		../../include/qnamespace.h \
		../../include/qevent.h \
		../../include/qregion.h \
		../../include/qrect.h \
		../../include/qsize.h \
		../../include/qpoint.h \
		../../include/qmime.h \
		../../include/qlist.h \
		../../include/qglist.h \
		../../include/qcollection.h \
		../../include/qstringlist.h \
		../../include/qvaluelist.h \
		../../include/qdatastream.h \
		../../include/qiodevice.h \
		../../include/qregexp.h \
		../../include/qsizepolicy.h \
		../../include/qwidget.h \
		../../include/qpaintdevice.h \
		../../include/qpalette.h \
		../../include/qcolor.h \
		../../include/qbrush.h \
		../../include/qcursor.h \
		../../include/qfont.h \
		../../include/qfontmetrics.h \
		../../include/qfontinfo.h \
		../../include/qstyle.h \
		../../include/qlabel.h \
		../../include/qframe.h \
		../../include/qslider.h \
		../../include/qrangecontrol.h \
		../../include/qpushbutton.h \
		../../include/qbutton.h \
		../../include/qiconset.h \
		../../include/qpixmap.h \
		qvfbratedlg.h \
		../../include/qdialog.h

main.o: main.cpp \
		../../include/qapplication.h \
		../../include/qwidget.h \
		../../include/qwindowdefs.h \
		../../include/qobjectdefs.h \
		../../include/qglobal.h \
		../../include/qfeatures.h \
		../../include/qconfig.h \
		../../include/qstring.h \
		../../include/qcstring.h \
		../../include/qarray.h \
		../../include/qgarray.h \
		../../include/qshared.h \
		../../include/qnamespace.h \
		../../include/qobject.h \
		../../include/qevent.h \
		../../include/qregion.h \
		../../include/qrect.h \
		../../include/qsize.h \
		../../include/qpoint.h \
		../../include/qmime.h \
		../../include/qlist.h \
		../../include/qglist.h \
		../../include/qcollection.h \
		../../include/qstringlist.h \
		../../include/qvaluelist.h \
		../../include/qdatastream.h \
		../../include/qiodevice.h \
		../../include/qregexp.h \
		../../include/qpaintdevice.h \
		../../include/qpalette.h \
		../../include/qcolor.h \
		../../include/qbrush.h \
		../../include/qcursor.h \
		../../include/qfont.h \
		../../include/qfontmetrics.h \
		../../include/qfontinfo.h \
		../../include/qstyle.h \
		../../include/qsizepolicy.h \
		../../include/qasciidict.h \
		../../include/qgdict.h \
		../../include/qtranslator.h \
		../../include/qintdict.h \
		qvfb.h \
		../../include/qmainwindow.h

qanimationwriter.o: qanimationwriter.cpp \
		qanimationwriter.h \
		../../include/qimage.h \
		../../include/qpixmap.h \
		../../include/qpaintdevice.h \
		../../include/qwindowdefs.h \
		../../include/qobjectdefs.h \
		../../include/qglobal.h \
		../../include/qfeatures.h \
		../../include/qconfig.h \
		../../include/qstring.h \
		../../include/qcstring.h \
		../../include/qarray.h \
		../../include/qgarray.h \
		../../include/qshared.h \
		../../include/qnamespace.h \
		../../include/qrect.h \
		../../include/qsize.h \
		../../include/qpoint.h \
		../../include/qcolor.h \
		../../include/qstrlist.h \
		../../include/qlist.h \
		../../include/qglist.h \
		../../include/qcollection.h \
		../../include/qdatastream.h \
		../../include/qiodevice.h \
		../../include/qstringlist.h \
		../../include/qvaluelist.h \
		../../include/qregexp.h \
		../../include/qfile.h

config.h: config.ui
	$(UIC) config.ui -o $(INTERFACE_DECL_PATH)/config.h

config.cpp: config.ui
	$(UIC) config.ui -i config.h -o config.cpp

config.o: config.cpp \
		config.h \
		config.ui

moc_qvfb.o: moc_qvfb.cpp \
		qvfb.h \
		../../include/qmainwindow.h \
		../../include/qwidget.h \
		../../include/qwindowdefs.h \
		../../include/qobjectdefs.h \
		../../include/qglobal.h \
		../../include/qfeatures.h \
		../../include/qconfig.h \
		../../include/qstring.h \
		../../include/qcstring.h \
		../../include/qarray.h \
		../../include/qgarray.h \
		../../include/qshared.h \
		../../include/qnamespace.h \
		../../include/qobject.h \
		../../include/qevent.h \
		../../include/qregion.h \
		../../include/qrect.h \
		../../include/qsize.h \
		../../include/qpoint.h \
		../../include/qmime.h \
		../../include/qlist.h \
		../../include/qglist.h \
		../../include/qcollection.h \
		../../include/qstringlist.h \
		../../include/qvaluelist.h \
		../../include/qdatastream.h \
		../../include/qiodevice.h \
		../../include/qregexp.h \
		../../include/qpaintdevice.h \
		../../include/qpalette.h \
		../../include/qcolor.h \
		../../include/qbrush.h \
		../../include/qcursor.h \
		../../include/qfont.h \
		../../include/qfontmetrics.h \
		../../include/qfontinfo.h \
		../../include/qstyle.h \
		../../include/qsizepolicy.h

moc_qvfbview.o: moc_qvfbview.cpp \
		qvfbview.h \
		../../include/qscrollview.h \
		../../include/qframe.h \
		../../include/qwidget.h \
		../../include/qwindowdefs.h \
		../../include/qobjectdefs.h \
		../../include/qglobal.h \
		../../include/qfeatures.h \
		../../include/qconfig.h \
		../../include/qstring.h \
		../../include/qcstring.h \
		../../include/qarray.h \
		../../include/qgarray.h \
		../../include/qshared.h \
		../../include/qnamespace.h \
		../../include/qobject.h \
		../../include/qevent.h \
		../../include/qregion.h \
		../../include/qrect.h \
		../../include/qsize.h \
		../../include/qpoint.h \
		../../include/qmime.h \
		../../include/qlist.h \
		../../include/qglist.h \
		../../include/qcollection.h \
		../../include/qstringlist.h \
		../../include/qvaluelist.h \
		../../include/qdatastream.h \
		../../include/qiodevice.h \
		../../include/qregexp.h \
		../../include/qpaintdevice.h \
		../../include/qpalette.h \
		../../include/qcolor.h \
		../../include/qbrush.h \
		../../include/qcursor.h \
		../../include/qfont.h \
		../../include/qfontmetrics.h \
		../../include/qfontinfo.h \
		../../include/qstyle.h \
		../../include/qsizepolicy.h \
		../../include/qscrollbar.h \
		../../include/qrangecontrol.h \
		../../include/qdrawutil.h \
		../../include/qpainter.h \
		../../include/qpen.h \
		../../include/qpointarray.h \
		../../include/qwmatrix.h

moc_qvfbratedlg.o: moc_qvfbratedlg.cpp \
		qvfbratedlg.h \
		../../include/qdialog.h \
		../../include/qwidget.h \
		../../include/qwindowdefs.h \
		../../include/qobjectdefs.h \
		../../include/qglobal.h \
		../../include/qfeatures.h \
		../../include/qconfig.h \
		../../include/qstring.h \
		../../include/qcstring.h \
		../../include/qarray.h \
		../../include/qgarray.h \
		../../include/qshared.h \
		../../include/qnamespace.h \
		../../include/qobject.h \
		../../include/qevent.h \
		../../include/qregion.h \
		../../include/qrect.h \
		../../include/qsize.h \
		../../include/qpoint.h \
		../../include/qmime.h \
		../../include/qlist.h \
		../../include/qglist.h \
		../../include/qcollection.h \
		../../include/qstringlist.h \
		../../include/qvaluelist.h \
		../../include/qdatastream.h \
		../../include/qiodevice.h \
		../../include/qregexp.h \
		../../include/qpaintdevice.h \
		../../include/qpalette.h \
		../../include/qcolor.h \
		../../include/qbrush.h \
		../../include/qcursor.h \
		../../include/qfont.h \
		../../include/qfontmetrics.h \
		../../include/qfontinfo.h \
		../../include/qstyle.h \
		../../include/qsizepolicy.h

moc_config.o: moc_config.cpp \
		config.h

moc_qvfb.cpp: qvfb.h
	$(MOC) qvfb.h -o moc_qvfb.cpp

moc_qvfbview.cpp: qvfbview.h
	$(MOC) qvfbview.h -o moc_qvfbview.cpp

moc_qvfbratedlg.cpp: qvfbratedlg.h
	$(MOC) qvfbratedlg.h -o moc_qvfbratedlg.cpp

moc_config.cpp: config.h
	$(MOC) config.h -o moc_config.cpp


