/****************************************************************************
** QSizeGrip meta object code from reading C++ file 'qsizegrip.h'
**
** Created: Fri Apr 20 02:01:51 2001
**      by: The Qt MOC ($Id$)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#if !defined(Q_MOC_OUTPUT_REVISION)
#define Q_MOC_OUTPUT_REVISION 9
#elif Q_MOC_OUTPUT_REVISION != 9
#error "Moc format conflict - please regenerate all moc files"
#endif

#include "qsizegrip.h"
#include <qmetaobject.h>
#include <qapplication.h>



const char *QSizeGrip::className() const
{
    return "QSizeGrip";
}

QMetaObject *QSizeGrip::metaObj = 0;

void QSizeGrip::initMetaObject()
{
    if ( metaObj )
	return;
    if ( qstrcmp(QWidget::className(), "QWidget") != 0 )
	badSuperclassWarning("QSizeGrip","QWidget");
    (void) staticMetaObject();
}

#ifndef QT_NO_TRANSLATION

QString QSizeGrip::tr(const char* s)
{
    return qApp->translate( "QSizeGrip", s, 0 );
}

QString QSizeGrip::tr(const char* s, const char * c)
{
    return qApp->translate( "QSizeGrip", s, c );
}

#endif // QT_NO_TRANSLATION

QMetaObject* QSizeGrip::staticMetaObject()
{
    if ( metaObj )
	return metaObj;
    (void) QWidget::staticMetaObject();
#ifndef QT_NO_PROPERTIES
#endif // QT_NO_PROPERTIES
    QMetaData::Access *slot_tbl_access = 0;
    metaObj = QMetaObject::new_metaobject(
	"QSizeGrip", "QWidget",
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
