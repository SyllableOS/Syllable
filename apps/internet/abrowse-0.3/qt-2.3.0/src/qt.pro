
TEMPLATE	= lib
CONFIG		+= qt warn_on release

unix:CONFIG += x11 
#xft internal
embedded:CONFIG -= x11

TARGET		= qt
embedded:TARGET	= qte
VERSION		= 2.3.0
DESTDIR		= ../lib
DLLDESTDIR	= ../bin

# All extension modules are listed here
# This is duplicated in examples.pro
MODULES_BASE	= tools kernel widgets dialogs
MODULES_PRO	= iconview workspace
MODULES_ENT	= network canvas table xml opengl
MODULES		= $$MODULES_BASE $$MODULES_PRO
enterprise:MODULES	+= $$MODULES_ENT

internal:MODULES	+= $$MODULES_ENT

CONFIG		+= $$MODULES

unix:LIBS += -lm
internal:CONFIG	+= png zlib  # Done differently in external system
embedded:CONFIG	+= png zlib ft
win32:CONFIG	+= png zlib
internal:CONFIG -= opengl
# internal:LIBS += -lpng -lz
# embedded:LIBS += -lgcc

# thread:CONFIG += opengl
# thread:TARGET = qt-mt
thread:DEFINES += QT_THREAD_SUPPORT

embedded:CONFIG -= opengl

# X11 font and render extension
xft:DEFINES += QT_XFT
xft:X11LIBS += -lXft

# Use line like this for configs specific to your work
#

# CONFIG += opengl
#internal:CONFIG += mng
internal:DEFINES += QT_NO_IMAGEIO_MNG

#unix:DEFINES    += QT_NAS_SUPPORT
#unix:LIBS	+= -laudio -lXt

internal:X11LIBS += -lICE -lSM

# Install jpegsrc.v6b.tar.gz (find with http://ftpsearch.lycos.com)
#
internal:CONFIG += jpeg
embedded:CONFIG -= jpeg
win32:CONFIG -= jpeg
jpeg:INTJPEGIU += -ljpeg
jpeg:INTJPEGIW += libjpeg.lib
DEFINESI += QT_NO_IMAGEIO_JPEG
jpeg:DEFINESI -= QT_NO_IMAGEIO_JPEG
unix:INTJPEGI += $$INTJPEGIU
win32:INTJPEGI += $$INTJPEGIW
internal:LIBS += $$INTJPEGI
internal:DEFINES += $$DEFINESI
win32:DEFINES += QT_NO_IMAGEIO_JPEG

internal:TMAKE_CXXFLAGS_EMBEDDED += -DQT_NO_QWS_VOODOO3 -DQT_NO_QWS_MATROX -DQT_NO_QWS_MACH64
internal:TMAKE_CXXFLAGS_EMBEDDED += -DQT_NO_QWS_VGA_16
#internal:TMAKE_CXXFLAGS_EMBEDDED += -DQT_NO_QWS_VFB
#internal:TMAKE_CXXFLAGS_EMBEDDED += -DQT_NO_QWS_TRANSFORMED
internal:TMAKE_CXXFLAGS_EMBEDDED += -DQT_NO_QWS_DEPTH_4
internal:TMAKE_CXXFLAGS_EMBEDDED += -DQT_NO_QWS_DEPTH_8
#internal:TMAKE_CXXFLAGS_EMBEDDED += -DQT_NO_QWS_DEPTH_16
internal:TMAKE_CXXFLAGS_EMBEDDED += -DQT_NO_QWS_DEPTH_24
#internal:TMAKE_CXXFLAGS_EMBEDDED += -DQT_NO_QWS_DEPTH_32
embedded:TMAKE_CXXFLAGS += $$TMAKE_CXXFLAGS_EMBEDDED

win32:LIBS += $$WINLIBS
unix:LIBS += $$X11LIBS
embedded:LIBS -= $$X11LIBS

# next few lines add cups support
cups:DEFINES += QT_CUPS_SUPPORT
cups:LIBS += -lcups

mng:LIBS	+= -lmng -ljpeg
#mng:LIBS	+= -L$(QTDIR)/src/3rdparty/libmng -lmng -ljpeg

#DEFINES	+= QT_NO_ASCII_CAST
#DEFINES	+= QT_NO_CAST_ASCII

unix:CONFIG	       += x11inc
unix:TMAKE_CXXFLAGS    += -DQT_FATAL_ASSERT

win32:TMAKE_CFLAGS     += -DUNICODE
win32:TMAKE_CXXFLAGS   += -DUNICODE
#win32:TMAKE_CFLAGS    += -MT
#win32:TMAKE_CXXFLAGS  += -MT

MNG_INCLUDEPATH		= 3rdparty/libmng
PNG_INCLUDEPATH		= 3rdparty/libpng
ZLIB_INCLUDEPATH	= 3rdparty/zlib
FT_INCLUDEPATH		= 3rdparty/freetype/src 3rdparty/freetype/include 3rdparty/freetype/builds/unix
#mng:INCLUDEPATH        += $$MNG_INCLUDEPATH
png:INCLUDEPATH        += $$PNG_INCLUDEPATH
zlib:INCLUDEPATH       += $$ZLIB_INCLUDEPATH
ft:INCLUDEPATH       += $$FT_INCLUDEPATH
win32:INCLUDEPATH      += tmp
win32-borland:INCLUDEPATH += kernel

win32:MOC_DIR	  = tmp
win32:OBJECTS_DIR = tmp

win32:DIALOGS_H	= ../include
win32:KERNEL_H	= ../include
win32:TOOLS_H	= ../include
win32:WIDGETS_H	= ../include
win32:OPENGL_H	= ../include
win32:NETWORK_H	= ../include
win32:CANVAS_H	= ../include
win32:TABLE_H	= ../include
win32:ICONVIEW_H	= ../include
win32:XML_H	= ../include
win32:WORKSPACE_H	= ../include

unix:DIALOGS_H	= dialogs
unix:KERNEL_H	= kernel
unix:TOOLS_H	= tools
unix:WIDGETS_H	= widgets
unix:OPENGL_H	= opengl
unix:NETWORK_H	= network
unix:CANVAS_H	= canvas
unix:TABLE_H	= table
unix:ICONVIEW_H	= iconview
unix:XML_H	= xml
unix:WORKSPACE_H	= workspace

DIALOGS_P	= dialogs
KERNEL_P	= kernel
TOOLS_P		= tools
WIDGETS_P	= widgets

win32:DEPENDPATH = ../include
unix:DEPENDPATH	= $$DIALOGS_H:$$KERNEL_H:$$TOOLS_H:$$WIDGETS_H:$$OPENGL_H:$$NETWORK_H:$$CANVAS_H:$$TABLE_H:$$ICONVIEW_H:$$XML_H:$$WORKSPACE_H

dialogs:HEADERS	+= $$DIALOGS_H/qcolordialog.h \
		  $$DIALOGS_H/qfiledialog.h \
		  $$DIALOGS_H/qfontdialog.h \
		  $$DIALOGS_H/qmessagebox.h \
		  $$DIALOGS_H/qprogressdialog.h \
		  $$DIALOGS_H/qtabdialog.h \
		  $$DIALOGS_H/qwizard.h \
		  $$DIALOGS_H/qinputdialog.h

kernel:HEADERS += $$KERNEL_H/qabstractlayout.h \
		  $$KERNEL_H/qaccel.h \
		  $$KERNEL_H/qapplication.h \
		  $$KERNEL_H/qasyncimageio.h \
		  $$KERNEL_H/qasyncio.h \
		  $$KERNEL_H/qbitmap.h \
		  $$KERNEL_H/qbrush.h \
		  $$KERNEL_H/qclipboard.h \
		  $$KERNEL_H/qcolor.h \
		  $$KERNEL_H/qconnection.h \
		  $$KERNEL_H/qcursor.h \
		  $$KERNEL_H/qdialog.h \
		  $$KERNEL_H/qdragobject.h \
		  $$KERNEL_H/qdrawutil.h \
		  $$KERNEL_H/qdropsite.h \
		  $$KERNEL_H/qevent.h \
		  $$KERNEL_H/qfocusdata.h \
		  $$KERNEL_H/qfont.h \
		  $$KERNEL_P/qfontdata_p.h \
		  $$KERNEL_H/qfontinfo.h \
		  $$KERNEL_H/qfontmetrics.h \
		  $$KERNEL_H/qguardedptr.h \
		  $$KERNEL_H/qgif.h \
		  $$KERNEL_H/qiconset.h \
		  $$KERNEL_H/qimage.h \
		  $$KERNEL_H/qkeycode.h \
		  $$KERNEL_H/qjpegio.h \
		  $$KERNEL_H/qlayout.h \
		  $$KERNEL_P/qlayoutengine_p.h \
		  $$KERNEL_H/qtranslator.h \
		  $$KERNEL_H/qmetaobject.h \
		  $$KERNEL_H/qmime.h \
		  $$KERNEL_H/qmngio.h \
		  $$KERNEL_H/qmovie.h \
		  $$KERNEL_H/qnamespace.h \
		  $$KERNEL_H/qnetworkprotocol.h \
		  $$KERNEL_H/qobject.h \
		  $$KERNEL_H/qobjectdefs.h \
		  $$KERNEL_H/qobjectdict.h \
		  $$KERNEL_H/qobjectlist.h \
		  $$KERNEL_H/qpaintdevice.h \
		  $$KERNEL_H/qpaintdevicedefs.h \
		  $$KERNEL_H/qpainter.h \
		  $$KERNEL_H/qpainter_p.h \
		  $$KERNEL_H/qpalette.h \
		  $$KERNEL_H/qpaintdevicemetrics.h \
		  $$KERNEL_H/qpen.h \
		  $$KERNEL_H/qpicture.h \
		  $$KERNEL_H/qpixmap.h \
		  $$KERNEL_H/qpixmapcache.h \
		  $$KERNEL_H/qpngio.h \
		  $$KERNEL_H/qpointarray.h \
		  $$KERNEL_H/qpoint.h \
		  $$KERNEL_H/qpolygonscanner.h \
		  $$KERNEL_H/qprinter.h \
		  $$KERNEL_H/qrect.h \
		  $$KERNEL_H/qregion.h \
		  $$KERNEL_H/qsemimodal.h \
		  $$KERNEL_H/qsessionmanager.h \
		  $$KERNEL_H/qsignal.h \
		  $$KERNEL_H/qsignalmapper.h \
		  $$KERNEL_H/qsignalslotimp.h \
		  $$KERNEL_H/qsimplerichtext.h \
		  $$KERNEL_H/qsize.h \
		  $$KERNEL_H/qsizegrip.h \
		  $$KERNEL_H/qsizepolicy.h \
		  $$KERNEL_H/qsocketnotifier.h \
		  $$KERNEL_H/qsound.h \
		  $$KERNEL_H/qstyle.h \
		  $$KERNEL_H/qstylesheet.h \
		  $$KERNEL_H/qthread.h \
		  $$KERNEL_H/qthread_p.h \
		  $$KERNEL_H/qtimer.h \
		  $$KERNEL_H/qurl.h \
		  $$KERNEL_H/qlocalfs.h \
		  $$KERNEL_H/qurloperator.h \
		  $$KERNEL_H/qurlinfo.h \
		  $$KERNEL_H/qwidget.h \
		  $$KERNEL_H/qwidgetintdict.h \
		  $$KERNEL_H/qwidgetlist.h \
		  $$KERNEL_H/qwindowdefs.h \
		  $$KERNEL_H/qwmatrix.h \
		  $$KERNEL_H/qvariant.h

tools:HEADERS +=  $$TOOLS_H/qarray.h \
		  $$TOOLS_H/qasciicache.h \
		  $$TOOLS_H/qasciidict.h \
		  $$TOOLS_H/qbig5codec.h \
		  $$TOOLS_H/qbitarray.h \
		  $$TOOLS_H/qbuffer.h \
		  $$TOOLS_H/qcache.h \
		  $$TOOLS_H/qcollection.h \
		  $$TOOLS_H/qcstring.h \
		  $$TOOLS_H/qdatastream.h \
		  $$TOOLS_H/qdatetime.h \
		  $$TOOLS_H/qdict.h \
		  $$TOOLS_H/qdir.h \
		  $$TOOLS_H/qeucjpcodec.h \
		  $$TOOLS_H/qeuckrcodec.h \
		  $$TOOLS_H/qfile.h \
		  $$TOOLS_P/qfiledefs_p.h \
		  $$TOOLS_H/qfileinfo.h \
		  $$TOOLS_H/qgarray.h \
		  $$TOOLS_H/qgbkcodec.h \
		  $$TOOLS_H/qgcache.h \
		  $$TOOLS_H/qgdict.h \
		  $$TOOLS_H/qgeneric.h \
		  $$TOOLS_H/qglist.h \
		  $$TOOLS_H/qglobal.h \
		  $$TOOLS_H/qgvector.h \
		  $$TOOLS_H/qintcache.h \
		  $$TOOLS_H/qintdict.h \
		  $$TOOLS_H/qiodevice.h \
		  $$TOOLS_H/qjiscodec.h \
		  $$TOOLS_H/qjpunicode.h \
		  $$TOOLS_H/qlist.h \
		  $$TOOLS_H/qmap.h \
		  $$TOOLS_H/qptrdict.h \
		  $$TOOLS_H/qqueue.h \
		  $$TOOLS_H/qregexp.h \
		  $$TOOLS_H/qrtlcodec.h \
		  $$TOOLS_H/qshared.h \
		  $$TOOLS_H/qsjiscodec.h \
		  $$TOOLS_H/qsortedlist.h \
		  $$TOOLS_H/qstack.h \
		  $$TOOLS_H/qstring.h \
		  $$TOOLS_H/qstringlist.h \
		  $$TOOLS_H/qstrlist.h \
		  $$TOOLS_H/qstrvec.h \
		  $$TOOLS_H/qtextcodec.h \
		  $$TOOLS_H/qtextstream.h \
		  $$TOOLS_H/qtsciicodec.h \
		  $$TOOLS_H/qutfcodec.h \
		  $$TOOLS_H/qvector.h \
	          $$TOOLS_H/qvaluelist.h

widgets:HEADERS += $$WIDGETS_H/qbuttongroup.h \
		  $$WIDGETS_H/qbutton.h \
		  $$WIDGETS_H/qcheckbox.h \
		  $$WIDGETS_H/qcdestyle.h \
		  $$WIDGETS_H/qcombobox.h \
		  $$WIDGETS_H/qcommonstyle.h \
		  $$WIDGETS_H/qdial.h \
		  $$WIDGETS_H/qframe.h \
		  $$WIDGETS_H/qgrid.h \
		  $$WIDGETS_H/qgroupbox.h \
		  $$WIDGETS_H/qhbuttongroup.h \
		  $$WIDGETS_H/qheader.h \
		  $$WIDGETS_H/qhgroupbox.h \
		  $$WIDGETS_H/qhbox.h \
		  $$WIDGETS_H/qinterlacestyle.h \
		  $$WIDGETS_H/qlabel.h \
		  $$WIDGETS_H/qlcdnumber.h \
		  $$WIDGETS_H/qlineedit.h \
		  $$WIDGETS_H/qlistbox.h \
		  $$WIDGETS_H/qlistview.h \
		  $$WIDGETS_H/qmainwindow.h \
		  $$WIDGETS_H/qmenubar.h \
		  $$WIDGETS_H/qmenudata.h \
		  $$WIDGETS_H/qmotifstyle.h \
		  $$WIDGETS_H/qmotifplusstyle.h \
		  $$WIDGETS_H/qmultilineedit.h \
		  $$WIDGETS_H/qplatinumstyle.h \
		  $$WIDGETS_H/qpopupmenu.h \
		  $$WIDGETS_H/qprogressbar.h \
		  $$WIDGETS_H/qpushbutton.h \
		  $$WIDGETS_H/qradiobutton.h \
		  $$WIDGETS_H/qrangecontrol.h \
		  $$WIDGETS_H/qscrollbar.h \
		  $$WIDGETS_H/qscrollview.h \
		  $$WIDGETS_H/qsgistyle.h \
		  $$WIDGETS_H/qslider.h \
		  $$WIDGETS_H/qspinbox.h \
		  $$WIDGETS_H/qsplitter.h \
		  $$WIDGETS_H/qstatusbar.h \
		  $$WIDGETS_H/qtabbar.h \
		  $$WIDGETS_H/qtabwidget.h \
		  $$WIDGETS_H/qtableview.h \
		  $$WIDGETS_H/qtextbrowser.h \
		  $$WIDGETS_H/qtextview.h \
		  $$WIDGETS_H/qtoolbar.h \
		  $$WIDGETS_H/qtoolbutton.h \
		  $$WIDGETS_H/qtooltip.h \
		  $$WIDGETS_H/qvalidator.h \
		  $$WIDGETS_H/qvbox.h \
		  $$WIDGETS_H/qvbuttongroup.h \
		  $$WIDGETS_H/qvgroupbox.h \
		  $$WIDGETS_H/qwhatsthis.h \
		  $$WIDGETS_H/qwidgetstack.h \
		  $$WIDGETS_H/qwindowsstyle.h \
		  $$WIDGETS_H/qaction.h

tools:WINSOURCES += tools/qdir_win.cpp \
	 	  tools/qfile_win.cpp \
		  tools/qfileinfo_win.cpp

kernel:WINSOURCES += kernel/qapplication_win.cpp \
		  kernel/qclipboard_win.cpp \
		  kernel/qcolor_win.cpp \
		  kernel/qcursor_win.cpp \
		  kernel/qdnd_win.cpp \
		  kernel/qfont_win.cpp \
		  kernel/qmime_win.cpp \
		  kernel/qpixmap_win.cpp \
		  kernel/qprinter_win.cpp \
		  kernel/qpaintdevice_win.cpp \
		  kernel/qpainter_win.cpp \
		  kernel/qregion_win.cpp \
		  kernel/qsound_win.cpp \
		  kernel/qthread_win.cpp \
		  kernel/qwidget_win.cpp \
		  kernel/qole_win.c

dialogs:WINSOURCES += dialogs/qfiledialog_win.cpp

win32:SOURCES += $$WINSOURCES

tools:UNIXSOURCES += tools/qdir_unix.cpp \
		  tools/qfile_unix.cpp \
		  tools/qfileinfo_unix.cpp

kernel:QWSSOURCES += kernel/qapplication_qws.cpp \
		  kernel/qsoundqss_qws.cpp \
		  kernel/qclipboard_qws.cpp \
		  kernel/qcolor_qws.cpp \
		  kernel/qcopchannel_qws.cpp \
		  kernel/qcursor_qws.cpp \
		  kernel/qdirectpainter_qws.cpp \
		  kernel/qdnd_qws.cpp \
		  kernel/qfont_qws.cpp \
		  kernel/qpixmap_qws.cpp \
		  kernel/qprinter_qws.cpp \
		  kernel/qpaintdevice_qws.cpp \
		  kernel/qpainter_qws.cpp \
		  kernel/qregion_qws.cpp \
		  kernel/qsound_qws.cpp \
		  kernel/qwidget_qws.cpp \
		  kernel/qgfx_qws.cpp \
		  kernel/qgfxraster_qws.cpp \
		  kernel/qfontmanager_qws.cpp \
		  kernel/qfontfactorybdf_qws.cpp \
		  kernel/qfontfactoryttf_qws.cpp \
		  kernel/qmemorymanager_qws.cpp \
		  kernel/qwscommand_qws.cpp \
		  kernel/qwsevent_qws.cpp \
		  kernel/qwindowsystem_qws.cpp \
		  kernel/qkeyboard_qws.cpp \
		  kernel/qwscursor_qws.cpp \
		  kernel/qwsdecoration_qws.cpp \
		  kernel/qwsmouse_qws.cpp \
		  kernel/qwsmanager_qws.cpp \
		  kernel/qwsdefaultdecoration_qws.cpp \
		  kernel/qwshydrodecoration_qws.cpp \
		  kernel/qwsbeosdecoration_qws.cpp \
		  kernel/qwskdedecoration_qws.cpp \
		  kernel/qwswindowsdecoration_qws.cpp \
		  kernel/qwskde2decoration_qws.cpp \
		  kernel/qwsproperty_qws.cpp \
		  kernel/qlock_qws.cpp \
		  kernel/qwsregionmanager_qws.cpp \
		  kernel/qwssocket_qws.cpp

kernel:X11SOURCES += kernel/qapplication_x11.cpp \
		  kernel/qclipboard_x11.cpp \
		  kernel/qcolor_x11.cpp \
		  kernel/qcursor_x11.cpp \
		  kernel/qdnd_x11.cpp \
		  kernel/qmotifdnd_x11.cpp \
		  kernel/qfont_x11.cpp \
		  kernel/qpixmap_x11.cpp \
		  kernel/qprinter_x11.cpp \
		  kernel/qpaintdevice_x11.cpp \
		  kernel/qpainter_x11.cpp \
		  kernel/qregion_x11.cpp \
		  kernel/qsound_x11.cpp \
		  kernel/qwidget_x11.cpp \
		  kernel/qnpsupport.cpp \
		  kernel/qwidgetcreate_x11.cpp

widgets:QWSSOURCES += $$WIDGETS_H/qcompactstyle.cpp

kernel:UNIXSOURCES += kernel/qpsprinter.cpp \
		    kernel/qthread_unix.cpp

dialogs:UNIXSOURCES += dialogs/qprintdialog.cpp

internal:X11SOURCES += $$WIDGETS_H/qcompactstyle.cpp
unix:SOURCES += $$UNIXSOURCES
unix:SOURCES += $$X11SOURCES
embedded:SOURCES -= $$X11SOURCES
embedded:SOURCES += $$QWSSOURCES

tools:SOURCES += tools/qbig5codec.cpp \
		  tools/qbitarray.cpp \
		  tools/qbuffer.cpp \
		  tools/qcollection.cpp \
		  tools/qcstring.cpp \
		  tools/qdatastream.cpp \
		  tools/qdatetime.cpp \
		  tools/qdir.cpp \
		  tools/qeucjpcodec.cpp \
		  tools/qeuckrcodec.cpp \
		  tools/qfile.cpp \
		  tools/qfileinfo.cpp \
		  tools/qgarray.cpp \
		  tools/qgbkcodec.cpp \
		  tools/qgcache.cpp \
		  tools/qgdict.cpp \
		  tools/qglist.cpp \
		  tools/qglobal.cpp \
		  tools/qgvector.cpp \
		  tools/qiodevice.cpp \
		  tools/qjiscodec.cpp \
		  tools/qjpunicode.cpp \
		  tools/qmap.cpp \
		  tools/qregexp.cpp \
		  tools/qrtlcodec.cpp \
		  tools/qsjiscodec.cpp \
		  tools/qstring.cpp \
		  tools/qstringlist.cpp \
		  tools/qtextcodec.cpp \
		  tools/qtextstream.cpp \
		  tools/qtsciicodec.cpp \
		  tools/qutfcodec.cpp

kernel:SOURCES += kernel/qabstractlayout.cpp \
		  kernel/qaccel.cpp \
		  kernel/qapplication.cpp \
		  kernel/qasyncimageio.cpp \
		  kernel/qasyncio.cpp \
		  kernel/qbitmap.cpp \
		  kernel/qclipboard.cpp \
		  kernel/qcolor.cpp \
		  kernel/qcolor_p.cpp \
		  kernel/qconnection.cpp \
		  kernel/qcursor.cpp \
		  kernel/qdialog.cpp \
		  kernel/qdragobject.cpp \
		  kernel/qdrawutil.cpp \
		  kernel/qdropsite.cpp \
		  kernel/qevent.cpp \
		  kernel/qfocusdata.cpp \
		  kernel/qfont.cpp \
		  kernel/qfontdatabase.cpp \
		  kernel/qguardedptr.cpp \
		  kernel/qiconset.cpp \
		  kernel/qimage.cpp \
		  kernel/qjpegio.cpp \
		  kernel/qlayout.cpp \
		  kernel/qlayoutengine.cpp \
		  kernel/qtranslator.cpp \
		  kernel/qmetaobject.cpp \
		  kernel/qmime.cpp \
		  kernel/qmngio.cpp \
		  kernel/qmovie.cpp \
		  kernel/qnetworkprotocol.cpp \
		  kernel/qobject.cpp \
		  kernel/qpainter.cpp \
		  kernel/qpalette.cpp \
		  kernel/qpaintdevicemetrics.cpp \
		  kernel/qpicture.cpp \
		  kernel/qpixmap.cpp \
		  kernel/qpixmapcache.cpp \
		  kernel/qpngio.cpp \
		  kernel/qpointarray.cpp \
		  kernel/qpoint.cpp \
		  kernel/qpolygonscanner.cpp \
		  kernel/qprinter.cpp \
		  kernel/qrect.cpp \
		  kernel/qregion.cpp \
		  kernel/qrichtext.cpp \
		  kernel/qsemimodal.cpp \
		  kernel/qsignal.cpp \
		  kernel/qsignalmapper.cpp \
		  kernel/qsimplerichtext.cpp \
		  kernel/qsize.cpp \
		  kernel/qsizegrip.cpp \
		  kernel/qstyle.cpp \
		  kernel/qsocketnotifier.cpp \
		  kernel/qsound.cpp \
		  kernel/qstylesheet.cpp \
		  kernel/qtimer.cpp \
		  kernel/qurl.cpp \
		  kernel/qlocalfs.cpp \
		  kernel/qurloperator.cpp \
		  kernel/qurlinfo.cpp \
		  kernel/qwidget.cpp \
		  kernel/qwmatrix.cpp \
		  kernel/qvariant.cpp

widgets:SOURCES += widgets/qbuttongroup.cpp \
		  widgets/qbutton.cpp \
		  widgets/qcdestyle.cpp \
		  widgets/qcheckbox.cpp \
		  widgets/qcombobox.cpp \
		  widgets/qcommonstyle.cpp \
		  widgets/qdial.cpp \
		  widgets/qframe.cpp \
		  widgets/qgrid.cpp \
		  widgets/qgroupbox.cpp \
		  widgets/qhbuttongroup.cpp \
		  widgets/qheader.cpp \
		  widgets/qhgroupbox.cpp \
		  widgets/qhbox.cpp \
		  widgets/qinterlacestyle.cpp \
		  widgets/qlabel.cpp \
		  widgets/qlcdnumber.cpp \
		  widgets/qlineedit.cpp \
		  widgets/qlistbox.cpp \
		  widgets/qlistview.cpp \
		  widgets/qmainwindow.cpp \
		  widgets/qmenubar.cpp \
		  widgets/qmenudata.cpp \
		  widgets/qmotifstyle.cpp \
		  widgets/qmotifplusstyle.cpp \
		  widgets/qmultilineedit.cpp \
		  widgets/qplatinumstyle.cpp \
		  widgets/qpopupmenu.cpp \
		  widgets/qprogressbar.cpp \
		  widgets/qpushbutton.cpp \
		  widgets/qradiobutton.cpp \
		  widgets/qrangecontrol.cpp \
		  widgets/qscrollbar.cpp \
		  widgets/qscrollview.cpp \
		  widgets/qsgistyle.cpp \
		  widgets/qslider.cpp \
		  widgets/qspinbox.cpp \
		  widgets/qsplitter.cpp \
		  widgets/qstatusbar.cpp \
		  widgets/qtabbar.cpp \
		  widgets/qtabwidget.cpp \
		  widgets/qtableview.cpp \
		  widgets/qtextbrowser.cpp \
		  widgets/qtextview.cpp \
		  widgets/qtoolbar.cpp \
		  widgets/qtoolbutton.cpp \
		  widgets/qtooltip.cpp \
		  widgets/qvalidator.cpp \
		  widgets/qvbox.cpp \
		  widgets/qvbuttongroup.cpp \
		  widgets/qvgroupbox.cpp \
		  widgets/qwhatsthis.cpp \
		  widgets/qwidgetstack.cpp \
		  widgets/qwindowsstyle.cpp \
		  widgets/qaction.cpp \
		  widgets/qeffects.cpp

dialogs:SOURCES += dialogs/qcolordialog.cpp \
		  dialogs/qfiledialog.cpp \
		  dialogs/qfontdialog.cpp \
		  dialogs/qmessagebox.cpp \
		  dialogs/qprogressdialog.cpp \
		  dialogs/qtabdialog.cpp \
		  dialogs/qwizard.cpp \
		  dialogs/qinputdialog.cpp

unix:HEADERS   += $$DIALOGS_H/qprintdialog.h \
		  $$KERNEL_P/qpsprinter_p.h \
		  $$KERNEL_H/qfontdatabase.h

unix:HEADERS   += $$WIDGETS_H/qcompactstyle.h
embedded:HEADERS   -= $$WIDGETS_H/qcompactstyle.h

embedded:HEADERS += $$KERNEL_H/qcopchannel_qws.h \
		  $$KERNEL_H/qfontmanager_qws.h \
		  $$KERNEL_H/qfontfactorybdf_qws.h \
		  $$KERNEL_H/qdirectpainter_qws.h \
		  $$KERNEL_H/qkeyboard_qws.h \
		  $$KERNEL_H/qsoundqss_qws.h \
		  $$KERNEL_H/qfontfactoryttf_qws.h \
		  $$KERNEL_H/qmemorymanager_qws.h \
		  $$KERNEL_H/qwsdecoration_qws.h \
		  $$KERNEL_H/qwsmanager_qws.h \
		  $$KERNEL_H/qwsdefaultdecoration_qws.h \
		  $$KERNEL_H/qwshydrodecoration_qws.h \
		  $$KERNEL_H/qwsbeosdecoration_qws.h \
		  $$KERNEL_H/qwskdedecoration_qws.h \
		  $$KERNEL_H/qwswindowsdecoration_qws.h \
		  $$KERNEL_H/qwskde2decoration_qws.h \
		  $$KERNEL_H/qgfx_qws.h \
		  $$KERNEL_H/qgfxraster_qws.h \
		  $$KERNEL_H/qgfxlinuxfb_qws.h \
		  $$KERNEL_H/qgfxvnc_qws.h \
		  $$KERNEL_H/qwindowsystem_qws.h \
		  $$KERNEL_H/qwscursor_qws.h \
		  $$KERNEL_H/qwsmouse_qws.h \
		  $$KERNEL_H/qlock_qws.h \
		  $$KERNEL_H/qwsregionmanager_qws.h \
		  $$KERNEL_H/qwsdisplay_qws.h \
		  $$KERNEL_H/qwssocket_qws.h \
		  $$WIDGETS_H/qcompactstyle.h

PNG_SOURCES	= 3rdparty/libpng/png.c \
		  3rdparty/libpng/pngerror.c \
		  3rdparty/libpng/pngget.c \
		  3rdparty/libpng/pngmem.c \
		  3rdparty/libpng/pngpread.c \
		  3rdparty/libpng/pngread.c \
		  3rdparty/libpng/pngrio.c \
		  3rdparty/libpng/pngrtran.c \
		  3rdparty/libpng/pngrutil.c \
		  3rdparty/libpng/pngset.c \
		  3rdparty/libpng/pngtrans.c \
		  3rdparty/libpng/pngwio.c \
		  3rdparty/libpng/pngwrite.c \
		  3rdparty/libpng/pngwtran.c \
		  3rdparty/libpng/pngwutil.c

ZLIB_SOURCES	= 3rdparty/zlib/adler32.c \
		  3rdparty/zlib/compress.c \
		  3rdparty/zlib/crc32.c \
		  3rdparty/zlib/deflate.c \
		  3rdparty/zlib/gzio.c \
		  3rdparty/zlib/infblock.c \
		  3rdparty/zlib/infcodes.c \
		  3rdparty/zlib/inffast.c \
		  3rdparty/zlib/inflate.c \
		  3rdparty/zlib/inftrees.c \
		  3rdparty/zlib/infutil.c \
		  3rdparty/zlib/trees.c \
		  3rdparty/zlib/uncompr.c \
		  3rdparty/zlib/zutil.c

FT_SOURCES =	\
		3rdparty/freetype/builds/unix/ftsystem.c \
		3rdparty/freetype/src/base/ftdebug.c \
		3rdparty/freetype/src/base/ftinit.c \
		3rdparty/freetype/src/base/ftbase.c \
		3rdparty/freetype/src/base/ftglyph.c \
		3rdparty/freetype/src/base/ftmm.c \
		3rdparty/freetype/src/base/ftbbox.c \
		3rdparty/freetype/src/autohint/autohint.c \
		3rdparty/freetype/src/cache/ftcache.c \
		3rdparty/freetype/src/cff/cff.c \
		3rdparty/freetype/src/cid/type1cid.c \
		3rdparty/freetype/src/psaux/psaux.c \
		3rdparty/freetype/src/psnames/psmodule.c \
		3rdparty/freetype/src/raster/raster.c \
		3rdparty/freetype/src/sfnt/sfnt.c \
		3rdparty/freetype/src/smooth/smooth.c \
		3rdparty/freetype/src/truetype/truetype.c \
		3rdparty/freetype/src/type1/type1.c \
		3rdparty/freetype/src/winfonts/winfnt.c

png:DYNSOURCES    += $$PNG_SOURCES
zlib:DYNSOURCES   += $$ZLIB_SOURCES
ft:DYNSOURCES     += $$FT_SOURCES

SOURCES += $$DYNSOURCES
static:SOURCES -= $$DYNSOURCES
propagate:SOURCES -= $$DYNSOURCES

xml:HEADERS += $$XML_H/qxml.h $$XML_H/qdom.h
xml:SOURCES += xml/qxml.cpp xml/qdom.cpp

workspace:HEADERS += $$WORKSPACE_H/qworkspace.h
workspace:SOURCES += workspace/qworkspace.cpp

canvas:HEADERS += $$CANVAS_H/qcanvas.h
canvas:SOURCES += canvas/qcanvas.cpp

iconview:HEADERS += $$ICONVIEW_H/qiconview.h
iconview:SOURCES += iconview/qiconview.cpp

table:HEADERS += $$TABLE_H/qtable.h
table:SOURCES += table/qtable.cpp

opengl:HEADERS += $$OPENGL_H/qgl.h
OPENGL_SOURCES	= opengl/qgl.cpp
unix:OPENGL_SOURCES += opengl/qgl_x11.cpp
win32:OPENGL_SOURCES += opengl/qgl_win.cpp
opengl:SOURCES    += $$OPENGL_SOURCES

network:HEADERS += $$NETWORK_H/qdns.h \
		    $$NETWORK_H/qftp.h \
		    $$NETWORK_H/qhostaddress.h \
		    $$NETWORK_H/qnetwork.h \
		    $$NETWORK_H/qserversocket.h \
		    $$NETWORK_H/qsocket.h \
		    $$NETWORK_H/qsocketdevice.h
NETWORK_SOURCES	= network/qdns.cpp \
		    network/qftp.cpp \
		    network/qhostaddress.cpp \
		    network/qnetwork.cpp \
		    network/qserversocket.cpp \
		    network/qsocket.cpp \
		    network/qsocketdevice.cpp
unix:NETWORK_SOURCES += network/qsocketdevice_unix.cpp
win32:NETWORK_SOURCES += network/qsocketdevice_win.cpp
network:SOURCES    += $$NETWORK_SOURCES


# Qt/Embedded
embedded:PRECOMPH=kernel/qt.h
QWSSUBLIBS = freetype
png:QWSSUBLIBS += png
zlib:QWSSUBLIBS += z
mng:QWSSUBLIBS += mng
jpeg:QWSSUBLIBS += jpeg
embedded:STATICSUBLIBS = $$QWSSUBLIBS
static:SUBLIBS += $$STATICSUBLIBS
embedded:MAKELIBz = $(MAKE) -C 3rdparty/zlib -f Makefile$$DASHCROSS; \
			cp 3rdparty/zlib/libz.a tmp
embedded:MAKELIBfreetype = $(MAKE) -C 3rdparty/freetype CONFIG_MK=config$$DASHCROSS.mk OBJ_DIR=../../tmp \
			    ../../tmp/libfreetype.a
embedded:MAKELIBpng = $(MAKE) -C 3rdparty/libpng \
			    -f scripts/makefile.linux$$DASHCROSS; \
			    cp 3rdparty/libpng/libpng.a tmp
embedded:MAKELIBmng = $(MAKE) -C 3rdparty/libmng \
			    -f makefiles/makefile.linux$$DASHCROSS; \
			    cp 3rdparty/libmng/libmng.a tmp
embedded:MAKELIBjpeg = $(MAKE) -C 3rdparty/jpeglib -f makefile.unix$$DASHCROSS; \
			    cp 3rdparty/jpeglib/libjpeg.a tmp

