/****************************************************************************
** QStyle meta object code from reading C++ file 'qstyle.h'
**
** Created: Fri Apr 20 02:01:57 2001
**      by: The Qt MOC ($Id$)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#if !defined(Q_MOC_OUTPUT_REVISION)
#define Q_MOC_OUTPUT_REVISION 9
#elif Q_MOC_OUTPUT_REVISION != 9
#error "Moc format conflict - please regenerate all moc files"
#endif

#include "qstyle.h"
#include <qmetaobject.h>
#include <qapplication.h>



const char *QStyle::className() const
{
    return "QStyle";
}

QMetaObject *QStyle::metaObj = 0;

void QStyle::initMetaObject()
{
    if ( metaObj )
	return;
    if ( qstrcmp(QObject::className(), "QObject") != 0 )
	badSuperclassWarning("QStyle","QObject");
    (void) staticMetaObject();
}

#ifndef QT_NO_TRANSLATION

QString QStyle::tr(const char* s)
{
    return qApp->translate( "QStyle", s, 0 );
}

QString QStyle::tr(const char* s, const char * c)
{
    return qApp->translate( "QStyle", s, c );
}

#endif // QT_NO_TRANSLATION

QMetaObject* QStyle::staticMetaObject()
{
    if ( metaObj )
	return metaObj;
    (void) QObject::staticMetaObject();
#ifndef QT_NO_PROPERTIES
#endif // QT_NO_PROPERTIES
    QMetaData::Access *slot_tbl_access = 0;
    metaObj = QMetaObject::new_metaobject(
	"QStyle", "QObject",
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
