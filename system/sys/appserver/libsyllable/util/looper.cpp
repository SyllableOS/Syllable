/*  libsyllable.so - the highlevel API library for Syllable
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *  Copyright (C) 2003 Syllable Team
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of version 2 of the GNU Library
 *  General Public License as published by the Free Software
 *  Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 *  MA 02111-1307, USA
 */

#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <malloc.h>
#include <cassert>
#include <atheos/types.h>

#include <atheos/semaphore.h>
#include <atheos/kernel.h>
#include <atheos/time.h>
#include <atheos/filesystem.h>
#include <atheos/msgport.h>
#include <gui/guidefines.h>
#include <util/looper.h>
#include <util/message.h>
#include <util/messagequeue.h>
#include <util/messagefilter.h>
#include <util/string.h>

using namespace os;

class Looper::Private
{
      public:
	Private( const String & cName ):m_cName( cName ), m_cDefaultMutex( cName.c_str() )
	{
		m_pcMutex = &m_cDefaultMutex;
	}

	String m_cName;
	TimerNode *m_pcFirstTimer;
	TimerNode *m_pcFirstRestartTimer;

	std::map <int, Handler * >m_cHandlerMap;
	Handler *m_pcDefaultHandler;
	int m_nHandlerCount;

	Message *m_pcCurrentMessage;
	MessageQueue *m_pcMsgQueue;

	int m_nID;
	int m_nPriority;
	thread_id m_hThread;
	port_id m_hPort;
	mutable Locker m_cDefaultMutex;
	Locker *m_pcMutex;
	Looper *m_pcNextLooper;
	bigtime_t m_nNextEvent;
	std::list <MessageFilter * >m_cCommonFilterList;
};

//-------------------- Static members   --------------------------------------

static Locker g_cLooperListMutex( "looper_list_lock" );
static std::map <Looper *, int >g_cLooperPtrMap;
static std::map <thread_id, Looper * >g_cLooperThreadMap;
static std::map <port_id, Looper * >g_cLooperPortMap;

Looper::TimerNode::TimerNode( Handler * pcHandler, int nID, bigtime_t nPeriode, bool bOneShot )
{
	m_nPeriode = nPeriode;
	m_nTimeout = get_system_time() + nPeriode;
	m_pcHandler = pcHandler;
	m_nID = nID;
	m_bOnShot = bOneShot;
}

/** The looper constructor.
 * \par Description:
 *	This is the only Looper constructor. It initiates the looper and
 *	create the needed message port and message queue but it does not
 *	spawn a new thread. To actually start the message loop you must
 *	call Run(). Before the constructor returns it will lock the looper
 *	by calling Lock(). This means that before any other threads are able
 *	to access it you must call Unlock().
 *
 *  \param pzName
 *	Passed down to the Handler::Handler() constructor.
 *  \param nPriority
 *	Stored for later usage as the looper-thread priority.
 *  \param nPortSize
 *	Maximum number of pending messages.
 *  \sa Handler::Handler()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

Looper::Looper( const String & cName, int nPriority, int nPortSize ):Handler( cName )
{
	static atomic_t nLastID = ATOMIC_INIT(1);

	m = new Private( cName );
	m->m_nID = atomic_inc_and_read( &nLastID );
	m->m_nPriority = nPriority;
	m->m_hThread = -1;
	m->m_hPort = create_port( ( String( "l:" ) + cName ).c_str(), nPortSize );
	m->m_pcDefaultHandler = NULL;
	m->m_pcFirstTimer = NULL;
	m->m_pcFirstRestartTimer = NULL;
	m->m_nNextEvent = INFINITE_TIMEOUT;

	m->m_pcCurrentMessage = NULL;
	m->m_pcMsgQueue = new MessageQueue();

	Lock();
	AddHandler( this );
	_AddLooper( this );
}

/** Looper destructor
 * \par Description:
 *	Free all resources allocated by the looper.
 * \note
 *	You shold normally not delete a looper with the "delete" operator.
 *	The looper thread will delete the object itself before it terminates.
 *	To get rid of a looper you should terminate it by calling Quit() or
 *	by sending it a M_QUIT message.
 * \par
 *	In some very rare cases it is still possible to delete the looper
 *	which is why the destructor is not private. If you just create the
 *	looper but never call Run() it is safe to delete the object. After
 *	calling Run() the looper thread are responsible for the looper object
 *	and deleting it will cause the application to crash!
 *  \sa Looper::Looper()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

Looper::~Looper()
{
	_RemoveLooper( this );
	RemoveHandler( this );
	delete_port( m->m_hPort );

	while( m->m_pcFirstTimer != NULL )
	{
		TimerNode *pcTimer = m->m_pcFirstTimer;

		m->m_pcFirstTimer = pcTimer->m_pcNext;
		delete pcTimer;
	}
	assert( m->m_pcFirstRestartTimer == NULL );
	delete( m->m_pcMsgQueue );
	delete m;
}


void Looper::_SetThread( thread_id hThread )
{
	m->m_hThread = hThread;
	if( hThread != -1 )
	{
		g_cLooperListMutex.Lock();
		g_cLooperThreadMap[hThread] = this;
		g_cLooperListMutex.Unlock();
	}
}

port_id Looper::_GetPort() const
{
	return ( m->m_hPort );
}

/** Rename the looper.
 * \par Description:
 *	Rename the looper. If the looper is already started the looper-thread
 *	will be renamed aswell. If there is no looper-thread yet the name
 *	will be remembered and will be used as the thread-name when the
 *	looper is started.
 * \par
 *	The looper names is mostly used for debugging and does not need to
 *	be unique.
 * \param cName
 *	The new looper name
 * \sa GetName()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void Looper::SetName( const String & cName )
{
	m->m_cName = cName;
	if( m->m_hThread != -1 )
	{
		rename_thread( m->m_hThread, cName.c_str() );
	}
}

/** Get the loopers name.
 * \par Description:
 *	Get the looper name as set by the constructor or the SetName()
 *	member.
 * \return The loopers name.
 * \sa SetName()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/
String Looper::GetName() const
{
	return ( m->m_cName );
}


/** See if the looper's message port is public.
 * \par Description:
 *	Retur
 * \return True if the message port is public, false otherwise.
 * \sa SetPublic()
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
bool Looper::IsPublic() const
{
	port_info	psInfo;
	
	if( get_port_info( m->m_hPort, &psInfo ) >= 0 ) {
		return ( psInfo.pi_flags & MSG_PORT_PUBLIC );
	} else {
		throw "x";	// FIXME: throw proper exception
	}
}

/** Make port public/private
 * \par Description:
 *	SetPublic() is used to decide whether the Looper's message port is to be
 *	public or not. A public message port can be accessed by other processes
 *	that only need to know the name of the message port.
 * \sa IsPublic(), os::Messenger
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
void Looper::SetPublic( bool bPublic )
{
	if( bPublic ) {
		make_port_public( m->m_hPort );
	} else {
		make_port_private( m->m_hPort );
	}
}

/** Add a timer to the looper.
 *  \par Description:
 *	Timers should be employed when you have a small task that should be
 *	run periodically or that you would like to schedule some point ahead
 *	in time. A timer is targeted at a spesific handler and will cause
 *	the os::Handler::TimerTick() member to be called each time the timer
 *	expire. You can add an arbritary number of timers to a looper.
 * \par
 *	The Handler::TimerTick() will be called by the looper thread with
 *	the looper locked.
 *  \note
 *	As with every callbacks from the looper thread, you should not do any
 *	long-lasting computing in the os::Handler::TimerTick() function. The looper
 *	will not be able to handle any messages until the function returns.
 *	This can cause the application to feel unresponsive. If you have any
 *	long-lasting tasks, you hould spawn a new thread to get the job done.
 *
 *  \param pcTarget
 *	Must be a valid ponter to a handler belonging to this looper.
 *	This is the handler that will get it's TimerTick() member called.
 *  \param nID
 *	An user defined ID that is passed to the TimerTick() function
 *	to make it possible to distinguish different timers.
 *  \param nPeriode
 *	Time in micro seconds before the timer fires. If not in
 *	one-shot mode, the timer will be rescheduled with the
 *	same delay until manually removed.
 *  \param bOneShot
 *	If true the timer will fire once, and then be removed.
 *	If false the timer will be continually rescheduled until
 *	removed by Looper::RemoveTimer().
 *
 *  \sa RemoveTimer(), os::Handler::TimerTick()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void Looper::AddTimer( Handler * pcTarget, int nID, bigtime_t nPeriode, bool bOneShot )
{
	RemoveTimer( pcTarget, nID );

	TimerNode *pcTimer = new TimerNode( pcTarget, nID, nPeriode, bOneShot );

	_InsertTimer( pcTimer );
}

/** Delete a timer.
 *  \par Description:
 *	When creating a timer in repeate mode you will propably at some time
 *	want to get rid of it. This is the way to go.
 *
 *  \param - pcTarget - The handler for which a timer is to be removed.
 *  \param - nID - The ID of the timer to be removed.
 *
 *  \return True if the timer was found, false otherwise.
 *  \sa AddTimer(), Handler::TimerTick()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

bool Looper::RemoveTimer( Handler * pcTarget, int nID )
{
	TimerNode **ppcTmp;

	for( ppcTmp = &m->m_pcFirstTimer; NULL != *ppcTmp; ppcTmp = &( *ppcTmp )->m_pcNext )
	{
		TimerNode *pcTimer = *ppcTmp;

		if( pcTimer->m_pcHandler == pcTarget && pcTimer->m_nID == nID )
		{
			*ppcTmp = pcTimer->m_pcNext;
			delete pcTimer;

			return ( true );
		}
	}
	for( ppcTmp = &m->m_pcFirstRestartTimer; NULL != *ppcTmp; ppcTmp = &( *ppcTmp )->m_pcNext )
	{
		TimerNode *pcTimer = *ppcTmp;

		if( pcTimer->m_pcHandler == pcTarget && pcTimer->m_nID == nID )
		{
			*ppcTmp = pcTimer->m_pcNext;
			delete pcTimer;

			return ( true );
		}
	}

	return ( false );
}

/** Obtain the low-level message port used by this looper.
 *  \par Description:
 *	Obtain the low-level message port used by this looper.
 *  \return The port ID of the internal message port.
 *  \sa GetMessageQueue(), GetCurrentMessage()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

port_id Looper::GetMsgPort() const
{
	return ( m->m_hPort );
}

/** Obtain the thread id of the looper thread.
 *  \par Description:
 *	Obtain the thread id of the looper thread.
 *  \return A valid thread ID if the loop is running, -1 otherwise.
 *  \sa GetProcess()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

thread_id Looper::GetThread() const
{
	return ( m->m_hThread );
}

/** Obtain the process ID of the loopers thread.
 *  \par Description:
 *	Obtain the process ID of the loopers thread.
 *  \return A valid process ID if the loop is running, -1 otherwise.
 *  \sa GetThread()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

proc_id Looper::GetProcess() const
{
	if( m->m_hThread == -1 )
	{
		return ( -1 );
	}
	else
	{
		return ( get_thread_proc( m->m_hThread ) );
	}
}

/** Lock the looper object.
 * \par Description:
 *	Before calling almost any other function on a looper or one of it's derived
 *	classes, you must lock it by calling this member. You can nest calls
 *	to Lock() within the same thread, each call to Lock() will then require a
 *	corresponding call to Unlock() to release the looper.
 * \par
 *	Looper::Lock() will not be affected by singnals that don't kill the thread.
 *	It will keep on retrying until it succed, or the failure was to someting
 *	else than a signal.
 * \note
 *	You should avoid keeping the looper locked for an extended periode of time
 *	since that will prevent it from handling any messages and can cause the
 *	application to feel unresponsive.
 *  \return Return 0, unless someting realy bad happened.
 *  \sa SafeLock(), Unlock()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

status_t Looper::Lock()
{
	status_t nError;

	for( ;; )
	{
		nError = m->m_pcMutex->Lock();

		if( nError < 0 )
		{
			dbprintf( "Looper::Lock() - failed to lock looper! Err = %s\n", strerror( errno ) );
			if( EINTR == errno )
			{
				continue;
			}
		}
		break;
	}
	return ( nError );
}

/** Lock the looper with a timeout.
 * \par Description:
 *	Lock the looper with a timeout. Like Lock() this member will attempt
 *	to acquire the loopers mutex but this version will only wait \p nTimeout
 *	micro seconds before timing out and fail with errno==ETIME.
 * \note
 *	Unlike the regular Lock() member this member will fail with errno==EINTR
 *	if the thread receive a POSIX signal while waiting to aquire the mutex
 *	or with errno==ETIME if the timeout is reached.
 * \par Warning:
 * \param nTimeout
 *	Maximum number of micro seconds to wait for the looper mutex
 *	to be available.
 * \return On success 0 is returned. On failure -1 is returned and a error
 *	code is written to the global variable \b errno.
 * \par Error codes:
 *	<dl>
 *	<dd>EINTR	An unignored POSIX signal was received.
 *	<dd>ETIME	The timeout expired before the mutex could be aquired.
 *	</dl>
 * \sa Lock(), SafeLock(), Unlock()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

status_t Looper::Lock( bigtime_t nTimeout )
{
	return ( m->m_pcMutex->Lock( nTimeout ) );
}

/** Attempt to lock the looper.
 * \par Description:
 *	SafeLock() have the same semantics as Lock() except that it verifies
 *	that the looper object is valid before attempting to lock it.
 * \par
 *	A common problem when locking a looper from an external thread is
 *	that the looper might terminate and become invalid between the
 *	time you obtain a pointer to it and the time you call Lock().
 *	This makes it impossible to lock a looper with Lock()
 *	from anything but the loopers own thread unless you have some other
 *	mechanism in place that guarantee that the looper will not go away
 *	before it is properly locked.
 * \par
 *	SafeLock() will verify that the looper is valid and lock it in
 *	a atomic fasion thus solving this problem. Note that this
 *	is much more expensive than the regular Lock() method so you
 *	should think twice before calling SafeLock().
 * \note
 *	When communicating between two loopers it is often better to
 *	do so asyncronously through messages than by locking the
 *	peer and do direct member calls.
 * \return On success 0 is returned. On error -1 is returned and an error code
 *	is written to the global variable \b errno.
 * \par Error codes:
 *	<dl>
 *	<dd>EINVAL	SafeLock() was called on an invalid looper.
 *	</dl>
 * \sa Lock(), Unlock()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

status_t Looper::SafeLock()
{
	status_t nError;

	g_cLooperListMutex.Lock();
	for( ;; )
	{
		std::map <Looper *, int >::iterator i = g_cLooperPtrMap.find( this );

		if( i != g_cLooperPtrMap.end() && ( *i ).second == m->m_nID )
		{
			nError = Lock( 5000LL );
			if( nError < 0 )
			{
				if( errno == ETIME || errno == EINTR )
				{
					g_cLooperListMutex.Unlock();
					snooze( 5000LL );
					g_cLooperListMutex.Lock();
					continue;
				}
				else
				{
					break;
				}
			}
			else
			{
				break;
			}
		}
		else
		{
			nError = -1;
			errno = EINVAL;
			break;
		}
	}
	g_cLooperListMutex.Unlock();
	return ( nError );
}



/** Unlock the looper.
 * \par Description:
 *	Unlock the looper.
 * \return Return 0, unless someting really bad happened.
 * \sa Lock(), SafeLock()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

status_t Looper::Unlock()
{
	return ( m->m_pcMutex->Unlock() );
}

/** Set a custom mutex to be used by the looper.
 * \par Description:
 *	Each looper own a mutex that is used to protect the looper object
 *	against other threads. Normally this mutex is handled entirely
 *	internally by the looper and is only accesses indirectly through
 *	the various Lock() and Unlock() members. In some very rare cases
 *	it might be necessarry to control this mutex from the application.
 * \par
 *	One possible usage for this is to "link" two loopers together
 *	to give the impression that they are both run by a single
 *	thread.  By fetching the mutex from one looper with GetMutex()
 *	and passing it to another looper through SetMutex() (or by
 *	creating a new semaphore and give it to both the loopers) you
 *	will assure that the two looper threads will never run
 *	simultanously. This should very rarly be used in native AtheOS
 *	application as it will cripple the concurrency you normally
 *	gain by using multiple loopers but it might sometimes be
 *	defendable. For example when porting applications from a
 *	singlethreaded environment it can often be very hard to
 *	rewrite all the code to work with multiple threads. Then
 *	SetMutex() is a possible way to chicken-out of the heavy
 *	multithreading normally employed by AtheOS.
 * \par
 *	In any case you should think twice before using this method.
 *	If you still think calling SetMutex() is a exellent idea
 *	think again.
 * \note
 *	The looper will automatically lock the looper before calling
 *	any event handlers (like os::Looper::DispatchMessage(),
 *	os::Handler::HandleMessage, and os::Handler::TimerTick) and
 *	unlock it again when the event handler return. If you replace
 *	the mutex from one of these members you must make sure the
 *	new mutex is locked exactly once so it will be unlocked
 *	correctly when the method returns.
 * \par
 *	You must also make sure that you don't introduce any
 *	race conditions with other threads as a result of the
 *	swap.
 * \note
 *	The mutex object will not be deleted when the looper dies. You are
 *	responsible to get rid of it after (but only after) the looper is
 *	dead. Also note that if you lend the internal mutex from one looper
 *	to another the second looper will be left high and dry if the first
 *	looper dies and delete it's internal semaphore.
 * \param pcMutex
 *	Pointer to the new mutex object or NULL to restore the default
 *	mutex
 * \sa GetMutex(), Lock(), Unlock(), os::Locker
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void Looper::SetMutex( Locker * pcMutex )
{
	if( pcMutex == NULL )
	{
		m->m_pcMutex = &m->m_cDefaultMutex;
	}
	else
	{
		m->m_pcMutex = pcMutex;
	}
}

/** Get a pointer to the loopers mutex.
 * \par Description:
 *	Returns the currently active mutex. This is normally a internal os::Locker
 *	object allocated by the looper itself but if the mutex has been replaced
 *	with SetMutex() the new mutex will be returned.
 * \return Pointer to the mutex currently used to protect the looper.
 * \sa SetMutex(), Lock(), Unlock(), os::Locker
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/


Locker *Looper::GetMutex() const
{
	return ( m->m_pcMutex );
}

/** Obtain the number of locks held on the looper.
 * \par Description:
 *	Return the nest count (number of locks held by the owner) from the
 *	internal mutex used to protect the looper.
 * \par Warning:
 *	This function is dangerous in that it is very hard to use without
 *	encountering race condition and are generally not very useful.
 *	The reason for including it is that it in some cases can be a useful
 *	help when debuggin a looper.
 * \return Loopers lock nest count.
 * \sa Lock(), SafeLock(), Unlock()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

int Looper::GetLockCount() const
{
	return ( m->m_pcMutex->GetLockCount() );
}

/** Obtaing the owner of the looper lock.
 * \par Description:
 *	Returns the thread ID of the thread currently holding the looper lock.
 *	If the looper is not locked, -1 is returned.
 * \par Warning:
 *	This function is dangerous in that it is very hard to use without
 *	encountering race condition and are generally not very useful.
 *	The reason for including it is that it in some cases can be a useful
 *	help when debuggin a looper.
 * \return The thread ID of the lock owner or -1 if the looper is not locked.
 * \sa GetLockCount(), Lock(), Unlock()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

thread_id Looper::GetLockingThread() const
{
	return ( m->m_pcMutex->GetOwner() );
}

/** Check if the looper lock is owned by the calling thread.
 * \par Description:
 *	Check if the looper lock is owned by the calling thread.
 * \par Warning:
 *	I can't think of any safe usage of this function, other than as a
 *	debugging aid.
 *  \return
 *  \sa Lock(), SafeLock(), Unlock()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

bool Looper::IsLocked() const
{
	return ( m->m_pcMutex->IsLocked() );
}

/** Obtain the message currently being processed.
 * \par Description:
 *	This member can be called from within os::Looper::DispatchMessage() or the
 *	active handlers os::Handler::HandleMessage() to learn whitch message is
 *	currently being processed. Given the fact that the very same message
 *	is passed to both this members you may wonder why you should ever
 *	need to call this function? The reason is that many of the classes
 *	derived from either os::Looper or os::Handler will convert known messages
 *	to specialized callbacks. These callbacks are normally not passed
 *	the entire message, only the most used elements. If you should find
 *	yourself in the unlucky situation of needing one of the not so much
 *	used elements this member is the solution.
 * \note
 *	The message will be automatically deleted when you return to the message
 *	loop. To avoid this you should use Looper::DetachCurrentMessage() instead.
 * \par
 *	Called outside the message loop this member will return NULL.
 *
 * \return Pointer to a message, or NULL if no message is currently being processed.
 * \sa DetachCurrentMessage(), DispatchMessage(), Handler::HandleMessage()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

Message *Looper::GetCurrentMessage() const
{
	return ( m->m_pcCurrentMessage );
}

/** Steal the current message.
 * \par Description:
 *	GetCurrentMessage() does basically the same job as GetCurrentMessage()
 *	except that it detatch the message from the looper, preventing it from being
 *	automatically deleted. You are responsible of getting rid of the message
 *	when it is no longer needed.
 * \return Pointer to a message, or NULL if no message is currently being processed.
 * \sa GetCurrentMessage()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

Message *Looper::DetachCurrentMessage()
{
	Message *pcMsg = m->m_pcCurrentMessage;

	m->m_pcCurrentMessage = NULL;
	return ( pcMsg );
}

/** The loopers message handling callback.
 * \par Description:
 *	When a message arrives, the looper extract's it from the queue and call
 *	this member with the message as a parameter. The default implementation
 *	will attempt to pass the message on to a Handler through it's
 *	os::Handler::HandleMessage() member. The Handler that should receive the
 *	message is selected as follows:
 * \par
 *	If the message targets a specific Handler, that Handler will receive the
 *	message. DispatchMessage() can determine wether the message had a final
 *	destination by examining the pcHandler argument. If not NULL it points at
 *	the handler targeted by the message.
 * \par
 *	If the message destination however is not fully qualified DispatchMessage()
 *	attempts to pass the message on to the Default handler (as set through the
 *	SetDefaultHandler()).
 * \par
 *	If there is no final destination and no default handler the Looper will
 *	handle the message itself by calling its own version of HandleMessage()
 *	(The looper is itself a os::Handler)
 * \par
 *	Not all messages are passed on to a handler. If the message code is M_QUIT
 *	the Looper::OkToQuit() member is called instead and if it return true
 *	looper object will be deleted and the looper thread terminated.
 * \par
 *	If you would like to handle certain messages directly by the looper, bypassing
 *	the normal scheduling you can overload DispatchMessage() to process messages
 *	before they are passed on to any handler. If you do so, you should call the
 *	loopers version of DispatchMessage() for each message you don't know how to
 *	handle.
 * \par
 *	Please note however that you should very rarly overload this member.
 *	It is normaly better to overload the HandleMessage() member and let
 *	the looper handle the message as any other handlers if you want to
 *	pass messages to the looper itself.
 * \note
 *	The looper is locked when DispatchMessage() is called.
 * \note
 *	Never do any lengthy operations in any hook members that are called
 *	from the looper thread if the looper is involved with the GUI (for
 *	example if the looper is a os::Window). The looper will not be
 *	able to dispatch messages until the hook returns so spending a
 *	long time in this member will make the GUI feel unresponsive.
 * \param pcMsg - Pointer to the received messge. Note that this message will be
 *		  deleted when DispatchMessage() returns, unless detatched from
 *		  the looper through DetachCurrentMessage().
 * \param pcHandler - Pointer to the handler targeted by this message. If the
 *		      message was not targeted at any spesific handler this
 *		      argument is NULL.
 * \sa SetDefaultHandler(), GetDefaultHandler(), PostMessage(), GetCurrentMessage()
 * \sa DetachCurrentMessage(), Handler::HandleMessage()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void Looper::DispatchMessage( Message * pcMsg, Handler * pcHandler )
{
	switch ( pcMsg->GetCode() )
	{
	case M_QUIT:
		if( OkToQuit() )
		{
			Unlock();
			delete this;

			exit_thread( 0 );
		}
		break;
	default:
		{
			if( pcHandler == NULL ) {
				pcHandler = m->m_pcDefaultHandler;
				if( pcHandler == NULL ) {
					pcHandler = this;
				}
			}
			
			FilterMessage( pcMsg, &pcHandler, &pcHandler->m_cFilterList );
			
			pcHandler->HandleMessage( pcMsg );
		}
	}
}

/** Obtain the internal message queue used by the looper.
 * \par Description:
 *	You should rarely need to examine the message queue yourself but it can
 *	be useful in certain situations. For example some messages might comes in such an
 *	overwelming amount and are of a nature that you can miss some of them
 *	without any harm. It is then useful to be able to look through the message
 *	queue when such a message arrive to check if there is more of them waiting
 *	and if so throw away the current message, and wait for the next to crawl its
 *	way down the queue. One example of such messages are the M_MOUSE_MOVED message.
 *	You will propably call SpoolMessages() before calling this message (but after
 *	locking the looper) to make sure the queue contains all messages sent to the
 *	looper.
 * \return A pointer to the internal MessageQueue object used by the looper.
 * \sa DispatchMessage(), PostMessage()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

MessageQueue *Looper::GetMessageQueue() const
{
	return ( m->m_pcMsgQueue );
}

/** Get the internal handler list
 * \par Description:
 *	You can use this member to gain read-only access to the internal stl
 *	map of handlers. The map key is an unique ID that is assigned to each
 *	handler in the system (unique accross process boundaries). This ID
 *	should be considered a implemntation detail and are of no value outside
 *	the looper/handler implemntation itself. You can still traverse the map
 *	to examine the handlers that currently populate the looper. You should
 *	always keep the looper locked while examining the map.
 * \return const reference to the internal stl map of handlers.
 * \sa FindHandler(), AddHandler(), RemoveHandler()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

const Looper::handler_map &Looper::GetHandlerMap() const
{
	return ( m->m_cHandlerMap );
}

/** Add a handler to the looper.
 * \par Description:
 *	Adds a handler to the looper. In order to receive messages and before
 *	it can be set as a default handler the handler must be added to a looper.
 *	The handler must not be member of any looper when added.
 * \param pcHandler - Pointer to the handler to be added.
 * \sa RemoveHandler(), SetDefaultHandler()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void Looper::AddHandler( Handler * pcHandler )
{
	AutoLocker _lock_( m->m_pcMutex );

	if( NULL == pcHandler->m_pcLooper )
	{
		m->m_cHandlerMap[pcHandler->m_nToken] = pcHandler;
		pcHandler->m_pcLooper = this;
	}
	else
	{
		dbprintf( "Warning: Attempt to add handler %d twice\n", pcHandler->m_nToken );
	}
}

/** Remove a handler previously added by AddHandler()
 * \par Description:
 *	Removes a handler from the looper. If the default handler is removed
 *	the default looper pointer as returned by GetDefaultHandler() will be
 *	set to NULL.
 * \param pcHandler
 *	The handler to remove
 * \return True if the Handler in fact was a member of the looper, false otherwise.
 * \sa AddHandler(), SetDefaultHandler(), GetDefaultHandler()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

bool Looper::RemoveHandler( Handler * pcHandler )
{
	if( this != pcHandler->m_pcLooper )
	{
		dbprintf( "Error : Attempt to remove handler not belonging to this looper\n" );
		return ( false );
	}

	handler_map::iterator i;

	AutoLocker _lock_( m->m_pcMutex );

	if( pcHandler == m->m_pcDefaultHandler )
	{
		m->m_pcDefaultHandler = NULL;
	}

	i = m->m_cHandlerMap.find( pcHandler->m_nToken );

	if( i == m->m_cHandlerMap.end() )
	{
		dbprintf( "Error: Looper::RemoveHandler() could not find handler %d in map!\n", pcHandler->m_nToken );
		return ( false );
	}
	else
	{
		m->m_cHandlerMap.erase( i );
		pcHandler->m_pcLooper = NULL;
		return ( true );
	}
	return ( false );
}

/** Search the looper for a named handler
 * \par Description:
 *	Return the handler named <pzName>, or NULL if there is no such handler
 *	added to the looper.
 * \param pzName
 *	The name to search for.
 * \return Pointer to the handler if one was found, NULL otherwise
 * \sa GetHandler(), GetHandlerCount(), GetHandlerIndex()
 * \sa AddHandler(), RemoveHandler()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

Handler *Looper::FindHandler( const String & cName ) const
{
	handler_map::const_iterator i;

	AutoLocker _lock_( m->m_pcMutex );

	for( i = m->m_cHandlerMap.begin(); i != m->m_cHandlerMap.end(  ); ++i )
	{
		Handler *pcHandler = ( *i ).second;

		if( pcHandler->m_cName == cName )
		{
			return ( pcHandler );
		}
	}
	dbprintf( "Error: Looper::FindHandler() Unable to find handler %s\n", cName.c_str() );
	return ( NULL );
}

/** Set the default target for incomming messages.
 * \par Description:
 *	Call this method to assign a handler as the default handler for the looper.
 *      When a handler become the default handler it will receive all messages
 *	that is not target directly at another handler.
 *	You can remove the current default handler by passing in a NULL pointer.
 * \note
 *	The handler must already be added to this looper with AddHandler()
 *	before it can be made the default handler of the looper.
 * \param pcHandler
 *	The handler to set as default, or NULL
 * \sa GetDefaultHandler(), DispatchMessage(), Handler::HandleMessage()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void Looper::SetDefaultHandler( Handler * pcHandler )
{
	AutoLocker _lock_( m->m_pcMutex );

	if( NULL == pcHandler || pcHandler->m_pcLooper == this )
	{
		m->m_pcDefaultHandler = pcHandler;
	}
}

/** Obtain the default handler for the looper.
 * \par Description:
 *	Return the handler last set by SetDefaultHandler() or NULL if there
 *	currently is no default handler
 * \return Pointer to the default handler.
 * \sa SetDefaultHandler()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

Handler *Looper::GetDefaultHandler() const
{
	return ( m->m_pcDefaultHandler );
}

/** Obtain the count of handlers added to this looper
 * \return Number of handlers currently attached to the looper
 * \sa GetHandler(), AddHandler(), RemoveHandler()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

int Looper::GetHandlerCount() const
{
	return ( m->m_cHandlerMap.size() );
}

/** Deliver a message to the looper.
 * \par Description:
 *	The message is posted to the looper with no absolute destination. This
 *	means that it will be handled by the default handler or the looper
 *	itself if there currently is no default handler.
 * \par Note:
 *	The caller is responsible for deleting the message (It is not deleted
 *	or kept by the looper)
 * \param pcMsg
 *	The message to post.
 * \return 0 if everyting went ok. If someting went wrong, a negative number is
 *	   retuned, and the errno variable is set.
 * \sa DispatchMessage()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

status_t Looper::PostMessage( Message * pcMsg )
{
	status_t nError;

	nError = pcMsg->_Post( GetMsgPort(), ~0 );
	if( nError < 0 )
	{
		dbprintf( "Error: Looper::PostMessage:1() failed to send message\n" );
	}
	return ( nError );
}

/** Deliver a message to the looper.
 * \par Description:
 *	Construct a message with the given code, and posts it to the looper
 *	by calling PostMessage( Message* pcMsg )
 * \param nCode
 *	The code that should be assigned to the sendt message
 * \return 0 if everyting went ok. If someting went wrong, a negative number is
 *	   retuned, and the errno variable is set.
 * \sa PostMessage( Message* pcMsg ),DispatchMessage()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

status_t Looper::PostMessage( uint32 nCode )
{
	Message cMsg( nCode );

	return ( PostMessage( &cMsg ) );
}

/** Deliver a message to the looper.
 * \par Description:
 *	Post a message to a specific handler. The message is targeted to the
 *	handler pointed at by pcHandler, and the reply is set to the handler
 *	pointed at by pcReplyTo.
 * \par Note:
 *	The caller is responsible for deleting the message (It is not deleted
 *	or kept by the looper)
 * \par
 *	You may not pass NULL as the target, but you can send a pointer to the
 *	looper itself to force it to handle the message.
 *
 * \param nCode
 *	The code that should be assigned to the sendt message
 * \param pcHandler
 *	Must point to a valid handler that will receive the message.
 * \param pcReplyTo
 *	If not NULL should point to the handler that should receive
 *	the reply sendt to the message.
 *
 * \return 0 if everyting went ok. If someting went wrong, a negative number is
 *	   retuned, and the errno variable is set.
 * \sa PostMessage( Message* pcMsg ), DispatchMessage()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

status_t Looper::PostMessage( Message * pcMsg, Handler * pcHandler, Handler * pcReplyHandler )
{
	Looper *pcLooper = NULL;
	port_id hReplyPort = -1;
	int nReplyToken = -1;

	if( NULL != pcReplyHandler )
	{
		pcLooper = pcReplyHandler->GetLooper();
		if( NULL != pcLooper )
		{
			hReplyPort = pcLooper->GetMsgPort();
			nReplyToken = pcReplyHandler->m_nToken;
		}
	}
	status_t nError = pcMsg->_Post( GetMsgPort(), pcHandler->m_nToken, hReplyPort, nReplyToken );

	if( nError < 0 )
	{
		dbprintf( "Looper::PostMessage:2() failed to send message\n" );
	}

	return ( nError );
}

/** Deliver a message to the looper.
 * \par Description:
 *	Construct a message with the given code, and posts it to a specific handler
 *	by calling PostMessage( Message *pcMsg, Handler *pcHandler, Handler* pcReplyHandler )
 *
 * \param nCode
 *	The code that should be assigned to the sendt message
 * \param pcHandler
 *	Must point to a valid handler that will receive the message.
 * \param pcReplyTo
 *	If not NULL should point to the handler that should receive
 *	the reply sendt to the message.
 *
 * \return 0 if everyting went ok. If someting went wrong, a negative number is
 *	   retuned, and the errno variable is set.
 * \sa PostMessage( Message *pcMsg, Handler *pcHandler, Handler* pcReplyHandler )
 * \sa DispatchMessage()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

status_t Looper::PostMessage( uint32 nCode, Handler * pcHandler, Handler * pcReplyTo )
{
	Message cMsg( nCode );

	return ( PostMessage( &cMsg, pcHandler, pcReplyTo ) );
}

/** Check if it is ok to break the loop.
 * \par Description:
 *	You can overload this function to affect how the looper will react to
 *	M_QUIT messages. When an M_QUIT message arrive the looper will call
 *	this function to figure out what to do. If it returns false the message
 *	is ignored. If it returns true the loop is terminated, the looper object
 *	deleted and the looper thread will exit.
 *
 * \return true if it is ok to terminate the looper, false otherwise.
 * \sa Quit(), PostMessage()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/


bool Looper::OkToQuit()
{
	return ( true );
}

/** Unconditionally terminate the looper.
 * \par Description:
 *	Calling quit will unconditionally terminate the looper (As opposed to
 *	posting an M_QUIT message that will politly call OkToQuit() before terminating
 *	the looper). If the call is made by the looper thread itself this member
 *	will delete the looper object, and call exit_thread(). If an external thread
 *	made the call, a M_TERMINATE message is sent, and the caller thread is
 *	blocked until the looper thread is dead. Either way the looper object
 *	gets deleted, and the looper thread dies.
 * \deprecated Use Terminate() instead.
 * \sa OkToQuit(), PostMessage()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/
void Looper::Quit()
{
	Terminate();
}


/** Unconditionally terminate the looper.
 * \par Description:
 *	Calling quit will unconditionally terminate the looper (As opposed to
 *	posting an M_QUIT message that will politly call OkToQuit() before terminating
 *	the looper). If the call is made by the looper thread itself this member
 *	will delete the looper object, and call exit_thread(). If an external thread
 *	made the call, a M_TERMINATE message is sent, and the caller thread is
 *	blocked until the looper thread is dead. Either way the looper object
 *	gets deleted, and the looper thread dies.
 * \note The Looper is checked for validity, so it IS safe to call Terminate()
 *	on an invalid pointer.
 * \sa OkToQuit(), PostMessage()
 *****************************************************************************/
void Looper::Terminate()
{
	if( SafeLock() == 0 ) {
		if( get_thread_id( NULL ) == m->m_hThread )
		{
			delete this;
	
			exit_thread( 0 );
		}
		else
		{
			if( m->m_hThread != -1 ) {
				Message cMsg( M_TERMINATE );
		
				PostMessage( &cMsg );
				
				Unlock();
				
				while( wait_for_thread( m->m_hThread ) < 0 )
				{
					if( errno != EINTR )
					{
						break;
					}
				}
			} else {
				delete this;
			}
		}
	}
}

/** Drain the low-level message port.
 * \par Description:
 *	SpoolMessage() will fetch all messages from the low-level message port
 *	deflatten the message objects, and add the os::Message objects to the
 *	internal message queue
 * \sa GetMessageQueue(), PostMessage()
 * \author Kurt Skauen (kurt@atheos.cx)
 * \internal Should have used the same temporary message buffer as the main loop!!
 *****************************************************************************/

void Looper::SpoolMessages()
{
	uint8 *pBuffer = new uint8[8192];
	uint32 nCode;

	while( get_msg_x( m->m_hPort, &nCode, pBuffer, 8192, 0 ) >= 0 )
	{
		_DecodeMessage( nCode, pBuffer );
	}
	delete[]pBuffer;
}

/** Start and unlock the looper.
 * \par Description:
 *	As mention in the description of the constructor, no thread are spawned
 *	there and the looper is locked. Calling Run() will spawn a new thread
 *	to run the message loop, and unlock the looper.
 * \par Note:
 *	Not all loopers will spawn a new thread. Ie. the Application class
 *	will overload the Run() member and lead the calling thread into the
 *	message loop. This means that when calling Run() on an application object
 *	it will not return until the loop terminates.
 * \return The thread ID of the looper thread.
 * \sa Quit(), OkToQuit(), PostMessage(), Lock(), Unlock()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

thread_id Looper::Run()
{
	m->m_hThread = spawn_thread( m->m_cName.c_str(), (void*)_Entry, m->m_nPriority, 0, this );
	if( m->m_hThread != -1 )
	{
		resume_thread( m->m_hThread );
		g_cLooperListMutex.Lock();
		g_cLooperThreadMap[m->m_hThread] = this;
		g_cLooperListMutex.Unlock();
	}
	Unlock();
	return ( m->m_hThread );
}

/** Wait for the looper thread to die
 * \par Description:
 *	This member can be called from an external thread to wait for the
 *	looper thread to die.
 * \note
 *	If this member return successfully it means that the looper is dead
 *	and no further members should be called on the looper object.
 * \return On success 0 is returned. On failure a negative value is returned
 *	and a error code is written to the global variable \b errno.
 * \sa Run(), Quit()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/


int Looper::Wait() const
{
	if( m->m_hThread < 0 )
	{
		errno = EINVAL;
		return ( -1 );
	}
	else
	{
		return ( wait_for_thread( m->m_hThread ) );
	}
}

/** Hook called each time the message queue is drained.
 * \par Description:
 *	Normally the looper thread is blocked on the internal message
 *	queue waiting for messages. When one or more messages
 *	arrive it will wake up and process the messages. When all
 *	messages is processed it will first call Idle() and then
 *	go back looking for more messages. If Idle() returned \b false
 *	the thread will block until the next message arrive and if
 *	Idle() returned \b true it will just look for new messages
 *	without blocking and call Idle() again (after processing new
 *	messages if any).
 * \par Note:
 *	The looper will not be able to process new messages until
 *	Idle() returns so it is very important to not do anything
 *	lengthy if the looper is part of for example an os::Window
 *	since that will make the application feel "unresponcive"
 *	to the user.
 *
 * \return
 *	The Idle() function should return \b true if it want to be
 *	called again imediatly after the looper has polled for new
 *	messages, or \b false if it don't want to be called until
 *	at least one more message has been processed.
 * \sa HandleMessage(), DispatchMessage()
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

bool Looper::Idle()
{
	return ( false );
}

/** Called by the looper thread before entering the message loop.
 * \par Description:
 *	This hook can be overloaded to do one-shot initialization
 *	of the looper that can not be done in the contructor.
 *	Started() is called from the loopers thread just before
 *	it starts processing messages.
 * \sa Idle(), Run()
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void Looper::Started()
{
}

void Looper::AddCommonFilter( MessageFilter * pcFilter )
{
	if( pcFilter == NULL )
	{
		dbprintf( "Error: Looper::AddCommonFilter() called with a NULL pointer\n" );
		return;
	}
	if( pcFilter->m_pcHandler != NULL )
	{
		if( pcFilter->m_bGlobalFilter )
		{
			static_cast < Looper * >( pcFilter->m_pcHandler )->RemoveCommonFilter( pcFilter );
		}
		else
		{
			pcFilter->m_pcHandler->RemoveFilter( pcFilter );
		}
	}
	pcFilter->m_cIterator = m->m_cCommonFilterList.insert( m->m_cCommonFilterList.end(), pcFilter );
	pcFilter->m_pcHandler = this;
	pcFilter->m_bGlobalFilter = true;
}

void Looper::RemoveCommonFilter( MessageFilter * pcFilter )
{
	if( pcFilter->m_pcHandler != this )
	{
		dbprintf( "Error: Looper::RemoveCommonFilter() attempt to remove filter not belonging to us\n" );
		return;
	}
	if( pcFilter->m_bGlobalFilter )
	{
		dbprintf( "Error: Looper::RemoveCommonFilter() attempt to remove a local filter using RemoveCommonFilter()\n" );
		return;
	}
	m->m_cCommonFilterList.erase( pcFilter->m_cIterator );
	pcFilter->m_pcHandler = NULL;
}

const MsgFilterList &Looper::GetCommonFilterList() const
{
	return ( m->m_cCommonFilterList );
}

bool Looper::FilterMessage( Message* pcMsg, Handler** ppcTarget, std::list<MessageFilter*>* pcFilterList )
{
	bool bDiscard = false;
	std::list<MessageFilter*>::iterator i;
			
	for( i = pcFilterList->begin(); i !=  pcFilterList->end(); i++ ) {
		Handler *pcFilterTarget = *ppcTarget;
		switch( (*i)->Filter( pcMsg, &pcFilterTarget ) ) {
			case MF_DISCARD_MESSAGE:
				bDiscard = true;
				break;
			case MF_DISPATCH_MESSAGE:
				*ppcTarget = pcFilterTarget;
				break;
		}
	}
	
	return (!bDiscard);
}

/******************************************************************************/

/****** Private functions *****************************************************/

/******************************************************************************/

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Looper::_AddLooper( Looper * pcLooper )
{
	g_cLooperListMutex.Lock();
	g_cLooperPtrMap[pcLooper] = pcLooper->m->m_nID;
	g_cLooperPortMap[pcLooper->m->m_hPort] = pcLooper;
//    pcLooper->m->m_pcNextLooper = s_pcFirstLooper;
//    s_pcFirstLooper = pcLooper;
	g_cLooperListMutex.Unlock();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Looper::_RemoveLooper( Looper * pcLooper )
{
//    Looper** ppcPtr;

	g_cLooperListMutex.Lock();

	std::map <Looper *, int >::iterator i = g_cLooperPtrMap.find( pcLooper );

	if( i != g_cLooperPtrMap.end() )
	{
		g_cLooperPtrMap.erase( i );
	}
	std::map <port_id, Looper * >::iterator j = g_cLooperPortMap.find( pcLooper->m->m_hPort );

	if( j != g_cLooperPortMap.end() )
	{
		g_cLooperPortMap.erase( j );
	}
	if( pcLooper->m->m_hThread != -1 )
	{
		std::map <thread_id, Looper * >::iterator k = g_cLooperThreadMap.find( pcLooper->m->m_hThread );

		if( k != g_cLooperThreadMap.end() )
		{
			g_cLooperThreadMap.erase( k );
		}
	}


/*    for ( ppcPtr = &s_pcFirstLooper ; NULL != *ppcPtr ; ppcPtr = &((*ppcPtr)->m->m_pcNextLooper) )
    {
	if ( *ppcPtr == pcLooper ) {
	    *ppcPtr = pcLooper->m->m_pcNextLooper;
	    break;
	}
    }*/
	g_cLooperListMutex.Unlock();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool Looper::_IsLooperValid( Looper * pcLooper )
{
	bool bValid = false;

	g_cLooperListMutex.Lock();

	std::map <Looper *, int >::iterator i = g_cLooperPtrMap.find( pcLooper );

	if( i != g_cLooperPtrMap.end() && ( *i ).second == pcLooper->m->m_nID )
	{
		bValid = true;
	}
	g_cLooperListMutex.Unlock();

	return ( bValid );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Looper *Looper::_GetLooperForThread( thread_id hThread )
{
	Looper *pcLooper = NULL;

	g_cLooperListMutex.Lock();

	std::map <thread_id, Looper * >::iterator i = g_cLooperThreadMap.find( hThread );

	if( i != g_cLooperThreadMap.end() )
	{
		pcLooper = ( *i ).second;
	}
	g_cLooperListMutex.Unlock();
	return ( pcLooper );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Looper *Looper::_GetLooperForPort( port_id hPort )
{
	Looper *pcLooper = NULL;

	g_cLooperListMutex.Lock();
	std::map <port_id, Looper * >::iterator i = g_cLooperPortMap.find( hPort );

	if( i != g_cLooperPortMap.end() )
	{
		pcLooper = ( *i ).second;
	}
	g_cLooperListMutex.Unlock();
	return ( pcLooper );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Handler *Looper::_FindHandler( int nToken ) const
{
	handler_map::const_iterator i;

	i = m->m_cHandlerMap.find( nToken );

	if( i == m->m_cHandlerMap.end() )
	{
		dbprintf( "Error: Looper::_FindHandler() could not find handler %d\n", nToken );
		return ( NULL );
	}
	else
	{
		return ( ( *i ).second );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Looper::_InsertTimer( TimerNode * pcTimer )
{
	TimerNode **ppcTmp;
	bool bWakeup = m->m_pcFirstTimer == NULL || pcTimer->m_nTimeout < m->m_pcFirstTimer->m_nTimeout;

	for( ppcTmp = &m->m_pcFirstTimer; NULL != *ppcTmp; ppcTmp = &( *ppcTmp )->m_pcNext )
	{
		if( pcTimer->m_nTimeout < ( *ppcTmp )->m_nTimeout )
		{
			pcTimer->m_pcNext = *ppcTmp;
			*ppcTmp = pcTimer;
			goto done;
		}
	}
	*ppcTmp = pcTimer;
	pcTimer->m_pcNext = NULL;
      done:
	if( bWakeup )
	{
		resume_thread( m->m_hThread );
	}
}

void Looper::_DecodeMessage( uint32 nCode, const void *pData )
{
	try
	{
		Message *pcMsg;

		if( nCode != M_NODE_MONITOR )
		{
			pcMsg = new Message( pData );
		}
		else
		{
			pcMsg = new Message( M_NODE_MONITOR );
			const NodeWatchEvent_s *psEvent = static_cast < const NodeWatchEvent_s * >( pData );

			pcMsg->AddInt32( "event", psEvent->nw_nEvent );
			pcMsg->AddInt32( "device", psEvent->nw_nDevice );
			pcMsg->AddInt64( "node", psEvent->nw_nNode );
			pcMsg->m_nTargetToken = int ( psEvent->nw_pUserData );

			switch ( psEvent->nw_nEvent )
			{
			case NWEVENT_CREATED:
			case NWEVENT_DELETED:
				pcMsg->AddInt64( "dir_node", psEvent->nw_nOldDir );
				pcMsg->AddString( "name", String( NWE_NAME( psEvent ), psEvent->nw_nNameLen ) );
				break;
			case NWEVENT_MOVED:
				pcMsg->AddInt64( "old_dir", psEvent->nw_nOldDir );
				pcMsg->AddInt64( "new_dir", psEvent->nw_nNewDir );
				pcMsg->AddString( "name", String( NWE_NAME( psEvent ), psEvent->nw_nNameLen ) );
				if( psEvent->nw_nPathLen > 0 )
				{
					if( psEvent->nw_nWhichPath == NWPATH_NEW )
					{
						pcMsg->AddString( "new_path", String( NWE_PATH( psEvent ), psEvent->nw_nPathLen ) );
					}
					else
					{
						pcMsg->AddString( "old_path", String( NWE_PATH( psEvent ), psEvent->nw_nPathLen ) );
					}
				}
				break;
			case NWEVENT_STAT_CHANGED:
				pcMsg->AddString( "name", String( NWE_NAME( psEvent ), psEvent->nw_nNameLen ) );
				break;
			case NWEVENT_ATTR_WRITTEN:
			case NWEVENT_ATTR_DELETED:
				pcMsg->AddString( "attr_name", String( NWE_NAME( psEvent ), psEvent->nw_nNameLen ) );
				break;
			case NWEVENT_FS_MOUNTED:
			case NWEVENT_FS_UNMOUNTED:
				break;
			}
		}
		if( m->m_pcMsgQueue->Lock() )
		{
			m->m_pcMsgQueue->AddMessage( pcMsg );
			m->m_pcMsgQueue->Unlock();
		}
		else
		{
			delete pcMsg;
		}
	}
	catch( std::exception & )
	{
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Looper::_Loop()
{
	void *pBuffer;

	pBuffer = malloc( 8192 );

	Lock();
	Started();
	bool bDoBlock = !Idle();

	Unlock();
	for( ;; )
	{
		bigtime_t nTimeOut;

		if( bDoBlock )
		{
			Lock();
			if( m->m_pcFirstTimer == NULL )
			{
				nTimeOut = 500000;
			}
			else
			{
				bigtime_t nCurTime = get_system_time();

				if( nCurTime < m->m_pcFirstTimer->m_nTimeout )
				{
					nTimeOut = m->m_pcFirstTimer->m_nTimeout - nCurTime;
				}
				else
				{
					nTimeOut = 0;
				}
			}
			Unlock();
		}
		else
		{
			nTimeOut = 0;
		}
		uint32 nCode;

		if( get_msg_x( m->m_hPort, &nCode, pBuffer, 8192, nTimeOut ) >= 0 )
		{
			_DecodeMessage( nCode, pBuffer );
		}

		Lock();
		bigtime_t nCurTime = get_system_time();

		m->m_pcFirstRestartTimer = NULL;

		while( m->m_pcFirstTimer != NULL && nCurTime >= m->m_pcFirstTimer->m_nTimeout )
		{
			TimerNode *pcTimer = m->m_pcFirstTimer;

			m->m_pcFirstTimer = pcTimer->m_pcNext;

			Handler *pcTarget = pcTimer->m_pcHandler;
			int nID = pcTimer->m_nID;

			if( pcTimer->m_bOnShot )
			{
				delete pcTimer;
			}
			else
			{
				pcTimer->m_nTimeout = nCurTime + pcTimer->m_nPeriode;
				pcTimer->m_pcNext = m->m_pcFirstRestartTimer;
				m->m_pcFirstRestartTimer = pcTimer;
			}
			pcTarget->TimerTick( nID );
		}
		while( m->m_pcFirstRestartTimer != NULL )
		{
			TimerNode *pcTimer = m->m_pcFirstRestartTimer;

			m->m_pcFirstRestartTimer = pcTimer->m_pcNext;
			_InsertTimer( pcTimer );
		}
		m->m_pcFirstRestartTimer = NULL;
		Unlock();

		while( get_msg_x( m->m_hPort, &nCode, pBuffer, 8192, 0 ) >= 0 )
		{
			_DecodeMessage( nCode, pBuffer );
		}

		for( ;; )
		{
			if( m->m_pcMsgQueue->Lock() == false )
			{
				dbprintf( "Error: Looper::_Loop() failed to lock message queue\n" );
				break;
			}
			m->m_pcCurrentMessage = m->m_pcMsgQueue->NextMessage();
			m->m_pcMsgQueue->Unlock();

			if( m->m_pcCurrentMessage == NULL )
			{
				break;
			}
			if( m->m_pcCurrentMessage->GetCode() == M_TERMINATE )
			{
				free( pBuffer );
				return;
			}
			if( m->m_pcCurrentMessage->GetCode() == M_QUIT )
			{
				if( OkToQuit() )
				{
					free( pBuffer );
					return;
				}
				else
				{
					continue;
				}
			}
			
			Handler *pcTarget;
			
			if( -1 != m->m_pcCurrentMessage->m_nTargetToken )
			{
				pcTarget = _FindHandler( m->m_pcCurrentMessage->m_nTargetToken );
			}
			else
			{
				pcTarget = NULL;
			}

			if( FilterMessage( m->m_pcCurrentMessage, &pcTarget, &m->m_cCommonFilterList ) ) {
				Lock();
				try
				{
					DispatchMessage( m->m_pcCurrentMessage, pcTarget );
				}
				catch( ... )
				{
					dbprintf( "Error: Looper::_Loop() uncaught exception in Looper::DispatchMessage()\n" );
				}
				Unlock();
			}

			delete m->m_pcCurrentMessage;

			m->m_pcCurrentMessage = NULL;
		}
		Lock();
		try
		{
			bDoBlock = !Idle();
		}
		catch( ... )
		{
			dbprintf( "Error: Looper::_Loop() uncaught exception in Looper::Idle()\n" );
		}
		Unlock();
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

uint32 Looper::_Entry( void *pData )
{
	Looper *pcThis = ( Looper * ) pData;

//      signal( SIGHUP, SIG_IGN );
//      signal( SIGINT, SIG_IGN );
//      signal( SIGQUIT, SIG_IGN );
	signal( SIGALRM, SIG_IGN );
	signal( SIGCHLD, SIG_IGN );
//      signal( SIGINT, SIG_IGN );
//      signal( SIGINT, SIG_IGN );

	pcThis->_Loop();

	delete pcThis;

	return ( 0 );
}

void Looper::__LO_reserved1__()
{
}
void Looper::__LO_reserved2__()
{
}
void Looper::__LO_reserved3__()
{
}
void Looper::__LO_reserved4__()
{
}
void Looper::__LO_reserved5__()
{
}
void Looper::__LO_reserved6__()
{
}
void Looper::__LO_reserved7__()
{
}
void Looper::__LO_reserved8__()
{
}

