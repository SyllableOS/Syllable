/****************************************************************************
** QDragObject meta object code from reading C++ file 'qdragobject.h'
**
** Created: Fri Apr 20 02:01:34 2001
**      by: The Qt MOC ($Id$)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#if !defined(Q_MOC_OUTPUT_REVISION)
#define Q_MOC_OUTPUT_REVISION 9
#elif Q_MOC_OUTPUT_REVISION != 9
#error "Moc format conflict - please regenerate all moc files"
#endif

#include "qdragobject.h"
#include <qmetaobject.h>
#include <qapplication.h>



const char *QDragObject::className() const
{
    return "QDragObject";
}

QMetaObject *QDragObject::metaObj = 0;

void QDragObject::initMetaObject()
{
    if ( metaObj )
	return;
    if ( qstrcmp(QObject::className(), "QObject") != 0 )
	badSuperclassWarning("QDragObject","QObject");
    (void) staticMetaObject();
}

#ifndef QT_NO_TRANSLATION

QString QDragObject::tr(const char* s)
{
    return qApp->translate( "QDragObject", s, 0 );
}

QString QDragObject::tr(const char* s, const char * c)
{
    return qApp->translate( "QDragObject", s, c );
}

#endif // QT_NO_TRANSLATION

QMetaObject* QDragObject::staticMetaObject()
{
    if ( metaObj )
	return metaObj;
    (void) QObject::staticMetaObject();
#ifndef QT_NO_PROPERTIES
#endif // QT_NO_PROPERTIES
    QMetaData::Access *slot_tbl_access = 0;
    metaObj = QMetaObject::new_metaobject(
	"QDragObject", "QObject",
	0, 0,
	0, 0,
#ifndef QT_NO_PROPERTIES
	0, 0,
	0, 0,
#endif // QT_NO_PROPERTIES
	0, 0 );
    metaObj->set_slot_access( slot_tbl_access );
#ifndef QT_NO_PROPERTIES
#endif // QT_NO_PROPERTIES
    return metaObj;
}


const char *QStoredDrag::className() const
{
    return "QStoredDrag";
}

QMetaObject *QStoredDrag::metaObj = 0;

void QStoredDrag::initMetaObject()
{
    if ( metaObj )
	return;
    if ( qstrcmp(QDragObject::className(), "QDragObject") != 0 )
	badSuperclassWarning("QStoredDrag","QDragObject");
    (void) staticMetaObject();
}

#ifndef QT_NO_TRANSLATION

QString QStoredDrag::tr(const char* s)
{
    return qApp->translate( "QStoredDrag", s, 0 );
}

QString QStoredDrag::tr(const char* s, const char * c)
{
    return qApp->translate( "QStoredDrag", s, c );
}

#endif // QT_NO_TRANSLATION

QMetaObject* QStoredDrag::staticMetaObject()
{
    if ( metaObj )
	return metaObj;
    (void) QDragObject::staticMetaObject();
#ifndef QT_NO_PROPERTIES
#endif // QT_NO_PROPERTIES
    QMetaData::Access *slot_tbl_access = 0;
    metaObj = QMetaObject::new_metaobject(
	"QStoredDrag", "QDragObject",
	0, 0,
	0, 0,
#ifndef QT_NO_PROPERTIES
	0, 0,
	0, 0,
#endif // QT_NO_PROPERTIES
	0, 0 );
    metaObj->set_slot_access( slot_tbl_access );
#ifndef QT_NO_PROPERTIES
#endif // QT_NO_PROPERTIES
    return metaObj;
}


const char *QTextDrag::className() const
{
    return "QTextDrag";
}

QMetaObject *QTextDrag::metaObj = 0;

void QTextDrag::initMetaObject()
{
    if ( metaObj )
	return;
    if ( qstrcmp(QDragObject::className(), "QDragObject") != 0 )
	badSuperclassWarning("QTextDrag","QDragObject");
    (void) staticMetaObject();
}

#ifndef QT_NO_TRANSLATION

QString QTextDrag::tr(const char* s)
{
    return qApp->translate( "QTextDrag", s, 0 );
}

QString QTextDrag::tr(const char* s, const char * c)
{
    return qApp->translate( "QTextDrag", s, c );
}

#endif // QT_NO_TRANSLATION

QMetaObject* QTextDrag::staticMetaObject()
{
    if ( metaObj )
	return metaObj;
    (void) QDragObject::staticMetaObject();
#ifndef QT_NO_PROPERTIES
#endif // QT_NO_PROPERTIES
    QMetaData::Access *slot_tbl_access = 0;
    metaObj = QMetaObject::new_metaobject(
	"QTextDrag", "QDragObject",
	0, 0,
	0, 0,
#ifndef QT_NO_PROPERTIES
	0, 0,
	0, 0,
#endif // QT_NO_PROPERTIES
	0, 0 );
    metaObj->set_slot_access( slot_tbl_access );
#ifndef QT_NO_PROPERTIES
#endif // QT_NO_PROPERTIES
    return metaObj;
}


const char *QImageDrag::className() const
{
    return "QImageDrag";
}

QMetaObject *QImageDrag::metaObj = 0;

void QImageDrag::initMetaObject()
{
    if ( metaObj )
	return;
    if ( qstrcmp(QDragObject::className(), "QDragObject") != 0 )
	badSuperclassWarning("QImageDrag","QDragObject");
    (void) staticMetaObject();
}

#ifndef QT_NO_TRANSLATION

QString QImageDrag::tr(const char* s)
{
    return qApp->translate( "QImageDrag", s, 0 );
}

QString QImageDrag::tr(const char* s, const char * c)
{
    return qApp->translate( "QImageDrag", s, c );
}

#endif // QT_NO_TRANSLATION

QMetaObject* QImageDrag::staticMetaObject()
{
    if ( metaObj )
	return metaObj;
    (void) QDragObject::staticMetaObject();
#ifndef QT_NO_PROPERTIES
#endif // QT_NO_PROPERTIES
    QMetaData::Access *slot_tbl_access = 0;
    metaObj = QMetaObject::new_metaobject(
	"QImageDrag", "QDragObject",
	0, 0,
	0, 0,
#ifndef QT_NO_PROPERTIES
	0, 0,
	0, 0,
#endif // QT_NO_PROPERTIES
	0, 0 );
    metaObj->set_slot_access( slot_tbl_access );
#ifndef QT_NO_PROPERTIES
#endif // QT_NO_PROPERTIES
    return metaObj;
}


const char *QUriDrag::className() const
{
    return "QUriDrag";
}

QMetaObject *QUriDrag::metaObj = 0;

void QUriDrag::initMetaObject()
{
    if ( metaObj )
	return;
    if ( qstrcmp(QStoredDrag::className(), "QStoredDrag") != 0 )
	badSuperclassWarning("QUriDrag","QStoredDrag");
    (void) staticMetaObject();
}

#ifndef QT_NO_TRANSLATION

QString QUriDrag::tr(const char* s)
{
    return qApp->translate( "QUriDrag", s, 0 );
}

QString QUriDrag::tr(const char* s, const char * c)
{
    return qApp->translate( "QUriDrag", s, c );
}

#endif // QT_NO_TRANSLATION

QMetaObject* QUriDrag::staticMetaObject()
{
    if ( metaObj )
	return metaObj;
    (void) QStoredDrag::staticMetaObject();
#ifndef QT_NO_PROPERTIES
#endif // QT_NO_PROPERTIES
    QMetaData::Access *slot_tbl_access = 0;
    metaObj = QMetaObject::new_metaobject(
	"QUriDrag", "QStoredDrag",
	0, 0,
	0, 0,
#ifndef QT_NO_PROPERTIES
	0, 0,
	0, 0,
#endif // QT_NO_PROPERTIES
	0, 0 );
    metaObj->set_slot_access( slot_tbl_access );
#ifndef QT_NO_PROPERTIES
#endif // QT_NO_PROPERTIES
    return metaObj;
}


const char *QColorDrag::className() const
{
    return "QColorDrag";
}

QMetaObject *QColorDrag::metaObj = 0;

void QColorDrag::initMetaObject()
{
    if ( metaObj )
	return;
    if ( qstrcmp(QStoredDrag::className(), "QStoredDrag") != 0 )
	badSuperclassWarning("QColorDrag","QStoredDrag");
    (void) staticMetaObject();
}

#ifndef QT_NO_TRANSLATION

QString QColorDrag::tr(const char* s)
{
    return qApp->translate( "QColorDrag", s, 0 );
}

QString QColorDrag::tr(const char* s, const char * c)
{
    return qApp->translate( "QColorDrag", s, c );
}

#endif // QT_NO_TRANSLATION

QMetaObject* QColorDrag::staticMetaObject()
{
    if ( metaObj )
	return metaObj;
    (void) QStoredDrag::staticMetaObject();
#ifndef QT_NO_PROPERTIES
#endif // QT_NO_PROPERTIES
    QMetaData::Access *slot_tbl_access = 0;
    metaObj = QMetaObject::new_metaobject(
	"QColorDrag", "QStoredDrag",
	0, 0,
	0, 0,
#ifndef QT_NO_PROPERTIES
	0, 0,
	0, 0,
#endif // QT_NO_PROPERTIES
	0, 0 );
    metaObj->set_slot_access( slot_tbl_access );
#ifndef QT_NO_PROPERTIES
#endif // QT_NO_PROPERTIES
    return metaObj;
}


const char *QDragManager::className() const
{
    return "QDragManager";
}

QMetaObject *QDragManager::metaObj = 0;

void QDragManager::initMetaObject()
{
    if ( metaObj )
	return;
    if ( qstrcmp(QObject::className(), "QObject") != 0 )
	badSuperclassWarning("QDragManager","QObject");
    (void) staticMetaObject();
}

#ifndef QT_NO_TRANSLATION

QString QDragManager::tr(const char* s)
{
    return qApp->translate( "QDragManager", s, 0 );
}

QString QDragManager::tr(const char* s, const char * c)
{
    return qApp->translate( "QDragManager", s, c );
}

#endif // QT_NO_TRANSLATION

QMetaObject* QDragManager::staticMetaObject()
{
    if ( metaObj )
	return metaObj;
    (void) QObject::staticMetaObject();
#ifndef QT_NO_PROPERTIES
#endif // QT_NO_PROPERTIES
    QMetaData::Access *slot_tbl_access = 0;
    metaObj = QMetaObject::new_metaobject(
	"QDragManager", "QObject",
	0, 0,
	0, 0,
#ifndef QT_NO_PROPERTIES
	0, 0,
	0, 0,
#endif // QT_NO_PROPERTIES
	0, 0 );
    metaObj->set_slot_access( slot_tbl_access );
#ifndef QT_NO_PROPERTIES
#endif // QT_NO_PROPERTIES
    return metaObj;
}
