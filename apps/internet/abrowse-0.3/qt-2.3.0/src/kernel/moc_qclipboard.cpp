/****************************************************************************
** QClipboard meta object code from reading C++ file 'qclipboard.h'
**
** Created: Fri Apr 20 02:01:30 2001
**      by: The Qt MOC ($Id$)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#if !defined(Q_MOC_OUTPUT_REVISION)
#define Q_MOC_OUTPUT_REVISION 9
#elif Q_MOC_OUTPUT_REVISION != 9
#error "Moc format conflict - please regenerate all moc files"
#endif

#include "qclipboard.h"
#include <qmetaobject.h>
#include <qapplication.h>



const char *QClipboard::className() const
{
    return "QClipboard";
}

QMetaObject *QClipboard::metaObj = 0;

void QClipboard::initMetaObject()
{
    if ( metaObj )
	return;
    if ( qstrcmp(QObject::className(), "QObject") != 0 )
	badSuperclassWarning("QClipboard","QObject");
    (void) staticMetaObject();
}

#ifndef QT_NO_TRANSLATION

QString QClipboard::tr(const char* s)
{
    return qApp->translate( "QClipboard", s, 0 );
}

QString QClipboard::tr(const char* s, const char * c)
{
    return qApp->translate( "QClipboard", s, c );
}

#endif // QT_NO_TRANSLATION

QMetaObject* QClipboard::staticMetaObject()
{
    if ( metaObj )
	return metaObj;
    (void) QObject::staticMetaObject();
#ifndef QT_NO_PROPERTIES
#endif // QT_NO_PROPERTIES
    typedef void (QClipboard::*m1_t0)();
    typedef void (QObject::*om1_t0)();
    m1_t0 v1_0 = &QClipboard::ownerDestroyed;
    om1_t0 ov1_0 = (om1_t0)v1_0;
    QMetaData *slot_tbl = QMetaObject::new_metadata(1);
    QMetaData::Access *slot_tbl_access = QMetaObject::new_metaaccess(1);
    slot_tbl[0].name = "ownerDestroyed()";
    slot_tbl[0].ptr = (QMember)ov1_0;
    slot_tbl_access[0] = QMetaData::Private;
    typedef void (QClipboard::*m2_t0)();
    typedef void (QObject::*om2_t0)();
    m2_t0 v2_0 = &QClipboard::dataChanged;
    om2_t0 ov2_0 = (om2_t0)v2_0;
    QMetaData *signal_tbl = QMetaObject::new_metadata(1);
    signal_tbl[0].name = "dataChanged()";
    signal_tbl[0].ptr = (QMember)ov2_0;
    metaObj = QMetaObject::new_metaobject(
	"QClipboard", "QObject",
	slot_tbl, 1,
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

// SIGNAL dataChanged
void QClipboard::dataChanged()
{
    activate_signal( "dataChanged()" );
}
