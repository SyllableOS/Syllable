/****************************************************************************
** QTimer meta object code from reading C++ file 'qtimer.h'
**
** Created: Fri Apr 20 02:02:00 2001
**      by: The Qt MOC ($Id$)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#if !defined(Q_MOC_OUTPUT_REVISION)
#define Q_MOC_OUTPUT_REVISION 9
#elif Q_MOC_OUTPUT_REVISION != 9
#error "Moc format conflict - please regenerate all moc files"
#endif

#include "qtimer.h"
#include <qmetaobject.h>
#include <qapplication.h>



const char *QTimer::className() const
{
    return "QTimer";
}

QMetaObject *QTimer::metaObj = 0;

void QTimer::initMetaObject()
{
    if ( metaObj )
	return;
    if ( qstrcmp(QObject::className(), "QObject") != 0 )
	badSuperclassWarning("QTimer","QObject");
    (void) staticMetaObject();
}

#ifndef QT_NO_TRANSLATION

QString QTimer::tr(const char* s)
{
    return qApp->translate( "QTimer", s, 0 );
}

QString QTimer::tr(const char* s, const char * c)
{
    return qApp->translate( "QTimer", s, c );
}

#endif // QT_NO_TRANSLATION

QMetaObject* QTimer::staticMetaObject()
{
    if ( metaObj )
	return metaObj;
    (void) QObject::staticMetaObject();
#ifndef QT_NO_PROPERTIES
#endif // QT_NO_PROPERTIES
    QMetaData::Access *slot_tbl_access = 0;
    typedef void (QTimer::*m2_t0)();
    typedef void (QObject::*om2_t0)();
    m2_t0 v2_0 = &QTimer::timeout;
    om2_t0 ov2_0 = (om2_t0)v2_0;
    QMetaData *signal_tbl = QMetaObject::new_metadata(1);
    signal_tbl[0].name = "timeout()";
    signal_tbl[0].ptr = (QMember)ov2_0;
    metaObj = QMetaObject::new_metaobject(
	"QTimer", "QObject",
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

// SIGNAL timeout
void QTimer::timeout()
{
    activate_signal( "timeout()" );
}
