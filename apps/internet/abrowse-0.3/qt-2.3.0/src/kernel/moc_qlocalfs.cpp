/****************************************************************************
** QLocalFs meta object code from reading C++ file 'qlocalfs.h'
**
** Created: Fri Apr 20 02:02:02 2001
**      by: The Qt MOC ($Id$)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#if !defined(Q_MOC_OUTPUT_REVISION)
#define Q_MOC_OUTPUT_REVISION 9
#elif Q_MOC_OUTPUT_REVISION != 9
#error "Moc format conflict - please regenerate all moc files"
#endif

#include "qlocalfs.h"
#include <qmetaobject.h>
#include <qapplication.h>



const char *QLocalFs::className() const
{
    return "QLocalFs";
}

QMetaObject *QLocalFs::metaObj = 0;

void QLocalFs::initMetaObject()
{
    if ( metaObj )
	return;
    if ( qstrcmp(QNetworkProtocol::className(), "QNetworkProtocol") != 0 )
	badSuperclassWarning("QLocalFs","QNetworkProtocol");
    (void) staticMetaObject();
}

#ifndef QT_NO_TRANSLATION

QString QLocalFs::tr(const char* s)
{
    return qApp->translate( "QLocalFs", s, 0 );
}

QString QLocalFs::tr(const char* s, const char * c)
{
    return qApp->translate( "QLocalFs", s, c );
}

#endif // QT_NO_TRANSLATION

QMetaObject* QLocalFs::staticMetaObject()
{
    if ( metaObj )
	return metaObj;
    (void) QNetworkProtocol::staticMetaObject();
#ifndef QT_NO_PROPERTIES
#endif // QT_NO_PROPERTIES
    QMetaData::Access *slot_tbl_access = 0;
    metaObj = QMetaObject::new_metaobject(
	"QLocalFs", "QNetworkProtocol",
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
