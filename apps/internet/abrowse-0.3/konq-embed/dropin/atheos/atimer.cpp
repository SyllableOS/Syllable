
#include <qtimer.h>
#include "amutex.h"
#include <util/looper.h>


//static os::Looper* g_pcTimerLooper = NULL;

#define g_pcTimerLooper (GlobalMutex::GetCurLooper())

enum { ID_TIMER };



QTimer::QTimer( QObject *parent, const char *name ) : QObject( parent, name ), Handler( "timer" )
{
//    if ( g_pcTimerLooper == NULL ) {
//	g_pcTimerLooper = new os::Looper( "khtml_timer_looper" );
//	g_pcTimerLooper->Run();
//    }
    m_bIsActive = false;
    m_bDeleteWhenTrigged = false;
    m_bSingleShot        = false;
    g_pcTimerLooper->AddHandler( this );
}
    
QTimer::~QTimer()
{
    if ( m_bIsActive ) {
	g_pcTimerLooper->RemoveTimer( this, ID_TIMER );
    }
}

void QTimer::TimerTick( int /*nID*/ )
{
//    GetLooper()->Unlock();
//    GlobalMutex::Lock();
//    GetLooper()->Lock();
    GlobalMutex::PushLooper( GetLooper() );
    
    emit timeout();
    if ( m_bSingleShot ) {
	m_bIsActive = false;
    }
    if ( m_bDeleteWhenTrigged ) {
	delete this;
    }
    GlobalMutex::PopLooper();
//    GlobalMutex::Unlock();
}

bool QTimer::isActive() const
{
    return( m_bIsActive );
}

int QTimer::start( int msec, bool sshot )
{
    m_bIsActive = true;
    m_bSingleShot = sshot;
//    g_pcTimerLooper->Lock();
//    GlobalMutex::Lock();
    g_pcTimerLooper->AddTimer( this, ID_TIMER, bigtime_t(msec) * 1000LL, sshot );
//    GlobalMutex::Unlock();
//    g_pcTimerLooper->Unlock();
    return( 0 );
}

void QTimer::changeInterval( int msec )
{
//    g_pcTimerLooper->Lock();
//    GlobalMutex::Lock();
    if ( m_bIsActive ) {
	g_pcTimerLooper->RemoveTimer( this, ID_TIMER );
    }
    g_pcTimerLooper->AddTimer( this, ID_TIMER, bigtime_t(msec) * 1000LL, m_bSingleShot );
    m_bIsActive = true;
//    g_pcTimerLooper->Unlock();
//    GlobalMutex::Unlock();
}

void QTimer::stop()
{
//    GlobalMutex::Lock();
//    g_pcTimerLooper->Lock();
    if ( m_bIsActive ) {
	g_pcTimerLooper->RemoveTimer( this, ID_TIMER );
	m_bIsActive = false;
    }
//    g_pcTimerLooper->Unlock();
//    GlobalMutex::Unlock();
}

void QTimer::singleShot( int msec, QObject *receiver, const char *member )
{
    QTimer* pcTimer = new QTimer();
//    pcTimer->Connect( receiver, ID_TIMEOUT, nEvent );
    receiver->connect( pcTimer, SIGNAL( timeout() ), receiver, member );
    pcTimer->start( msec, true );
    pcTimer->m_bDeleteWhenTrigged = true;    
}

#include "atimer.moc"
