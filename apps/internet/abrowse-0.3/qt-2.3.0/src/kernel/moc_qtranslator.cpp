/****************************************************************************
** QTranslator meta object code from reading C++ file 'qtranslator.h'
**
** Created: Fri Apr 20 02:01:41 2001
**      by: The Qt MOC ($Id$)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#if !defined(Q_MOC_OUTPUT_REVISION)
#define Q_MOC_OUTPUT_REVISION 9
#elif Q_MOC_OUTPUT_REVISION != 9
#error "Moc format conflict - please regenerate all moc files"
#endif

#include "qtranslator.h"
#include <qmetaobject.h>
#include <qapplication.h>



const char *QTranslator::className() const
{
    return "QTranslator";
}

QMetaObject *QTranslator::metaObj = 0;

void QTranslator::initMetaObject()
{
    if ( metaObj )
	return;
    if ( qstrcmp(QObject::className(), "QObject") != 0 )
	badSuperclassWarning("QTranslator","QObject");
    (void) staticMetaObject();
}

#ifndef QT_NO_TRANSLATION

QString QTranslator::tr(const char* s)
{
    return qApp->translate( "QTranslator", s, 0 );
}

QString QTranslator::tr(const char* s, const char * c)
{
    return qApp->translate( "QTranslator", s, c );
}

#endif // QT_NO_TRANSLATION

QMetaObject* QTranslator::staticMetaObject()
{
    if ( metaObj )
	return metaObj;
    (void) QObject::staticMetaObject();
#ifndef QT_NO_PROPERTIES
#endif // QT_NO_PROPERTIES
    QMetaData::Access *slot_tbl_access = 0;
    metaObj = QMetaObject::new_metaobject(
	"QTranslator", "QObject",
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
