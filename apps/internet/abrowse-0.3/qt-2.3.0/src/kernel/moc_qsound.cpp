/****************************************************************************
** QSound meta object code from reading C++ file 'qsound.h'
**
** Created: Fri Apr 20 02:01:55 2001
**      by: The Qt MOC ($Id$)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#if !defined(Q_MOC_OUTPUT_REVISION)
#define Q_MOC_OUTPUT_REVISION 9
#elif Q_MOC_OUTPUT_REVISION != 9
#error "Moc format conflict - please regenerate all moc files"
#endif

#include "qsound.h"
#include <qmetaobject.h>
#include <qapplication.h>



const char *QSound::className() const
{
    return "QSound";
}

QMetaObject *QSound::metaObj = 0;

void QSound::initMetaObject()
{
    if ( metaObj )
	return;
    if ( qstrcmp(QObject::className(), "QObject") != 0 )
	badSuperclassWarning("QSound","QObject");
    (void) staticMetaObject();
}

#ifndef QT_NO_TRANSLATION

QString QSound::tr(const char* s)
{
    return qApp->translate( "QSound", s, 0 );
}

QString QSound::tr(const char* s, const char * c)
{
    return qApp->translate( "QSound", s, c );
}

#endif // QT_NO_TRANSLATION

QMetaObject* QSound::staticMetaObject()
{
    if ( metaObj )
	return metaObj;
    (void) QObject::staticMetaObject();
#ifndef QT_NO_PROPERTIES
#endif // QT_NO_PROPERTIES
    typedef void (QSound::*m1_t0)();
    typedef void (QObject::*om1_t0)();
    m1_t0 v1_0 = &QSound::play;
    om1_t0 ov1_0 = (om1_t0)v1_0;
    QMetaData *slot_tbl = QMetaObject::new_metadata(1);
    QMetaData::Access *slot_tbl_access = QMetaObject::new_metaaccess(1);
    slot_tbl[0].name = "play()";
    slot_tbl[0].ptr = (QMember)ov1_0;
    slot_tbl_access[0] = QMetaData::Public;
    metaObj = QMetaObject::new_metaobject(
	"QSound", "QObject",
	slot_tbl, 1,
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


const char *QAuServer::className() const
{
    return "QAuServer";
}

QMetaObject *QAuServer::metaObj = 0;

void QAuServer::initMetaObject()
{
    if ( metaObj )
	return;
    if ( qstrcmp(QObject::className(), "QObject") != 0 )
	badSuperclassWarning("QAuServer","QObject");
    (void) staticMetaObject();
}

#ifndef QT_NO_TRANSLATION

QString QAuServer::tr(const char* s)
{
    return qApp->translate( "QAuServer", s, 0 );
}

QString QAuServer::tr(const char* s, const char * c)
{
    return qApp->translate( "QAuServer", s, c );
}

#endif // QT_NO_TRANSLATION

QMetaObject* QAuServer::staticMetaObject()
{
    if ( metaObj )
	return metaObj;
    (void) QObject::staticMetaObject();
#ifndef QT_NO_PROPERTIES
#endif // QT_NO_PROPERTIES
    QMetaData::Access *slot_tbl_access = 0;
    metaObj = QMetaObject::new_metaobject(
	"QAuServer", "QObject",
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
