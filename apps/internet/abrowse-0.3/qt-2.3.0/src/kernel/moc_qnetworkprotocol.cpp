/****************************************************************************
** QNetworkProtocol meta object code from reading C++ file 'qnetworkprotocol.h'
**
** Created: Fri Apr 20 02:01:42 2001
**      by: The Qt MOC ($Id$)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#if !defined(Q_MOC_OUTPUT_REVISION)
#define Q_MOC_OUTPUT_REVISION 9
#elif Q_MOC_OUTPUT_REVISION != 9
#error "Moc format conflict - please regenerate all moc files"
#endif

#include "qnetworkprotocol.h"
#include <qmetaobject.h>
#include <qapplication.h>



const char *QNetworkProtocol::className() const
{
    return "QNetworkProtocol";
}

QMetaObject *QNetworkProtocol::metaObj = 0;

void QNetworkProtocol::initMetaObject()
{
    if ( metaObj )
	return;
    if ( qstrcmp(QObject::className(), "QObject") != 0 )
	badSuperclassWarning("QNetworkProtocol","QObject");
    (void) staticMetaObject();
}

#ifndef QT_NO_TRANSLATION

QString QNetworkProtocol::tr(const char* s)
{
    return qApp->translate( "QNetworkProtocol", s, 0 );
}

QString QNetworkProtocol::tr(const char* s, const char * c)
{
    return qApp->translate( "QNetworkProtocol", s, c );
}

#endif // QT_NO_TRANSLATION

QMetaObject* QNetworkProtocol::staticMetaObject()
{
    if ( metaObj )
	return metaObj;
    (void) QObject::staticMetaObject();
#ifndef QT_NO_PROPERTIES
#endif // QT_NO_PROPERTIES
    typedef void (QNetworkProtocol::*m1_t0)(QNetworkOperation*);
    typedef void (QObject::*om1_t0)(QNetworkOperation*);
    typedef void (QNetworkProtocol::*m1_t1)();
    typedef void (QObject::*om1_t1)();
    typedef void (QNetworkProtocol::*m1_t2)(const QUrlInfo&,QNetworkOperation*);
    typedef void (QObject::*om1_t2)(const QUrlInfo&,QNetworkOperation*);
    typedef void (QNetworkProtocol::*m1_t3)();
    typedef void (QObject::*om1_t3)();
    m1_t0 v1_0 = &QNetworkProtocol::processNextOperation;
    om1_t0 ov1_0 = (om1_t0)v1_0;
    m1_t1 v1_1 = &QNetworkProtocol::startOps;
    om1_t1 ov1_1 = (om1_t1)v1_1;
    m1_t2 v1_2 = &QNetworkProtocol::emitNewChildren;
    om1_t2 ov1_2 = (om1_t2)v1_2;
    m1_t3 v1_3 = &QNetworkProtocol::removeMe;
    om1_t3 ov1_3 = (om1_t3)v1_3;
    QMetaData *slot_tbl = QMetaObject::new_metadata(4);
    QMetaData::Access *slot_tbl_access = QMetaObject::new_metaaccess(4);
    slot_tbl[0].name = "processNextOperation(QNetworkOperation*)";
    slot_tbl[0].ptr = (QMember)ov1_0;
    slot_tbl_access[0] = QMetaData::Private;
    slot_tbl[1].name = "startOps()";
    slot_tbl[1].ptr = (QMember)ov1_1;
    slot_tbl_access[1] = QMetaData::Private;
    slot_tbl[2].name = "emitNewChildren(const QUrlInfo&,QNetworkOperation*)";
    slot_tbl[2].ptr = (QMember)ov1_2;
    slot_tbl_access[2] = QMetaData::Private;
    slot_tbl[3].name = "removeMe()";
    slot_tbl[3].ptr = (QMember)ov1_3;
    slot_tbl_access[3] = QMetaData::Private;
    typedef void (QNetworkProtocol::*m2_t0)(const QByteArray&,QNetworkOperation*);
    typedef void (QObject::*om2_t0)(const QByteArray&,QNetworkOperation*);
    typedef void (QNetworkProtocol::*m2_t1)(int,const QString&);
    typedef void (QObject::*om2_t1)(int,const QString&);
    typedef void (QNetworkProtocol::*m2_t2)(QNetworkOperation*);
    typedef void (QObject::*om2_t2)(QNetworkOperation*);
    typedef void (QNetworkProtocol::*m2_t3)(QNetworkOperation*);
    typedef void (QObject::*om2_t3)(QNetworkOperation*);
    typedef void (QNetworkProtocol::*m2_t4)(const QValueList<QUrlInfo>&,QNetworkOperation*);
    typedef void (QObject::*om2_t4)(const QValueList<QUrlInfo>&,QNetworkOperation*);
    typedef void (QNetworkProtocol::*m2_t5)(const QUrlInfo&,QNetworkOperation*);
    typedef void (QObject::*om2_t5)(const QUrlInfo&,QNetworkOperation*);
    typedef void (QNetworkProtocol::*m2_t6)(const QUrlInfo&,QNetworkOperation*);
    typedef void (QObject::*om2_t6)(const QUrlInfo&,QNetworkOperation*);
    typedef void (QNetworkProtocol::*m2_t7)(QNetworkOperation*);
    typedef void (QObject::*om2_t7)(QNetworkOperation*);
    typedef void (QNetworkProtocol::*m2_t8)(QNetworkOperation*);
    typedef void (QObject::*om2_t8)(QNetworkOperation*);
    typedef void (QNetworkProtocol::*m2_t9)(int,int,QNetworkOperation*);
    typedef void (QObject::*om2_t9)(int,int,QNetworkOperation*);
    m2_t0 v2_0 = &QNetworkProtocol::data;
    om2_t0 ov2_0 = (om2_t0)v2_0;
    m2_t1 v2_1 = &QNetworkProtocol::connectionStateChanged;
    om2_t1 ov2_1 = (om2_t1)v2_1;
    m2_t2 v2_2 = &QNetworkProtocol::finished;
    om2_t2 ov2_2 = (om2_t2)v2_2;
    m2_t3 v2_3 = &QNetworkProtocol::start;
    om2_t3 ov2_3 = (om2_t3)v2_3;
    m2_t4 v2_4 = &QNetworkProtocol::newChildren;
    om2_t4 ov2_4 = (om2_t4)v2_4;
    m2_t5 v2_5 = &QNetworkProtocol::newChild;
    om2_t5 ov2_5 = (om2_t5)v2_5;
    m2_t6 v2_6 = &QNetworkProtocol::createdDirectory;
    om2_t6 ov2_6 = (om2_t6)v2_6;
    m2_t7 v2_7 = &QNetworkProtocol::removed;
    om2_t7 ov2_7 = (om2_t7)v2_7;
    m2_t8 v2_8 = &QNetworkProtocol::itemChanged;
    om2_t8 ov2_8 = (om2_t8)v2_8;
    m2_t9 v2_9 = &QNetworkProtocol::dataTransferProgress;
    om2_t9 ov2_9 = (om2_t9)v2_9;
    QMetaData *signal_tbl = QMetaObject::new_metadata(10);
    signal_tbl[0].name = "data(const QByteArray&,QNetworkOperation*)";
    signal_tbl[0].ptr = (QMember)ov2_0;
    signal_tbl[1].name = "connectionStateChanged(int,const QString&)";
    signal_tbl[1].ptr = (QMember)ov2_1;
    signal_tbl[2].name = "finished(QNetworkOperation*)";
    signal_tbl[2].ptr = (QMember)ov2_2;
    signal_tbl[3].name = "start(QNetworkOperation*)";
    signal_tbl[3].ptr = (QMember)ov2_3;
    signal_tbl[4].name = "newChildren(const QValueList<QUrlInfo>&,QNetworkOperation*)";
    signal_tbl[4].ptr = (QMember)ov2_4;
    signal_tbl[5].name = "newChild(const QUrlInfo&,QNetworkOperation*)";
    signal_tbl[5].ptr = (QMember)ov2_5;
    signal_tbl[6].name = "createdDirectory(const QUrlInfo&,QNetworkOperation*)";
    signal_tbl[6].ptr = (QMember)ov2_6;
    signal_tbl[7].name = "removed(QNetworkOperation*)";
    signal_tbl[7].ptr = (QMember)ov2_7;
    signal_tbl[8].name = "itemChanged(QNetworkOperation*)";
    signal_tbl[8].ptr = (QMember)ov2_8;
    signal_tbl[9].name = "dataTransferProgress(int,int,QNetworkOperation*)";
    signal_tbl[9].ptr = (QMember)ov2_9;
    metaObj = QMetaObject::new_metaobject(
	"QNetworkProtocol", "QObject",
	slot_tbl, 4,
	signal_tbl, 10,
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

#include <qobjectdefs.h>
#include <qsignalslotimp.h>

// SIGNAL data
void QNetworkProtocol::data( const QByteArray& t0, QNetworkOperation* t1 )
{
    // No builtin function for signal parameter type const QByteArray&,QNetworkOperation*
    QConnectionList *clist = receivers("data(const QByteArray&,QNetworkOperation*)");
    if ( !clist || signalsBlocked() )
	return;
    typedef void (QObject::*RT0)();
    typedef void (QObject::*RT1)(const QByteArray&);
    typedef void (QObject::*RT2)(const QByteArray&,QNetworkOperation*);
    RT0 r0;
    RT1 r1;
    RT2 r2;
    QConnectionListIt it(*clist);
    QConnection   *c;
    QSenderObject *object;
    while ( (c=it.current()) ) {
	++it;
	object = (QSenderObject*)c->object();
	object->setSender( this );
	switch ( c->numArgs() ) {
	    case 0:
#ifdef Q_FP_CCAST_BROKEN
		r0 = reinterpret_cast<RT0>(*(c->member()));
#else
		r0 = (RT0)*(c->member());
#endif
		(object->*r0)();
		break;
	    case 1:
#ifdef Q_FP_CCAST_BROKEN
		r1 = reinterpret_cast<RT1>(*(c->member()));
#else
		r1 = (RT1)*(c->member());
#endif
		(object->*r1)(t0);
		break;
	    case 2:
#ifdef Q_FP_CCAST_BROKEN
		r2 = reinterpret_cast<RT2>(*(c->member()));
#else
		r2 = (RT2)*(c->member());
#endif
		(object->*r2)(t0, t1);
		break;
	}
    }
}

// SIGNAL connectionStateChanged
void QNetworkProtocol::connectionStateChanged( int t0, const QString& t1 )
{
    // No builtin function for signal parameter type int,const QString&
    QConnectionList *clist = receivers("connectionStateChanged(int,const QString&)");
    if ( !clist || signalsBlocked() )
	return;
    typedef void (QObject::*RT0)();
    typedef void (QObject::*RT1)(int);
    typedef void (QObject::*RT2)(int,const QString&);
    RT0 r0;
    RT1 r1;
    RT2 r2;
    QConnectionListIt it(*clist);
    QConnection   *c;
    QSenderObject *object;
    while ( (c=it.current()) ) {
	++it;
	object = (QSenderObject*)c->object();
	object->setSender( this );
	switch ( c->numArgs() ) {
	    case 0:
#ifdef Q_FP_CCAST_BROKEN
		r0 = reinterpret_cast<RT0>(*(c->member()));
#else
		r0 = (RT0)*(c->member());
#endif
		(object->*r0)();
		break;
	    case 1:
#ifdef Q_FP_CCAST_BROKEN
		r1 = reinterpret_cast<RT1>(*(c->member()));
#else
		r1 = (RT1)*(c->member());
#endif
		(object->*r1)(t0);
		break;
	    case 2:
#ifdef Q_FP_CCAST_BROKEN
		r2 = reinterpret_cast<RT2>(*(c->member()));
#else
		r2 = (RT2)*(c->member());
#endif
		(object->*r2)(t0, t1);
		break;
	}
    }
}

// SIGNAL finished
void QNetworkProtocol::finished( QNetworkOperation* t0 )
{
    // No builtin function for signal parameter type QNetworkOperation*
    QConnectionList *clist = receivers("finished(QNetworkOperation*)");
    if ( !clist || signalsBlocked() )
	return;
    typedef void (QObject::*RT0)();
    typedef void (QObject::*RT1)(QNetworkOperation*);
    RT0 r0;
    RT1 r1;
    QConnectionListIt it(*clist);
    QConnection   *c;
    QSenderObject *object;
    while ( (c=it.current()) ) {
	++it;
	object = (QSenderObject*)c->object();
	object->setSender( this );
	switch ( c->numArgs() ) {
	    case 0:
#ifdef Q_FP_CCAST_BROKEN
		r0 = reinterpret_cast<RT0>(*(c->member()));
#else
		r0 = (RT0)*(c->member());
#endif
		(object->*r0)();
		break;
	    case 1:
#ifdef Q_FP_CCAST_BROKEN
		r1 = reinterpret_cast<RT1>(*(c->member()));
#else
		r1 = (RT1)*(c->member());
#endif
		(object->*r1)(t0);
		break;
	}
    }
}

// SIGNAL start
void QNetworkProtocol::start( QNetworkOperation* t0 )
{
    // No builtin function for signal parameter type QNetworkOperation*
    QConnectionList *clist = receivers("start(QNetworkOperation*)");
    if ( !clist || signalsBlocked() )
	return;
    typedef void (QObject::*RT0)();
    typedef void (QObject::*RT1)(QNetworkOperation*);
    RT0 r0;
    RT1 r1;
    QConnectionListIt it(*clist);
    QConnection   *c;
    QSenderObject *object;
    while ( (c=it.current()) ) {
	++it;
	object = (QSenderObject*)c->object();
	object->setSender( this );
	switch ( c->numArgs() ) {
	    case 0:
#ifdef Q_FP_CCAST_BROKEN
		r0 = reinterpret_cast<RT0>(*(c->member()));
#else
		r0 = (RT0)*(c->member());
#endif
		(object->*r0)();
		break;
	    case 1:
#ifdef Q_FP_CCAST_BROKEN
		r1 = reinterpret_cast<RT1>(*(c->member()));
#else
		r1 = (RT1)*(c->member());
#endif
		(object->*r1)(t0);
		break;
	}
    }
}

// SIGNAL newChildren
void QNetworkProtocol::newChildren( const QValueList<QUrlInfo>& t0, QNetworkOperation* t1 )
{
    // No builtin function for signal parameter type const QValueList<QUrlInfo>&,QNetworkOperation*
    QConnectionList *clist = receivers("newChildren(const QValueList<QUrlInfo>&,QNetworkOperation*)");
    if ( !clist || signalsBlocked() )
	return;
    typedef void (QObject::*RT0)();
    typedef void (QObject::*RT1)(const QValueList<QUrlInfo>&);
    typedef void (QObject::*RT2)(const QValueList<QUrlInfo>&,QNetworkOperation*);
    RT0 r0;
    RT1 r1;
    RT2 r2;
    QConnectionListIt it(*clist);
    QConnection   *c;
    QSenderObject *object;
    while ( (c=it.current()) ) {
	++it;
	object = (QSenderObject*)c->object();
	object->setSender( this );
	switch ( c->numArgs() ) {
	    case 0:
#ifdef Q_FP_CCAST_BROKEN
		r0 = reinterpret_cast<RT0>(*(c->member()));
#else
		r0 = (RT0)*(c->member());
#endif
		(object->*r0)();
		break;
	    case 1:
#ifdef Q_FP_CCAST_BROKEN
		r1 = reinterpret_cast<RT1>(*(c->member()));
#else
		r1 = (RT1)*(c->member());
#endif
		(object->*r1)(t0);
		break;
	    case 2:
#ifdef Q_FP_CCAST_BROKEN
		r2 = reinterpret_cast<RT2>(*(c->member()));
#else
		r2 = (RT2)*(c->member());
#endif
		(object->*r2)(t0, t1);
		break;
	}
    }
}

// SIGNAL newChild
void QNetworkProtocol::newChild( const QUrlInfo& t0, QNetworkOperation* t1 )
{
    // No builtin function for signal parameter type const QUrlInfo&,QNetworkOperation*
    QConnectionList *clist = receivers("newChild(const QUrlInfo&,QNetworkOperation*)");
    if ( !clist || signalsBlocked() )
	return;
    typedef void (QObject::*RT0)();
    typedef void (QObject::*RT1)(const QUrlInfo&);
    typedef void (QObject::*RT2)(const QUrlInfo&,QNetworkOperation*);
    RT0 r0;
    RT1 r1;
    RT2 r2;
    QConnectionListIt it(*clist);
    QConnection   *c;
    QSenderObject *object;
    while ( (c=it.current()) ) {
	++it;
	object = (QSenderObject*)c->object();
	object->setSender( this );
	switch ( c->numArgs() ) {
	    case 0:
#ifdef Q_FP_CCAST_BROKEN
		r0 = reinterpret_cast<RT0>(*(c->member()));
#else
		r0 = (RT0)*(c->member());
#endif
		(object->*r0)();
		break;
	    case 1:
#ifdef Q_FP_CCAST_BROKEN
		r1 = reinterpret_cast<RT1>(*(c->member()));
#else
		r1 = (RT1)*(c->member());
#endif
		(object->*r1)(t0);
		break;
	    case 2:
#ifdef Q_FP_CCAST_BROKEN
		r2 = reinterpret_cast<RT2>(*(c->member()));
#else
		r2 = (RT2)*(c->member());
#endif
		(object->*r2)(t0, t1);
		break;
	}
    }
}

// SIGNAL createdDirectory
void QNetworkProtocol::createdDirectory( const QUrlInfo& t0, QNetworkOperation* t1 )
{
    // No builtin function for signal parameter type const QUrlInfo&,QNetworkOperation*
    QConnectionList *clist = receivers("createdDirectory(const QUrlInfo&,QNetworkOperation*)");
    if ( !clist || signalsBlocked() )
	return;
    typedef void (QObject::*RT0)();
    typedef void (QObject::*RT1)(const QUrlInfo&);
    typedef void (QObject::*RT2)(const QUrlInfo&,QNetworkOperation*);
    RT0 r0;
    RT1 r1;
    RT2 r2;
    QConnectionListIt it(*clist);
    QConnection   *c;
    QSenderObject *object;
    while ( (c=it.current()) ) {
	++it;
	object = (QSenderObject*)c->object();
	object->setSender( this );
	switch ( c->numArgs() ) {
	    case 0:
#ifdef Q_FP_CCAST_BROKEN
		r0 = reinterpret_cast<RT0>(*(c->member()));
#else
		r0 = (RT0)*(c->member());
#endif
		(object->*r0)();
		break;
	    case 1:
#ifdef Q_FP_CCAST_BROKEN
		r1 = reinterpret_cast<RT1>(*(c->member()));
#else
		r1 = (RT1)*(c->member());
#endif
		(object->*r1)(t0);
		break;
	    case 2:
#ifdef Q_FP_CCAST_BROKEN
		r2 = reinterpret_cast<RT2>(*(c->member()));
#else
		r2 = (RT2)*(c->member());
#endif
		(object->*r2)(t0, t1);
		break;
	}
    }
}

// SIGNAL removed
void QNetworkProtocol::removed( QNetworkOperation* t0 )
{
    // No builtin function for signal parameter type QNetworkOperation*
    QConnectionList *clist = receivers("removed(QNetworkOperation*)");
    if ( !clist || signalsBlocked() )
	return;
    typedef void (QObject::*RT0)();
    typedef void (QObject::*RT1)(QNetworkOperation*);
    RT0 r0;
    RT1 r1;
    QConnectionListIt it(*clist);
    QConnection   *c;
    QSenderObject *object;
    while ( (c=it.current()) ) {
	++it;
	object = (QSenderObject*)c->object();
	object->setSender( this );
	switch ( c->numArgs() ) {
	    case 0:
#ifdef Q_FP_CCAST_BROKEN
		r0 = reinterpret_cast<RT0>(*(c->member()));
#else
		r0 = (RT0)*(c->member());
#endif
		(object->*r0)();
		break;
	    case 1:
#ifdef Q_FP_CCAST_BROKEN
		r1 = reinterpret_cast<RT1>(*(c->member()));
#else
		r1 = (RT1)*(c->member());
#endif
		(object->*r1)(t0);
		break;
	}
    }
}

// SIGNAL itemChanged
void QNetworkProtocol::itemChanged( QNetworkOperation* t0 )
{
    // No builtin function for signal parameter type QNetworkOperation*
    QConnectionList *clist = receivers("itemChanged(QNetworkOperation*)");
    if ( !clist || signalsBlocked() )
	return;
    typedef void (QObject::*RT0)();
    typedef void (QObject::*RT1)(QNetworkOperation*);
    RT0 r0;
    RT1 r1;
    QConnectionListIt it(*clist);
    QConnection   *c;
    QSenderObject *object;
    while ( (c=it.current()) ) {
	++it;
	object = (QSenderObject*)c->object();
	object->setSender( this );
	switch ( c->numArgs() ) {
	    case 0:
#ifdef Q_FP_CCAST_BROKEN
		r0 = reinterpret_cast<RT0>(*(c->member()));
#else
		r0 = (RT0)*(c->member());
#endif
		(object->*r0)();
		break;
	    case 1:
#ifdef Q_FP_CCAST_BROKEN
		r1 = reinterpret_cast<RT1>(*(c->member()));
#else
		r1 = (RT1)*(c->member());
#endif
		(object->*r1)(t0);
		break;
	}
    }
}

// SIGNAL dataTransferProgress
void QNetworkProtocol::dataTransferProgress( int t0, int t1, QNetworkOperation* t2 )
{
    // No builtin function for signal parameter type int,int,QNetworkOperation*
    QConnectionList *clist = receivers("dataTransferProgress(int,int,QNetworkOperation*)");
    if ( !clist || signalsBlocked() )
	return;
    typedef void (QObject::*RT0)();
    typedef void (QObject::*RT1)(int);
    typedef void (QObject::*RT2)(int,int);
    typedef void (QObject::*RT3)(int,int,QNetworkOperation*);
    RT0 r0;
    RT1 r1;
    RT2 r2;
    RT3 r3;
    QConnectionListIt it(*clist);
    QConnection   *c;
    QSenderObject *object;
    while ( (c=it.current()) ) {
	++it;
	object = (QSenderObject*)c->object();
	object->setSender( this );
	switch ( c->numArgs() ) {
	    case 0:
#ifdef Q_FP_CCAST_BROKEN
		r0 = reinterpret_cast<RT0>(*(c->member()));
#else
		r0 = (RT0)*(c->member());
#endif
		(object->*r0)();
		break;
	    case 1:
#ifdef Q_FP_CCAST_BROKEN
		r1 = reinterpret_cast<RT1>(*(c->member()));
#else
		r1 = (RT1)*(c->member());
#endif
		(object->*r1)(t0);
		break;
	    case 2:
#ifdef Q_FP_CCAST_BROKEN
		r2 = reinterpret_cast<RT2>(*(c->member()));
#else
		r2 = (RT2)*(c->member());
#endif
		(object->*r2)(t0, t1);
		break;
	    case 3:
#ifdef Q_FP_CCAST_BROKEN
		r3 = reinterpret_cast<RT3>(*(c->member()));
#else
		r3 = (RT3)*(c->member());
#endif
		(object->*r3)(t0, t1, t2);
		break;
	}
    }
}


const char *QNetworkOperation::className() const
{
    return "QNetworkOperation";
}

QMetaObject *QNetworkOperation::metaObj = 0;

void QNetworkOperation::initMetaObject()
{
    if ( metaObj )
	return;
    if ( qstrcmp(QObject::className(), "QObject") != 0 )
	badSuperclassWarning("QNetworkOperation","QObject");
    (void) staticMetaObject();
}

#ifndef QT_NO_TRANSLATION

QString QNetworkOperation::tr(const char* s)
{
    return qApp->translate( "QNetworkOperation", s, 0 );
}

QString QNetworkOperation::tr(const char* s, const char * c)
{
    return qApp->translate( "QNetworkOperation", s, c );
}

#endif // QT_NO_TRANSLATION

QMetaObject* QNetworkOperation::staticMetaObject()
{
    if ( metaObj )
	return metaObj;
    (void) QObject::staticMetaObject();
#ifndef QT_NO_PROPERTIES
#endif // QT_NO_PROPERTIES
    typedef void (QNetworkOperation::*m1_t0)();
    typedef void (QObject::*om1_t0)();
    m1_t0 v1_0 = &QNetworkOperation::deleteMe;
    om1_t0 ov1_0 = (om1_t0)v1_0;
    QMetaData *slot_tbl = QMetaObject::new_metadata(1);
    QMetaData::Access *slot_tbl_access = QMetaObject::new_metaaccess(1);
    slot_tbl[0].name = "deleteMe()";
    slot_tbl[0].ptr = (QMember)ov1_0;
    slot_tbl_access[0] = QMetaData::Private;
    metaObj = QMetaObject::new_metaobject(
	"QNetworkOperation", "QObject",
	slot_tbl, 1,
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
