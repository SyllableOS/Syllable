/****************************************************************************
** QUrlOperator meta object code from reading C++ file 'qurloperator.h'
**
** Created: Fri Apr 20 02:02:04 2001
**      by: The Qt MOC ($Id$)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#if !defined(Q_MOC_OUTPUT_REVISION)
#define Q_MOC_OUTPUT_REVISION 9
#elif Q_MOC_OUTPUT_REVISION != 9
#error "Moc format conflict - please regenerate all moc files"
#endif

#include "qurloperator.h"
#include <qmetaobject.h>
#include <qapplication.h>



const char *QUrlOperator::className() const
{
    return "QUrlOperator";
}

QMetaObject *QUrlOperator::metaObj = 0;

void QUrlOperator::initMetaObject()
{
    if ( metaObj )
	return;
    if ( qstrcmp(QObject::className(), "QObject") != 0 )
	badSuperclassWarning("QUrlOperator","QObject");
    (void) staticMetaObject();
}

#ifndef QT_NO_TRANSLATION

QString QUrlOperator::tr(const char* s)
{
    return qApp->translate( "QUrlOperator", s, 0 );
}

QString QUrlOperator::tr(const char* s, const char * c)
{
    return qApp->translate( "QUrlOperator", s, c );
}

#endif // QT_NO_TRANSLATION

QMetaObject* QUrlOperator::staticMetaObject()
{
    if ( metaObj )
	return metaObj;
    (void) QObject::staticMetaObject();
#ifndef QT_NO_PROPERTIES
#endif // QT_NO_PROPERTIES
    typedef void (QUrlOperator::*m1_t0)(const QByteArray&,QNetworkOperation*);
    typedef void (QObject::*om1_t0)(const QByteArray&,QNetworkOperation*);
    typedef void (QUrlOperator::*m1_t1)(QNetworkOperation*);
    typedef void (QObject::*om1_t1)(QNetworkOperation*);
    typedef void (QUrlOperator::*m1_t2)();
    typedef void (QObject::*om1_t2)();
    typedef void (QUrlOperator::*m1_t3)(const QValueList<QUrlInfo>&);
    typedef void (QObject::*om1_t3)(const QValueList<QUrlInfo>&);
    m1_t0 v1_0 = &QUrlOperator::copyGotData;
    om1_t0 ov1_0 = (om1_t0)v1_0;
    m1_t1 v1_1 = &QUrlOperator::continueCopy;
    om1_t1 ov1_1 = (om1_t1)v1_1;
    m1_t2 v1_2 = &QUrlOperator::finishedCopy;
    om1_t2 ov1_2 = (om1_t2)v1_2;
    m1_t3 v1_3 = &QUrlOperator::addEntry;
    om1_t3 ov1_3 = (om1_t3)v1_3;
    QMetaData *slot_tbl = QMetaObject::new_metadata(4);
    QMetaData::Access *slot_tbl_access = QMetaObject::new_metaaccess(4);
    slot_tbl[0].name = "copyGotData(const QByteArray&,QNetworkOperation*)";
    slot_tbl[0].ptr = (QMember)ov1_0;
    slot_tbl_access[0] = QMetaData::Private;
    slot_tbl[1].name = "continueCopy(QNetworkOperation*)";
    slot_tbl[1].ptr = (QMember)ov1_1;
    slot_tbl_access[1] = QMetaData::Private;
    slot_tbl[2].name = "finishedCopy()";
    slot_tbl[2].ptr = (QMember)ov1_2;
    slot_tbl_access[2] = QMetaData::Private;
    slot_tbl[3].name = "addEntry(const QValueList<QUrlInfo>&)";
    slot_tbl[3].ptr = (QMember)ov1_3;
    slot_tbl_access[3] = QMetaData::Private;
    typedef void (QUrlOperator::*m2_t0)(const QValueList<QUrlInfo>&,QNetworkOperation*);
    typedef void (QObject::*om2_t0)(const QValueList<QUrlInfo>&,QNetworkOperation*);
    typedef void (QUrlOperator::*m2_t1)(QNetworkOperation*);
    typedef void (QObject::*om2_t1)(QNetworkOperation*);
    typedef void (QUrlOperator::*m2_t2)(QNetworkOperation*);
    typedef void (QObject::*om2_t2)(QNetworkOperation*);
    typedef void (QUrlOperator::*m2_t3)(const QUrlInfo&,QNetworkOperation*);
    typedef void (QObject::*om2_t3)(const QUrlInfo&,QNetworkOperation*);
    typedef void (QUrlOperator::*m2_t4)(QNetworkOperation*);
    typedef void (QObject::*om2_t4)(QNetworkOperation*);
    typedef void (QUrlOperator::*m2_t5)(QNetworkOperation*);
    typedef void (QObject::*om2_t5)(QNetworkOperation*);
    typedef void (QUrlOperator::*m2_t6)(const QByteArray&,QNetworkOperation*);
    typedef void (QObject::*om2_t6)(const QByteArray&,QNetworkOperation*);
    typedef void (QUrlOperator::*m2_t7)(int,int,QNetworkOperation*);
    typedef void (QObject::*om2_t7)(int,int,QNetworkOperation*);
    typedef void (QUrlOperator::*m2_t8)(const QList<QNetworkOperation>&);
    typedef void (QObject::*om2_t8)(const QList<QNetworkOperation>&);
    typedef void (QUrlOperator::*m2_t9)(int,const QString&);
    typedef void (QObject::*om2_t9)(int,const QString&);
    m2_t0 v2_0 = &QUrlOperator::newChildren;
    om2_t0 ov2_0 = (om2_t0)v2_0;
    m2_t1 v2_1 = &QUrlOperator::finished;
    om2_t1 ov2_1 = (om2_t1)v2_1;
    m2_t2 v2_2 = &QUrlOperator::start;
    om2_t2 ov2_2 = (om2_t2)v2_2;
    m2_t3 v2_3 = &QUrlOperator::createdDirectory;
    om2_t3 ov2_3 = (om2_t3)v2_3;
    m2_t4 v2_4 = &QUrlOperator::removed;
    om2_t4 ov2_4 = (om2_t4)v2_4;
    m2_t5 v2_5 = &QUrlOperator::itemChanged;
    om2_t5 ov2_5 = (om2_t5)v2_5;
    m2_t6 v2_6 = &QUrlOperator::data;
    om2_t6 ov2_6 = (om2_t6)v2_6;
    m2_t7 v2_7 = &QUrlOperator::dataTransferProgress;
    om2_t7 ov2_7 = (om2_t7)v2_7;
    m2_t8 v2_8 = &QUrlOperator::startedNextCopy;
    om2_t8 ov2_8 = (om2_t8)v2_8;
    m2_t9 v2_9 = &QUrlOperator::connectionStateChanged;
    om2_t9 ov2_9 = (om2_t9)v2_9;
    QMetaData *signal_tbl = QMetaObject::new_metadata(10);
    signal_tbl[0].name = "newChildren(const QValueList<QUrlInfo>&,QNetworkOperation*)";
    signal_tbl[0].ptr = (QMember)ov2_0;
    signal_tbl[1].name = "finished(QNetworkOperation*)";
    signal_tbl[1].ptr = (QMember)ov2_1;
    signal_tbl[2].name = "start(QNetworkOperation*)";
    signal_tbl[2].ptr = (QMember)ov2_2;
    signal_tbl[3].name = "createdDirectory(const QUrlInfo&,QNetworkOperation*)";
    signal_tbl[3].ptr = (QMember)ov2_3;
    signal_tbl[4].name = "removed(QNetworkOperation*)";
    signal_tbl[4].ptr = (QMember)ov2_4;
    signal_tbl[5].name = "itemChanged(QNetworkOperation*)";
    signal_tbl[5].ptr = (QMember)ov2_5;
    signal_tbl[6].name = "data(const QByteArray&,QNetworkOperation*)";
    signal_tbl[6].ptr = (QMember)ov2_6;
    signal_tbl[7].name = "dataTransferProgress(int,int,QNetworkOperation*)";
    signal_tbl[7].ptr = (QMember)ov2_7;
    signal_tbl[8].name = "startedNextCopy(const QList<QNetworkOperation>&)";
    signal_tbl[8].ptr = (QMember)ov2_8;
    signal_tbl[9].name = "connectionStateChanged(int,const QString&)";
    signal_tbl[9].ptr = (QMember)ov2_9;
    metaObj = QMetaObject::new_metaobject(
	"QUrlOperator", "QObject",
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

// SIGNAL newChildren
void QUrlOperator::newChildren( const QValueList<QUrlInfo>& t0, QNetworkOperation* t1 )
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

// SIGNAL finished
void QUrlOperator::finished( QNetworkOperation* t0 )
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
void QUrlOperator::start( QNetworkOperation* t0 )
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

// SIGNAL createdDirectory
void QUrlOperator::createdDirectory( const QUrlInfo& t0, QNetworkOperation* t1 )
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
void QUrlOperator::removed( QNetworkOperation* t0 )
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
void QUrlOperator::itemChanged( QNetworkOperation* t0 )
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

// SIGNAL data
void QUrlOperator::data( const QByteArray& t0, QNetworkOperation* t1 )
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

// SIGNAL dataTransferProgress
void QUrlOperator::dataTransferProgress( int t0, int t1, QNetworkOperation* t2 )
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

// SIGNAL startedNextCopy
void QUrlOperator::startedNextCopy( const QList<QNetworkOperation>& t0 )
{
    // No builtin function for signal parameter type const QList<QNetworkOperation>&
    QConnectionList *clist = receivers("startedNextCopy(const QList<QNetworkOperation>&)");
    if ( !clist || signalsBlocked() )
	return;
    typedef void (QObject::*RT0)();
    typedef void (QObject::*RT1)(const QList<QNetworkOperation>&);
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

// SIGNAL connectionStateChanged
void QUrlOperator::connectionStateChanged( int t0, const QString& t1 )
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
