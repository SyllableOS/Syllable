/****************************************************************************
** QSignalMapper meta object code from reading C++ file 'qsignalmapper.h'
**
** Created: Fri Apr 20 02:01:49 2001
**      by: The Qt MOC ($Id$)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#if !defined(Q_MOC_OUTPUT_REVISION)
#define Q_MOC_OUTPUT_REVISION 9
#elif Q_MOC_OUTPUT_REVISION != 9
#error "Moc format conflict - please regenerate all moc files"
#endif

#include "qsignalmapper.h"
#include <qmetaobject.h>
#include <qapplication.h>



const char *QSignalMapper::className() const
{
    return "QSignalMapper";
}

QMetaObject *QSignalMapper::metaObj = 0;

void QSignalMapper::initMetaObject()
{
    if ( metaObj )
	return;
    if ( qstrcmp(QObject::className(), "QObject") != 0 )
	badSuperclassWarning("QSignalMapper","QObject");
    (void) staticMetaObject();
}

#ifndef QT_NO_TRANSLATION

QString QSignalMapper::tr(const char* s)
{
    return qApp->translate( "QSignalMapper", s, 0 );
}

QString QSignalMapper::tr(const char* s, const char * c)
{
    return qApp->translate( "QSignalMapper", s, c );
}

#endif // QT_NO_TRANSLATION

QMetaObject* QSignalMapper::staticMetaObject()
{
    if ( metaObj )
	return metaObj;
    (void) QObject::staticMetaObject();
#ifndef QT_NO_PROPERTIES
#endif // QT_NO_PROPERTIES
    typedef void (QSignalMapper::*m1_t0)();
    typedef void (QObject::*om1_t0)();
    typedef void (QSignalMapper::*m1_t1)();
    typedef void (QObject::*om1_t1)();
    m1_t0 v1_0 = &QSignalMapper::map;
    om1_t0 ov1_0 = (om1_t0)v1_0;
    m1_t1 v1_1 = &QSignalMapper::removeMapping;
    om1_t1 ov1_1 = (om1_t1)v1_1;
    QMetaData *slot_tbl = QMetaObject::new_metadata(2);
    QMetaData::Access *slot_tbl_access = QMetaObject::new_metaaccess(2);
    slot_tbl[0].name = "map()";
    slot_tbl[0].ptr = (QMember)ov1_0;
    slot_tbl_access[0] = QMetaData::Public;
    slot_tbl[1].name = "removeMapping()";
    slot_tbl[1].ptr = (QMember)ov1_1;
    slot_tbl_access[1] = QMetaData::Private;
    typedef void (QSignalMapper::*m2_t0)(int);
    typedef void (QObject::*om2_t0)(int);
    typedef void (QSignalMapper::*m2_t1)(const QString&);
    typedef void (QObject::*om2_t1)(const QString&);
    m2_t0 v2_0 = &QSignalMapper::mapped;
    om2_t0 ov2_0 = (om2_t0)v2_0;
    m2_t1 v2_1 = &QSignalMapper::mapped;
    om2_t1 ov2_1 = (om2_t1)v2_1;
    QMetaData *signal_tbl = QMetaObject::new_metadata(2);
    signal_tbl[0].name = "mapped(int)";
    signal_tbl[0].ptr = (QMember)ov2_0;
    signal_tbl[1].name = "mapped(const QString&)";
    signal_tbl[1].ptr = (QMember)ov2_1;
    metaObj = QMetaObject::new_metaobject(
	"QSignalMapper", "QObject",
	slot_tbl, 2,
	signal_tbl, 2,
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

// SIGNAL mapped
void QSignalMapper::mapped( int t0 )
{
    activate_signal( "mapped(int)", t0 );
}

// SIGNAL mapped
void QSignalMapper::mapped( const QString& t0 )
{
    activate_signal_strref( "mapped(const QString&)", t0 );
}
