TEMPLATE	= app
CONFIG		+= qt warn_on release
HEADERS		= qvfb.h qvfbview.h qvfbhdr.h qvfbratedlg.h qanimationwriter.h
SOURCES		= qvfb.cpp qvfbview.cpp qvfbratedlg.cpp \
		  main.cpp qanimationwriter.cpp
INTERFACES=config.ui
TARGET		= qvfb
INCLUDEPATH+=$(QTDIR)/src/3rdparty/libpng
DEPENDPATH=../../include
