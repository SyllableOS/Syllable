/****************************************************************************
** QApplication meta object code from reading C++ file 'qapplication.h'
**
** Created: Fri Apr 20 02:01:26 2001
**      by: The Qt MOC ($Id$)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#if !defined(Q_MOC_OUTPUT_REVISION)
#define Q_MOC_OUTPUT_REVISION 9
#elif Q_MOC_OUTPUT_REVISION != 9
#error "Moc format conflict - please regenerate all moc files"
#endif

#include "qapplication.h"
#include <qmetaobject.h>
#include <qapplication.h>



const char *QApplication::className() const
{
    return "QApplication";
}

QMetaObject *QApplication::metaObj = 0;

void QApplication::initMetaObject()
{
    if ( metaObj )
	return;
    if ( qstrcmp(QObject::className(), "QObject") != 0 )
	badSuperclassWarning("QApplication","QObject");
    (void) staticMetaObject();
}

#ifndef QT_NO_TRANSLATION

QString QApplication::tr(const char* s)
{
    return qApp->translate( "QApplication", s, 0 );
}

QString QApplication::tr(const char* s, const char * c)
{
    return qApp->translate( "QApplication", s, c );
}

#endif // QT_NO_TRANSLATION

QMetaObject* QApplication::staticMetaObject()
{
    if ( metaObj )
	return metaObj;
    (void) QObject::staticMetaObject();
#ifndef QT_NO_PROPERTIES
#endif // QT_NO_PROPERTIES
    typedef void (QApplication::*m1_t0)();
    typedef void (QObject::*om1_t0)();
    typedef void (QApplication::*m1_t1)();
    typedef void (QObject::*om1_t1)();
    m1_t0 v1_0 = &QApplication::quit;
    om1_t0 ov1_0 = (om1_t0)v1_0;
    m1_t1 v1_1 = &QApplication::closeAllWindows;
    om1_t1 ov1_1 = (om1_t1)v1_1;
    QMetaData *slot_tbl = QMetaObject::new_metadata(2);
    QMetaData::Access *slot_tbl_access = QMetaObject::new_metaaccess(2);
    slot_tbl[0].name = "quit()";
    slot_tbl[0].ptr = (QMember)ov1_0;
    slot_tbl_access[0] = QMetaData::Public;
    slot_tbl[1].name = "closeAllWindows()";
    slot_tbl[1].ptr = (QMember)ov1_1;
    slot_tbl_access[1] = QMetaData::Public;
    typedef void (QApplication::*m2_t0)();
    typedef void (QObject::*om2_t0)();
    typedef void (QApplication::*m2_t1)();
    typedef void (QObject::*om2_t1)();
    typedef void (QApplication::*m2_t2)();
    typedef void (QObject::*om2_t2)();
    m2_t0 v2_0 = &QApplication::lastWindowClosed;
    om2_t0 ov2_0 = (om2_t0)v2_0;
    m2_t1 v2_1 = &QApplication::aboutToQuit;
    om2_t1 ov2_1 = (om2_t1)v2_1;
    m2_t2 v2_2 = &QApplication::guiThreadAwake;
    om2_t2 ov2_2 = (om2_t2)v2_2;
    QMetaData *signal_tbl = QMetaObject::new_metadata(3);
    signal_tbl[0].name = "lastWindowClosed()";
    signal_tbl[0].ptr = (QMember)ov2_0;
    signal_tbl[1].name = "aboutToQuit()";
    signal_tbl[1].ptr = (QMember)ov2_1;
    signal_tbl[2].name = "guiThreadAwake()";
    signal_tbl[2].ptr = (QMember)ov2_2;
    metaObj = QMetaObject::new_metaobject(
	"QApplication", "QObject",
	slot_tbl, 2,
	signal_tbl, 3,
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

// SIGNAL lastWindowClosed
void QApplication::lastWindowClosed()
{
    activate_signal( "lastWindowClosed()" );
}

// SIGNAL aboutToQuit
void QApplication::aboutToQuit()
{
    activate_signal( "aboutToQuit()" );
}

// SIGNAL guiThreadAwake
void QApplication::guiThreadAwake()
{
    activate_signal( "guiThreadAwake()" );
}
