/****************************************************************************
** QDialog meta object code from reading C++ file 'qdialog.h'
**
** Created: Fri Apr 20 02:01:32 2001
**      by: The Qt MOC ($Id$)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#if !defined(Q_MOC_OUTPUT_REVISION)
#define Q_MOC_OUTPUT_REVISION 9
#elif Q_MOC_OUTPUT_REVISION != 9
#error "Moc format conflict - please regenerate all moc files"
#endif

#include "qdialog.h"
#include <qmetaobject.h>
#include <qapplication.h>



const char *QDialog::className() const
{
    return "QDialog";
}

QMetaObject *QDialog::metaObj = 0;

void QDialog::initMetaObject()
{
    if ( metaObj )
	return;
    if ( qstrcmp(QWidget::className(), "QWidget") != 0 )
	badSuperclassWarning("QDialog","QWidget");
    (void) staticMetaObject();
}

#ifndef QT_NO_TRANSLATION

QString QDialog::tr(const char* s)
{
    return qApp->translate( "QDialog", s, 0 );
}

QString QDialog::tr(const char* s, const char * c)
{
    return qApp->translate( "QDialog", s, c );
}

#endif // QT_NO_TRANSLATION

QMetaObject* QDialog::staticMetaObject()
{
    if ( metaObj )
	return metaObj;
    (void) QWidget::staticMetaObject();
#ifndef QT_NO_PROPERTIES
    typedef bool (QDialog::*m3_t0)()const;
    typedef bool (QObject::*om3_t0)()const;
    typedef void (QDialog::*m3_t1)(bool);
    typedef void (QObject::*om3_t1)(bool);
    m3_t0 v3_0 = &QDialog::isSizeGripEnabled;
    om3_t0 ov3_0 = (om3_t0)v3_0;
    m3_t1 v3_1 = &QDialog::setSizeGripEnabled;
    om3_t1 ov3_1 = (om3_t1)v3_1;
    QMetaProperty *props_tbl = QMetaObject::new_metaproperty( 1 );
    props_tbl[0].t = "bool";
    props_tbl[0].n = "sizeGripEnabled";
    props_tbl[0].get = (QMember)ov3_0;
    props_tbl[0].set = (QMember)ov3_1;
    props_tbl[0].reset = 0;
    props_tbl[0].gspec = QMetaProperty::Class;
    props_tbl[0].sspec = QMetaProperty::Class;
    props_tbl[0].setFlags(QMetaProperty::StdSet);
#endif // QT_NO_PROPERTIES
    typedef void (QDialog::*m1_t0)(int);
    typedef void (QObject::*om1_t0)(int);
    typedef void (QDialog::*m1_t1)();
    typedef void (QObject::*om1_t1)();
    typedef void (QDialog::*m1_t2)();
    typedef void (QObject::*om1_t2)();
    typedef void (QDialog::*m1_t3)(bool);
    typedef void (QObject::*om1_t3)(bool);
    m1_t0 v1_0 = &QDialog::done;
    om1_t0 ov1_0 = (om1_t0)v1_0;
    m1_t1 v1_1 = &QDialog::accept;
    om1_t1 ov1_1 = (om1_t1)v1_1;
    m1_t2 v1_2 = &QDialog::reject;
    om1_t2 ov1_2 = (om1_t2)v1_2;
    m1_t3 v1_3 = &QDialog::showExtension;
    om1_t3 ov1_3 = (om1_t3)v1_3;
    QMetaData *slot_tbl = QMetaObject::new_metadata(4);
    QMetaData::Access *slot_tbl_access = QMetaObject::new_metaaccess(4);
    slot_tbl[0].name = "done(int)";
    slot_tbl[0].ptr = (QMember)ov1_0;
    slot_tbl_access[0] = QMetaData::Protected;
    slot_tbl[1].name = "accept()";
    slot_tbl[1].ptr = (QMember)ov1_1;
    slot_tbl_access[1] = QMetaData::Protected;
    slot_tbl[2].name = "reject()";
    slot_tbl[2].ptr = (QMember)ov1_2;
    slot_tbl_access[2] = QMetaData::Protected;
    slot_tbl[3].name = "showExtension(bool)";
    slot_tbl[3].ptr = (QMember)ov1_3;
    slot_tbl_access[3] = QMetaData::Protected;
    metaObj = QMetaObject::new_metaobject(
	"QDialog", "QWidget",
	slot_tbl, 4,
	0, 0,
#ifndef QT_NO_PROPERTIES
	props_tbl, 1,
	0, 0,
#endif // QT_NO_PROPERTIES
	0, 0 );
    metaObj->set_slot_access( slot_tbl_access );
#ifndef QT_NO_PROPERTIES
#endif // QT_NO_PROPERTIES
    return metaObj;
}
