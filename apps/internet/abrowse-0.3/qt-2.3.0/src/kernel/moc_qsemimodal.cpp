/****************************************************************************
** QSemiModal meta object code from reading C++ file 'qsemimodal.h'
**
** Created: Fri Apr 20 02:01:45 2001
**      by: The Qt MOC ($Id$)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#if !defined(Q_MOC_OUTPUT_REVISION)
#define Q_MOC_OUTPUT_REVISION 9
#elif Q_MOC_OUTPUT_REVISION != 9
#error "Moc format conflict - please regenerate all moc files"
#endif

#include "qsemimodal.h"
#include <qmetaobject.h>
#include <qapplication.h>



const char *QSemiModal::className() const
{
    return "QSemiModal";
}

QMetaObject *QSemiModal::metaObj = 0;

void QSemiModal::initMetaObject()
{
    if ( metaObj )
	return;
    if ( qstrcmp(QWidget::className(), "QWidget") != 0 )
	badSuperclassWarning("QSemiModal","QWidget");
    (void) staticMetaObject();
}

#ifndef QT_NO_TRANSLATION

QString QSemiModal::tr(const char* s)
{
    return qApp->translate( "QSemiModal", s, 0 );
}

QString QSemiModal::tr(const char* s, const char * c)
{
    return qApp->translate( "QSemiModal", s, c );
}

#endif // QT_NO_TRANSLATION

QMetaObject* QSemiModal::staticMetaObject()
{
    if ( metaObj )
	return metaObj;
    (void) QWidget::staticMetaObject();
#ifndef QT_NO_PROPERTIES
#endif // QT_NO_PROPERTIES
    QMetaData::Access *slot_tbl_access = 0;
    metaObj = QMetaObject::new_metaobject(
	"QSemiModal", "QWidget",
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
