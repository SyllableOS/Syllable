
#include <qguardedptr.h>
#include <khtml/atheos.h>

enum { ID_OBJECT_DELETED };

QGuardedPtrPrivate::QGuardedPtrPrivate( QObject* o)
    : obj( o )
{
    if ( obj ) {
	obj->Connect( this, ID_QOBJECT_DELETED, ID_OBJECT_DELETED );
    }
}


QGuardedPtrPrivate::~QGuardedPtrPrivate()
{
}


void QGuardedPtrPrivate::DispatchEvent( ConnectTransmitter* /*pcSource*/, int nEvent, os::Message* /*pcMsg*/ )
{
    if ( nEvent == ID_OBJECT_DELETED ) {
	obj = 0;
    } else {
	printf( "Warning: QGuardedPtrPrivate::DispatchEvent() got unknown event %d\n", nEvent );
    }
}
