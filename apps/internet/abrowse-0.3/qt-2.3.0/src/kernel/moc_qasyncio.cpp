/****************************************************************************
** QDataPump meta object code from reading C++ file 'qasyncio.h'
**
** Created: Fri Apr 20 02:01:28 2001
**      by: The Qt MOC ($Id$)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#if !defined(Q_MOC_OUTPUT_REVISION)
#define Q_MOC_OUTPUT_REVISION 9
#elif Q_MOC_OUTPUT_REVISION != 9
#error "Moc format conflict - please regenerate all moc files"
#endif

#include "qasyncio.h"
#include <qmetaobject.h>
#include <qapplication.h>



const char *QDataPump::className() const
{
    return "QDataPump";
}

QMetaObject *QDataPump::metaObj = 0;

void QDataPump::initMetaObject()
{
    if ( metaObj )
	return;
    if ( qstrcmp(QObject::className(), "QObject") != 0 )
	badSuperclassWarning("QDataPump","QObject");
    (void) staticMetaObject();
}

#ifndef QT_NO_TRANSLATION

QString QDataPump::tr(const char* s)
{
    return qApp->translate( "QDataPump", s, 0 );
}

QString QDataPump::tr(const char* s, const char * c)
{
    return qApp->translate( "QDataPump", s, c );
}

#endif // QT_NO_TRANSLATION

QMetaObject* QDataPump::staticMetaObject()
{
    if ( metaObj )
	return metaObj;
    (void) QObject::staticMetaObject();
#ifndef QT_NO_PROPERTIES
#endif // QT_NO_PROPERTIES
    typedef void (QDataPump::*m1_t0)();
    typedef void (QObject::*om1_t0)();
    typedef void (QDataPump::*m1_t1)();
    typedef void (QObject::*om1_t1)();
    m1_t0 v1_0 = &QDataPump::kickStart;
    om1_t0 ov1_0 = (om1_t0)v1_0;
    m1_t1 v1_1 = &QDataPump::tryToPump;
    om1_t1 ov1_1 = (om1_t1)v1_1;
    QMetaData *slot_tbl = QMetaObject::new_metadata(2);
    QMetaData::Access *slot_tbl_access = QMetaObject::new_metaaccess(2);
    slot_tbl[0].name = "kickStart()";
    slot_tbl[0].ptr = (QMember)ov1_0;
    slot_tbl_access[0] = QMetaData::Private;
    slot_tbl[1].name = "tryToPump()";
    slot_tbl[1].ptr = (QMember)ov1_1;
    slot_tbl_access[1] = QMetaData::Private;
    metaObj = QMetaObject::new_metaobject(
	"QDataPump", "QObject",
	slot_tbl, 2,
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
