/****************************************************************************
** KIO::Job meta object code from reading C++ file 'jobclasses.h'
**
** Created: Sat Sep 15 13:10:54 2001
**      by: The Qt MOC ($Id$)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#if !defined(Q_MOC_OUTPUT_REVISION)
#define Q_MOC_OUTPUT_REVISION 9
#elif Q_MOC_OUTPUT_REVISION != 9
#error "Moc format conflict - please regenerate all moc files"
#endif

#include "jobclasses.h"
#include <qmetaobject.h>
#include <qapplication.h>



const char *KIO::Job::className() const
{
    return "KIO::Job";
}

QMetaObject *KIO::Job::metaObj = 0;

void KIO::Job::initMetaObject()
{
    if ( metaObj )
	return;
    if ( qstrcmp(QObject::className(), "QObject") != 0 )
	badSuperclassWarning("KIO::Job","QObject");
    (void) staticMetaObject();
}

#ifndef QT_NO_TRANSLATION

QString KIO::Job::tr(const char* s)
{
    return qApp->translate( "KIO::Job", s, 0 );
}

QString KIO::Job::tr(const char* s, const char * c)
{
    return qApp->translate( "KIO::Job", s, c );
}

#endif // QT_NO_TRANSLATION

QMetaObject* KIO::Job::staticMetaObject()
{
    if ( metaObj )
	return metaObj;
    (void) QObject::staticMetaObject();
#ifndef QT_NO_PROPERTIES
#endif // QT_NO_PROPERTIES
    QMetaData::Access *slot_tbl_access = 0;
    typedef void (KIO::Job::*m2_t0)(KIO::Job*);
    typedef void (QObject::*om2_t0)(KIO::Job*);
    typedef void (KIO::Job::*m2_t1)(KIO::Job*,const QString&);
    typedef void (QObject::*om2_t1)(KIO::Job*,const QString&);
    typedef void (KIO::Job::*m2_t2)(KIO::Job*);
    typedef void (QObject::*om2_t2)(KIO::Job*);
    typedef void (KIO::Job::*m2_t3)(KIO::Job*,unsigned long);
    typedef void (QObject::*om2_t3)(KIO::Job*,unsigned long);
    typedef void (KIO::Job::*m2_t4)(KIO::Job*,unsigned long);
    typedef void (QObject::*om2_t4)(KIO::Job*,unsigned long);
    typedef void (KIO::Job::*m2_t5)(KIO::Job*,unsigned long);
    typedef void (QObject::*om2_t5)(KIO::Job*,unsigned long);
    typedef void (KIO::Job::*m2_t6)(KIO::Job*,unsigned long);
    typedef void (QObject::*om2_t6)(KIO::Job*,unsigned long);
    m2_t0 v2_0 = &KIO::Job::result;
    om2_t0 ov2_0 = (om2_t0)v2_0;
    m2_t1 v2_1 = &KIO::Job::infoMessage;
    om2_t1 ov2_1 = (om2_t1)v2_1;
    m2_t2 v2_2 = &KIO::Job::connect;
    om2_t2 ov2_2 = (om2_t2)v2_2;
    m2_t3 v2_3 = &KIO::Job::percent;
    om2_t3 ov2_3 = (om2_t3)v2_3;
    m2_t4 v2_4 = &KIO::Job::totalSize;
    om2_t4 ov2_4 = (om2_t4)v2_4;
    m2_t5 v2_5 = &KIO::Job::processedSize;
    om2_t5 ov2_5 = (om2_t5)v2_5;
    m2_t6 v2_6 = &KIO::Job::speed;
    om2_t6 ov2_6 = (om2_t6)v2_6;
    QMetaData *signal_tbl = QMetaObject::new_metadata(7);
    signal_tbl[0].name = "result(KIO::Job*)";
    signal_tbl[0].ptr = (QMember)ov2_0;
    signal_tbl[1].name = "infoMessage(KIO::Job*,const QString&)";
    signal_tbl[1].ptr = (QMember)ov2_1;
    signal_tbl[2].name = "connect(KIO::Job*)";
    signal_tbl[2].ptr = (QMember)ov2_2;
    signal_tbl[3].name = "percent(KIO::Job*,unsigned long)";
    signal_tbl[3].ptr = (QMember)ov2_3;
    signal_tbl[4].name = "totalSize(KIO::Job*,unsigned long)";
    signal_tbl[4].ptr = (QMember)ov2_4;
    signal_tbl[5].name = "processedSize(KIO::Job*,unsigned long)";
    signal_tbl[5].ptr = (QMember)ov2_5;
    signal_tbl[6].name = "speed(KIO::Job*,unsigned long)";
    signal_tbl[6].ptr = (QMember)ov2_6;
    metaObj = QMetaObject::new_metaobject(
	"KIO::Job", "QObject",
	0, 0,
	signal_tbl, 7,
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

// SIGNAL result
void KIO::Job::result( KIO::Job* t0 )
{
    // No builtin function for signal parameter type KIO::Job*
    QConnectionList *clist = receivers("result(KIO::Job*)");
    if ( !clist || signalsBlocked() )
	return;
    typedef void (QObject::*RT0)();
    typedef void (QObject::*RT1)(KIO::Job*);
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

// SIGNAL infoMessage
void KIO::Job::infoMessage( KIO::Job* t0, const QString& t1 )
{
    // No builtin function for signal parameter type KIO::Job*,const QString&
    QConnectionList *clist = receivers("infoMessage(KIO::Job*,const QString&)");
    if ( !clist || signalsBlocked() )
	return;
    typedef void (QObject::*RT0)();
    typedef void (QObject::*RT1)(KIO::Job*);
    typedef void (QObject::*RT2)(KIO::Job*,const QString&);
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

// SIGNAL connect
void KIO::Job::connect( KIO::Job* t0 )
{
    // No builtin function for signal parameter type KIO::Job*
    QConnectionList *clist = receivers("connect(KIO::Job*)");
    if ( !clist || signalsBlocked() )
	return;
    typedef void (QObject::*RT0)();
    typedef void (QObject::*RT1)(KIO::Job*);
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

// SIGNAL percent
void KIO::Job::percent( KIO::Job* t0, unsigned long t1 )
{
    // No builtin function for signal parameter type KIO::Job*,unsigned long
    QConnectionList *clist = receivers("percent(KIO::Job*,unsigned long)");
    if ( !clist || signalsBlocked() )
	return;
    typedef void (QObject::*RT0)();
    typedef void (QObject::*RT1)(KIO::Job*);
    typedef void (QObject::*RT2)(KIO::Job*,unsigned long);
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

// SIGNAL totalSize
void KIO::Job::totalSize( KIO::Job* t0, unsigned long t1 )
{
    // No builtin function for signal parameter type KIO::Job*,unsigned long
    QConnectionList *clist = receivers("totalSize(KIO::Job*,unsigned long)");
    if ( !clist || signalsBlocked() )
	return;
    typedef void (QObject::*RT0)();
    typedef void (QObject::*RT1)(KIO::Job*);
    typedef void (QObject::*RT2)(KIO::Job*,unsigned long);
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

// SIGNAL processedSize
void KIO::Job::processedSize( KIO::Job* t0, unsigned long t1 )
{
    // No builtin function for signal parameter type KIO::Job*,unsigned long
    QConnectionList *clist = receivers("processedSize(KIO::Job*,unsigned long)");
    if ( !clist || signalsBlocked() )
	return;
    typedef void (QObject::*RT0)();
    typedef void (QObject::*RT1)(KIO::Job*);
    typedef void (QObject::*RT2)(KIO::Job*,unsigned long);
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

// SIGNAL speed
void KIO::Job::speed( KIO::Job* t0, unsigned long t1 )
{
    // No builtin function for signal parameter type KIO::Job*,unsigned long
    QConnectionList *clist = receivers("speed(KIO::Job*,unsigned long)");
    if ( !clist || signalsBlocked() )
	return;
    typedef void (QObject::*RT0)();
    typedef void (QObject::*RT1)(KIO::Job*);
    typedef void (QObject::*RT2)(KIO::Job*,unsigned long);
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


const char *KIO::SimpleJob::className() const
{
    return "KIO::SimpleJob";
}

QMetaObject *KIO::SimpleJob::metaObj = 0;

void KIO::SimpleJob::initMetaObject()
{
    if ( metaObj )
	return;
    if ( qstrcmp(KIO::Job::className(), "KIO::Job") != 0 )
	badSuperclassWarning("KIO::SimpleJob","KIO::Job");
    (void) staticMetaObject();
}

#ifndef QT_NO_TRANSLATION

QString KIO::SimpleJob::tr(const char* s)
{
    return qApp->translate( "KIO::SimpleJob", s, 0 );
}

QString KIO::SimpleJob::tr(const char* s, const char * c)
{
    return qApp->translate( "KIO::SimpleJob", s, c );
}

#endif // QT_NO_TRANSLATION

QMetaObject* KIO::SimpleJob::staticMetaObject()
{
    if ( metaObj )
	return metaObj;
    (void) KIO::Job::staticMetaObject();
#ifndef QT_NO_PROPERTIES
#endif // QT_NO_PROPERTIES
    QMetaData::Access *slot_tbl_access = 0;
    metaObj = QMetaObject::new_metaobject(
	"KIO::SimpleJob", "KIO::Job",
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


const char *KIO::TransferJob::className() const
{
    return "KIO::TransferJob";
}

QMetaObject *KIO::TransferJob::metaObj = 0;

void KIO::TransferJob::initMetaObject()
{
    if ( metaObj )
	return;
    if ( qstrcmp(KIO::SimpleJob::className(), "KIO::SimpleJob") != 0 )
	badSuperclassWarning("KIO::TransferJob","KIO::SimpleJob");
    (void) staticMetaObject();
}

#ifndef QT_NO_TRANSLATION

QString KIO::TransferJob::tr(const char* s)
{
    return qApp->translate( "KIO::TransferJob", s, 0 );
}

QString KIO::TransferJob::tr(const char* s, const char * c)
{
    return qApp->translate( "KIO::TransferJob", s, c );
}

#endif // QT_NO_TRANSLATION

QMetaObject* KIO::TransferJob::staticMetaObject()
{
    if ( metaObj )
	return metaObj;
    (void) KIO::SimpleJob::staticMetaObject();
#ifndef QT_NO_PROPERTIES
#endif // QT_NO_PROPERTIES
    QMetaData::Access *slot_tbl_access = 0;
    typedef void (KIO::TransferJob::*m2_t0)(KIO::Job*,const KURL&);
    typedef void (QObject::*om2_t0)(KIO::Job*,const KURL&);
    typedef void (KIO::TransferJob::*m2_t1)(KIO::Job*,const QByteArray&);
    typedef void (QObject::*om2_t1)(KIO::Job*,const QByteArray&);
    typedef void (KIO::TransferJob::*m2_t2)(KIO::Job*,const QString&);
    typedef void (QObject::*om2_t2)(KIO::Job*,const QString&);
    m2_t0 v2_0 = &KIO::TransferJob::redirection;
    om2_t0 ov2_0 = (om2_t0)v2_0;
    m2_t1 v2_1 = &KIO::TransferJob::data;
    om2_t1 ov2_1 = (om2_t1)v2_1;
    m2_t2 v2_2 = &KIO::TransferJob::mimetype;
    om2_t2 ov2_2 = (om2_t2)v2_2;
    QMetaData *signal_tbl = QMetaObject::new_metadata(3);
    signal_tbl[0].name = "redirection(KIO::Job*,const KURL&)";
    signal_tbl[0].ptr = (QMember)ov2_0;
    signal_tbl[1].name = "data(KIO::Job*,const QByteArray&)";
    signal_tbl[1].ptr = (QMember)ov2_1;
    signal_tbl[2].name = "mimetype(KIO::Job*,const QString&)";
    signal_tbl[2].ptr = (QMember)ov2_2;
    metaObj = QMetaObject::new_metaobject(
	"KIO::TransferJob", "KIO::SimpleJob",
	0, 0,
	signal_tbl, 3,
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

// SIGNAL redirection
void KIO::TransferJob::redirection( KIO::Job* t0, const KURL& t1 )
{
    // No builtin function for signal parameter type KIO::Job*,const KURL&
    QConnectionList *clist = receivers("redirection(KIO::Job*,const KURL&)");
    if ( !clist || signalsBlocked() )
	return;
    typedef void (QObject::*RT0)();
    typedef void (QObject::*RT1)(KIO::Job*);
    typedef void (QObject::*RT2)(KIO::Job*,const KURL&);
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

// SIGNAL data
void KIO::TransferJob::data( KIO::Job* t0, const QByteArray& t1 )
{
    // No builtin function for signal parameter type KIO::Job*,const QByteArray&
    QConnectionList *clist = receivers("data(KIO::Job*,const QByteArray&)");
    if ( !clist || signalsBlocked() )
	return;
    typedef void (QObject::*RT0)();
    typedef void (QObject::*RT1)(KIO::Job*);
    typedef void (QObject::*RT2)(KIO::Job*,const QByteArray&);
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

// SIGNAL mimetype
void KIO::TransferJob::mimetype( KIO::Job* t0, const QString& t1 )
{
    // No builtin function for signal parameter type KIO::Job*,const QString&
    QConnectionList *clist = receivers("mimetype(KIO::Job*,const QString&)");
    if ( !clist || signalsBlocked() )
	return;
    typedef void (QObject::*RT0)();
    typedef void (QObject::*RT1)(KIO::Job*);
    typedef void (QObject::*RT2)(KIO::Job*,const QString&);
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
