TEMPLATE	= lib
CONFIG		+= qt staticlib warn_on release
win32:SOURCES	= kernel/qtmain_win.cpp
DEFINES		+= QT_DLL
TARGET		= qtmain
VERSION		= 2.20
DESTDIR		= ../lib

win32:TMAKE_CFLAGS     += -DUNICODE
win32:TMAKE_CXXFLAGS   += -DUNICODE

win32:MOC_DIR	  = tmp
win32:OBJECTS_DIR = tmp

win32:DEPENDPATH = ../include

#win32:TMAKE_CFLAGS     += -MT
#win32:TMAKE_CXXFLAGS   += -MT
