/****************************************************************************
** QAccel meta object code from reading C++ file 'qaccel.h'
**
** Created: Fri Apr 20 02:01:24 2001
**      by: The Qt MOC ($Id$)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#if !defined(Q_MOC_OUTPUT_REVISION)
#define Q_MOC_OUTPUT_REVISION 9
#elif Q_MOC_OUTPUT_REVISION != 9
#error "Moc format conflict - please regenerate all moc files"
#endif

#include "qaccel.h"
#include <qmetaobject.h>
#include <qapplication.h>



const char *QAccel::className() const
{
    return "QAccel";
}

QMetaObject *QAccel::metaObj = 0;

void QAccel::initMetaObject()
{
    if ( metaObj )
	return;
    if ( qstrcmp(QObject::className(), "QObject") != 0 )
	badSuperclassWarning("QAccel","QObject");
    (void) staticMetaObject();
}

#ifndef QT_NO_TRANSLATION

QString QAccel::tr(const char* s)
{
    return qApp->translate( "QAccel", s, 0 );
}

QString QAccel::tr(const char* s, const char * c)
{
    return qApp->translate( "QAccel", s, c );
}

#endif // QT_NO_TRANSLATION

QMetaObject* QAccel::staticMetaObject()
{
    if ( metaObj )
	return metaObj;
    (void) QObject::staticMetaObject();
#ifndef QT_NO_PROPERTIES
#endif // QT_NO_PROPERTIES
    QMetaData::Access *slot_tbl_access = 0;
    typedef void (QAccel::*m2_t0)(int);
    typedef void (QObject::*om2_t0)(int);
    m2_t0 v2_0 = &QAccel::activated;
    om2_t0 ov2_0 = (om2_t0)v2_0;
    QMetaData *signal_tbl = QMetaObject::new_metadata(1);
    signal_tbl[0].name = "activated(int)";
    signal_tbl[0].ptr = (QMember)ov2_0;
    metaObj = QMetaObject::new_metaobject(
	"QAccel", "QObject",
	0, 0,
	signal_tbl, 1,
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

// SIGNAL activated
void QAccel::activated( int t0 )
{
    activate_signal( "activated(int)", t0 );
}
