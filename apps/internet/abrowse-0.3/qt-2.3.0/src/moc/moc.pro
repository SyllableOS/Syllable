TEMPLATE	= moc.t
CONFIG		= console release qtinc yacc
DEFINES		= QT_NO_CODECS QT_LITE_UNICODE
win32:DEFINES  += QT_NODLL
LEXINPUT	= moc.l
YACCINPUT	= moc.y
INCLUDEPATH	= ../../include ../tools .
DEPENDPATH	= ../../include ../tools .
OBJECTS_DIR	= .
MOCGEN		= mocgen.cpp
SOURCES		= $$MOCGEN		    \
		  ../tools/qbuffer.cpp	    \
		  ../tools/qcollection.cpp  \
		  ../tools/qcstring.cpp	    \
		  ../tools/qdatastream.cpp  \
		  ../tools/qdatetime.cpp    \
		  ../tools/qfile.cpp	    \
		  ../tools/qgarray.cpp	    \
		  ../tools/qgdict.cpp	    \
		  ../tools/qglist.cpp	    \
		  ../tools/qglobal.cpp	    \
		  ../tools/qgvector.cpp	    \
		  ../tools/qiodevice.cpp    \
		  ../tools/qregexp.cpp	    \
		  ../tools/qstring.cpp	    \
		  ../tools/qtextcodec.cpp   \
		  ../tools/qtextstream.cpp

unix:SOURCES	+= ../tools/qfile_unix.cpp

win32:SOURCES	+= ../tools/qfile_win.cpp

TARGET		= moc
