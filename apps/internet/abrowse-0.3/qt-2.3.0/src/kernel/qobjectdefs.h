/****************************************************************************
** $Id$
**
** Macros and definitions related to QObject
**
** Created : 930419
**
** Copyright (C) 1992-2000 Trolltech AS.  All rights reserved.
**
** This file is part of the kernel module of the Qt GUI Toolkit.
**
** This file may be distributed under the terms of the Q Public License
** as defined by Trolltech AS of Norway and appearing in the file
** LICENSE.QPL included in the packaging of this file.
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** Licensees holding valid Qt Enterprise Edition or Qt Professional Edition
** licenses may use this file in accordance with the Qt Commercial License
** Agreement provided with the Software.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.trolltech.com/pricing.html or email sales@trolltech.com for
**   information about Qt Commercial License Agreements.
** See http://www.trolltech.com/qpl/ for QPL licensing information.
** See http://www.trolltech.com/gpl/ for GPL licensing information.
**
** Contact info@trolltech.com if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

#ifndef QOBJECTDEFS_H
#define QOBJECTDEFS_H

#ifndef QT_H
#include "qglobal.h"
#endif // QT_H


// The following macros are our "extensions" to C++
// They are used, strictly speaking, only by the moc.
// Only Q_OBJECT is used by the compiler.

#ifndef QT_NO_TRANSLATION
#define QT_TR_FUNCTION static QString tr(const char*); \
		       static QString tr(const char*, const char*);
#else
#define QT_TR_FUNCTION // inherit the one from QObject
#endif

#ifdef QT_MOC_CPP
 #define slots			    slots
 #define signals		    signals
 #define Q_CLASSINFO( name, value ) Q_CLASSINFO( name, value )
 #define Q_PROPERTY( text )	    Q_PROPERTY( text )
 #define Q_OVERRIDE( text )	    Q_OVERRIDE( text )
 #define Q_ENUMS( x )		    Q_ENUMS( x )
 #define Q_SETS( x )		    Q_SETS( x )
 /* tmake ignore Q_OBJECT */
 #define Q_OBJECT		    Q_OBJECT
 /* tmake ignore Q_OBJECT */
 #define Q_OBJECT_FAKE		    Q_OBJECT_FAKE

#else
 #define slots					// slots:   in class
 #define signals protected			// signals: in class
 #define emit					// emit signal
 #define Q_CLASSINFO( name, value )		// class info
 #define Q_PROPERTY( text )			// property
 #define Q_OVERRIDE( text )			// override property
 #define Q_ENUMS( x )
 #define Q_SETS( x )

/* tmake ignore Q_OBJECT */
#ifdef __ATHEOS__
// #define Q_OBJECT
 #define Q_OBJECT					\
public:							\
    QMetaObject *metaObject() const { 			\
         return staticMetaObject();			\
    }							\
    const char *className() const;			\
    static QMetaObject* staticMetaObject();		\
    QT_TR_FUNCTION					\
protected:						\
    void initMetaObject();				\
private:						\
    static QMetaObject *metaObj;

#else
 #define Q_OBJECT					\
public:							\
    QMetaObject *metaObject() const { 			\
         return staticMetaObject();			\
    }							\
    const char *className() const;			\
    static QMetaObject* staticMetaObject();		\
    QT_TR_FUNCTION					\
protected:						\
    void initMetaObject();				\
private:						\
    static QMetaObject *metaObj;
#endif // __ATHEOS__

/* tmake ignore Q_OBJECT */
#define Q_OBJECT_FAKE Q_OBJECT

#endif

// macro for naming members
#if defined(_OLD_CPP_)
#define METHOD(a)	"0""a"
#define SLOT(a)		"1""a"
#define SIGNAL(a)	"2""a"
#else
#define METHOD(a)	"0"#a
#define SLOT(a)		"1"#a
#define SIGNAL(a)	"2"#a
#endif

#define METHOD_CODE	0			// member type codes
#define SLOT_CODE	1
#define SIGNAL_CODE	2


// Forward declarations so you don't have to include files you don't need

class QObject;
class QMetaObject;
class QSignal;
class QConnection;
class QEvent;
struct QMetaData;
class QConnectionList;
class QConnectionListIt;
class QSignalDict;
class QSignalDictIt;
class QObjectList;
class QObjectListIt;
class QMemberDict;


#endif // QOBJECTDEFS_H
