/****************************************************************************
** QLayout meta object code from reading C++ file 'qabstractlayout.h'
**
** Created: Fri Apr 20 02:01:22 2001
**      by: The Qt MOC ($Id$)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#if !defined(Q_MOC_OUTPUT_REVISION)
#define Q_MOC_OUTPUT_REVISION 9
#elif Q_MOC_OUTPUT_REVISION != 9
#error "Moc format conflict - please regenerate all moc files"
#endif

#include "qabstractlayout.h"
#include <qmetaobject.h>
#include <qapplication.h>



const char *QLayout::className() const
{
    return "QLayout";
}

QMetaObject *QLayout::metaObj = 0;

void QLayout::initMetaObject()
{
    if ( metaObj )
	return;
    if ( qstrcmp(QObject::className(), "QObject") != 0 )
	badSuperclassWarning("QLayout","QObject");
    (void) staticMetaObject();
}

#ifndef QT_NO_TRANSLATION

QString QLayout::tr(const char* s)
{
    return qApp->translate( "QLayout", s, 0 );
}

QString QLayout::tr(const char* s, const char * c)
{
    return qApp->translate( "QLayout", s, c );
}

#endif // QT_NO_TRANSLATION

QMetaObject* QLayout::staticMetaObject()
{
    if ( metaObj )
	return metaObj;
    (void) QObject::staticMetaObject();
#ifndef QT_NO_PROPERTIES
    QMetaEnum* enum_tbl = QMetaObject::new_metaenum( 1 );
    enum_tbl[0].name = "ResizeMode";
    enum_tbl[0].count = 3;
    enum_tbl[0].items = QMetaObject::new_metaenum_item( 3 );
    enum_tbl[0].set = FALSE;
    enum_tbl[0].items[0].key = "FreeResize";
    enum_tbl[0].items[0].value = (int) QLayout::FreeResize;
    enum_tbl[0].items[1].key = "Minimum";
    enum_tbl[0].items[1].value = (int) QLayout::Minimum;
    enum_tbl[0].items[2].key = "Fixed";
    enum_tbl[0].items[2].value = (int) QLayout::Fixed;
#endif // QT_NO_PROPERTIES
#ifndef QT_NO_PROPERTIES
    typedef int (QLayout::*m3_t0)()const;
    typedef int (QObject::*om3_t0)()const;
    typedef void (QLayout::*m3_t1)(int);
    typedef void (QObject::*om3_t1)(int);
    typedef int (QLayout::*m3_t3)()const;
    typedef int (QObject::*om3_t3)()const;
    typedef void (QLayout::*m3_t4)(int);
    typedef void (QObject::*om3_t4)(int);
    typedef ResizeMode (QLayout::*m3_t6)()const;
    typedef ResizeMode (QObject::*om3_t6)()const;
    typedef void (QLayout::*m3_t7)(ResizeMode);
    typedef void (QObject::*om3_t7)(ResizeMode);
    m3_t0 v3_0 = &QLayout::margin;
    om3_t0 ov3_0 = (om3_t0)v3_0;
    m3_t1 v3_1 = &QLayout::setMargin;
    om3_t1 ov3_1 = (om3_t1)v3_1;
    m3_t3 v3_3 = &QLayout::spacing;
    om3_t3 ov3_3 = (om3_t3)v3_3;
    m3_t4 v3_4 = &QLayout::setSpacing;
    om3_t4 ov3_4 = (om3_t4)v3_4;
    m3_t6 v3_6 = &QLayout::resizeMode;
    om3_t6 ov3_6 = (om3_t6)v3_6;
    m3_t7 v3_7 = &QLayout::setResizeMode;
    om3_t7 ov3_7 = (om3_t7)v3_7;
    QMetaProperty *props_tbl = QMetaObject::new_metaproperty( 3 );
    props_tbl[0].t = "int";
    props_tbl[0].n = "margin";
    props_tbl[0].get = (QMember)ov3_0;
    props_tbl[0].set = (QMember)ov3_1;
    props_tbl[0].reset = 0;
    props_tbl[0].gspec = QMetaProperty::Class;
    props_tbl[0].sspec = QMetaProperty::Class;
    props_tbl[0].setFlags(QMetaProperty::StdSet);
    props_tbl[1].t = "int";
    props_tbl[1].n = "spacing";
    props_tbl[1].get = (QMember)ov3_3;
    props_tbl[1].set = (QMember)ov3_4;
    props_tbl[1].reset = 0;
    props_tbl[1].gspec = QMetaProperty::Class;
    props_tbl[1].sspec = QMetaProperty::Class;
    props_tbl[1].setFlags(QMetaProperty::StdSet);
    props_tbl[2].t = "ResizeMode";
    props_tbl[2].n = "resizeMode";
    props_tbl[2].get = (QMember)ov3_6;
    props_tbl[2].set = (QMember)ov3_7;
    props_tbl[2].reset = 0;
    props_tbl[2].gspec = QMetaProperty::Class;
    props_tbl[2].sspec = QMetaProperty::Class;
    props_tbl[2].enumData = &enum_tbl[0];
    props_tbl[2].setFlags(QMetaProperty::StdSet);
#endif // QT_NO_PROPERTIES
    QMetaData::Access *slot_tbl_access = 0;
    metaObj = QMetaObject::new_metaobject(
	"QLayout", "QObject",
	0, 0,
	0, 0,
#ifndef QT_NO_PROPERTIES
	props_tbl, 3,
	enum_tbl, 1,
#endif // QT_NO_PROPERTIES
	0, 0 );
    metaObj->set_slot_access( slot_tbl_access );
#ifndef QT_NO_PROPERTIES
#endif // QT_NO_PROPERTIES
    return metaObj;
}
