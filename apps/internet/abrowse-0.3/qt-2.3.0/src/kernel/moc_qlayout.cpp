/****************************************************************************
** QGridLayout meta object code from reading C++ file 'qlayout.h'
**
** Created: Fri Apr 20 02:01:38 2001
**      by: The Qt MOC ($Id$)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#if !defined(Q_MOC_OUTPUT_REVISION)
#define Q_MOC_OUTPUT_REVISION 9
#elif Q_MOC_OUTPUT_REVISION != 9
#error "Moc format conflict - please regenerate all moc files"
#endif

#include "qlayout.h"
#include <qmetaobject.h>
#include <qapplication.h>



const char *QGridLayout::className() const
{
    return "QGridLayout";
}

QMetaObject *QGridLayout::metaObj = 0;

void QGridLayout::initMetaObject()
{
    if ( metaObj )
	return;
    if ( qstrcmp(QLayout::className(), "QLayout") != 0 )
	badSuperclassWarning("QGridLayout","QLayout");
    (void) staticMetaObject();
}

#ifndef QT_NO_TRANSLATION

QString QGridLayout::tr(const char* s)
{
    return qApp->translate( "QGridLayout", s, 0 );
}

QString QGridLayout::tr(const char* s, const char * c)
{
    return qApp->translate( "QGridLayout", s, c );
}

#endif // QT_NO_TRANSLATION

QMetaObject* QGridLayout::staticMetaObject()
{
    if ( metaObj )
	return metaObj;
    (void) QLayout::staticMetaObject();
#ifndef QT_NO_PROPERTIES
#endif // QT_NO_PROPERTIES
    QMetaData::Access *slot_tbl_access = 0;
    metaObj = QMetaObject::new_metaobject(
	"QGridLayout", "QLayout",
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


const char *QBoxLayout::className() const
{
    return "QBoxLayout";
}

QMetaObject *QBoxLayout::metaObj = 0;

void QBoxLayout::initMetaObject()
{
    if ( metaObj )
	return;
    if ( qstrcmp(QLayout::className(), "QLayout") != 0 )
	badSuperclassWarning("QBoxLayout","QLayout");
    (void) staticMetaObject();
}

#ifndef QT_NO_TRANSLATION

QString QBoxLayout::tr(const char* s)
{
    return qApp->translate( "QBoxLayout", s, 0 );
}

QString QBoxLayout::tr(const char* s, const char * c)
{
    return qApp->translate( "QBoxLayout", s, c );
}

#endif // QT_NO_TRANSLATION

QMetaObject* QBoxLayout::staticMetaObject()
{
    if ( metaObj )
	return metaObj;
    (void) QLayout::staticMetaObject();
#ifndef QT_NO_PROPERTIES
#endif // QT_NO_PROPERTIES
    QMetaData::Access *slot_tbl_access = 0;
    metaObj = QMetaObject::new_metaobject(
	"QBoxLayout", "QLayout",
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


const char *QHBoxLayout::className() const
{
    return "QHBoxLayout";
}

QMetaObject *QHBoxLayout::metaObj = 0;

void QHBoxLayout::initMetaObject()
{
    if ( metaObj )
	return;
    if ( qstrcmp(QBoxLayout::className(), "QBoxLayout") != 0 )
	badSuperclassWarning("QHBoxLayout","QBoxLayout");
    (void) staticMetaObject();
}

#ifndef QT_NO_TRANSLATION

QString QHBoxLayout::tr(const char* s)
{
    return qApp->translate( "QHBoxLayout", s, 0 );
}

QString QHBoxLayout::tr(const char* s, const char * c)
{
    return qApp->translate( "QHBoxLayout", s, c );
}

#endif // QT_NO_TRANSLATION

QMetaObject* QHBoxLayout::staticMetaObject()
{
    if ( metaObj )
	return metaObj;
    (void) QBoxLayout::staticMetaObject();
#ifndef QT_NO_PROPERTIES
#endif // QT_NO_PROPERTIES
    QMetaData::Access *slot_tbl_access = 0;
    metaObj = QMetaObject::new_metaobject(
	"QHBoxLayout", "QBoxLayout",
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


const char *QVBoxLayout::className() const
{
    return "QVBoxLayout";
}

QMetaObject *QVBoxLayout::metaObj = 0;

void QVBoxLayout::initMetaObject()
{
    if ( metaObj )
	return;
    if ( qstrcmp(QBoxLayout::className(), "QBoxLayout") != 0 )
	badSuperclassWarning("QVBoxLayout","QBoxLayout");
    (void) staticMetaObject();
}

#ifndef QT_NO_TRANSLATION

QString QVBoxLayout::tr(const char* s)
{
    return qApp->translate( "QVBoxLayout", s, 0 );
}

QString QVBoxLayout::tr(const char* s, const char * c)
{
    return qApp->translate( "QVBoxLayout", s, c );
}

#endif // QT_NO_TRANSLATION

QMetaObject* QVBoxLayout::staticMetaObject()
{
    if ( metaObj )
	return metaObj;
    (void) QBoxLayout::staticMetaObject();
#ifndef QT_NO_PROPERTIES
#endif // QT_NO_PROPERTIES
    QMetaData::Access *slot_tbl_access = 0;
    metaObj = QMetaObject::new_metaobject(
	"QVBoxLayout", "QBoxLayout",
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
