/****************************************************************************
** KRun meta object code from reading C++ file 'krun.h'
**
** Created: Sat Sep 15 13:10:54 2001
**      by: The Qt MOC ($Id$)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#if !defined(Q_MOC_OUTPUT_REVISION)
#define Q_MOC_OUTPUT_REVISION 9
#elif Q_MOC_OUTPUT_REVISION != 9
#error "Moc format conflict - please regenerate all moc files"
#endif

#include "krun.h"
#include <qmetaobject.h>
#include <qapplication.h>



const char *KRun::className() const
{
    return "KRun";
}

QMetaObject *KRun::metaObj = 0;

void KRun::initMetaObject()
{
    if ( metaObj )
	return;
    if ( qstrcmp(QObject::className(), "QObject") != 0 )
	badSuperclassWarning("KRun","QObject");
    (void) staticMetaObject();
}

#ifndef QT_NO_TRANSLATION

QString KRun::tr(const char* s)
{
    return qApp->translate( "KRun", s, 0 );
}

QString KRun::tr(const char* s, const char * c)
{
    return qApp->translate( "KRun", s, c );
}

#endif // QT_NO_TRANSLATION

QMetaObject* KRun::staticMetaObject()
{
    if ( metaObj )
	return metaObj;
    (void) QObject::staticMetaObject();
#ifndef QT_NO_PROPERTIES
#endif // QT_NO_PROPERTIES
    typedef void (KRun::*m1_t0)();
    typedef void (QObject::*om1_t0)();
    m1_t0 v1_0 = &KRun::slotStart;
    om1_t0 ov1_0 = (om1_t0)v1_0;
    QMetaData *slot_tbl = QMetaObject::new_metadata(1);
    QMetaData::Access *slot_tbl_access = QMetaObject::new_metaaccess(1);
    slot_tbl[0].name = "slotStart()";
    slot_tbl[0].ptr = (QMember)ov1_0;
    slot_tbl_access[0] = QMetaData::Private;
    metaObj = QMetaObject::new_metaobject(
	"KRun", "QObject",
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
