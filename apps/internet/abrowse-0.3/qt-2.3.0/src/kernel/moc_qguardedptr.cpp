/****************************************************************************
** QGuardedPtrPrivate meta object code from reading C++ file 'qguardedptr.h'
**
** Created: Sun Jul 15 14:11:30 2001
**      by: The Qt MOC ($Id$)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#if !defined(Q_MOC_OUTPUT_REVISION)
#define Q_MOC_OUTPUT_REVISION 9
#elif Q_MOC_OUTPUT_REVISION != 9
#error "Moc format conflict - please regenerate all moc files"
#endif

#include "qguardedptr.h"
#include <qmetaobject.h>
#include <qapplication.h>



const char *QGuardedPtrPrivate::className() const
{
    return "QGuardedPtrPrivate";
}

QMetaObject *QGuardedPtrPrivate::metaObj = 0;

void QGuardedPtrPrivate::initMetaObject()
{
    if ( metaObj )
	return;
    if ( qstrcmp(QObject::className(), "QObject") != 0 )
	badSuperclassWarning("QGuardedPtrPrivate","QObject");
    (void) staticMetaObject();
}

#ifndef QT_NO_TRANSLATION

QString QGuardedPtrPrivate::tr(const char* s)
{
    return qApp->translate( "QGuardedPtrPrivate", s, 0 );
}

QString QGuardedPtrPrivate::tr(const char* s, const char * c)
{
    return qApp->translate( "QGuardedPtrPrivate", s, c );
}

#endif // QT_NO_TRANSLATION

QMetaObject* QGuardedPtrPrivate::staticMetaObject()
{
    if ( metaObj )
	return metaObj;
    (void) QObject::staticMetaObject();
#ifndef QT_NO_PROPERTIES
#endif // QT_NO_PROPERTIES
    typedef void (QGuardedPtrPrivate::*m1_t0)();
    typedef void (QObject::*om1_t0)();
    m1_t0 v1_0 = &QGuardedPtrPrivate::objectDestroyed;
    om1_t0 ov1_0 = (om1_t0)v1_0;
    QMetaData *slot_tbl = QMetaObject::new_metadata(1);
    QMetaData::Access *slot_tbl_access = QMetaObject::new_metaaccess(1);
    slot_tbl[0].name = "objectDestroyed()";
    slot_tbl[0].ptr = (QMember)ov1_0;
    slot_tbl_access[0] = QMetaData::Private;
    metaObj = QMetaObject::new_metaobject(
	"QGuardedPtrPrivate", "QObject",
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
