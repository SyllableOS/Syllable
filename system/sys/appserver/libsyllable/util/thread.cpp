//#include <sys/time.h>
#include <atheos/kernel.h>
#include <signal.h>
#include <errno.h>

#include <util/thread.h>

using namespace os;

int32 Thread::_Entry( void *pvoid )
{
	Thread *pThis = ( Thread * ) pvoid;

	if( !pThis )
		return -1;

	return pThis->Run();
}

/** Constructor
 * \par		Description:
 *		Create a new thread.
 * \param	pzName Name of the thread.
 * \param	nPriority Thread priority (IDLE_PRIORITY, LOW_PRIORITY,
 *		NORMAL_PRIORITY, DISPLAY_PRIORITY, URGENT_DISPLAY_PRIORITY or
 *		REALTIME_PRIORITY).
 * \note	Threads are created in a suspended state. To start a thread's
 *		execution, call Start().
 * \sa Start()
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
Thread::Thread( const char *pzName, int nPriority, int nStackSize )
{
	m_iThread = -1;
	Initialize( pzName, nPriority, nStackSize );
}

Thread::~Thread( void )
{
	Terminate();
}

/** Suspend execution
 * \par	Description:
 *	Suspends thread execution so that it can be resumed using Start().
 * \sa Start(), Terminate()
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
void Thread::Stop( void )
{
	if( m_iThread < 0 )
		throw ThreadException ( "Invalid thread ID", EINVALIDTHREAD );

	suspend_thread( m_iThread );
}

/** Begin/Resume execution
 * \par	Description:
 *	Resume execution of a previously stopped thread. The thread will
 *	continue to run from where it were when it was stopped.
 *	New threads defaults to suspended state, so a newly created thread must
 *	be started with Start().
 * \sa Stop()
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
void Thread::Start( void )
{
	if( m_iThread < 0 )
		throw ThreadException ( "Invalid thread ID", EINVALIDTHREAD );

	resume_thread( m_iThread );
}

/** Wait for thread
 * \par	Description:
 *	Wait for the thread to finish. This function will not return until the
 *	thread has finished executing.
 * \note Do not call this method from the thread's own code.
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
void Thread::WaitFor( void )
{
	if( m_iThread >= 0 )
	{
		wait_for_thread( m_iThread );
	}
}

/** Kill thread unconditionally
 * \par	Description:
 *	Send a KILL signal to the thread and terminate it immediately.
 *	A terminated thread cannot resume execution, but it can be reset and
 *	restarted.
 * \sa Stop(), Initialize()
 * \note The thread will be invalid after calling this method. The only valid
 *	operation on the object will be deleting it or calling Initialize() to
 *	reset the thread.
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
void Thread::Terminate( void )
{
	if( m_iThread < 0 )
		return;

	kill( m_iThread, SIGKILL );	//SIGABRT SIGSTOP SIGKILL SIGTERM

	m_iThread = -1;
}

/** Reset thread
 * \par	Description:
 *	Re-initializes a thread and prepares it for execution, after it has
 *	been terminated with Terminate().
 *	If the thread has not been terminated prior to this call, it will be
 *	terminated and then initialized.
 * \param	pzName Name of the thread.
 * \param	nPriority Thread priority (IDLE_PRIORITY, LOW_PRIORITY,
 *		NORMAL_PRIORITY, DISPLAY_PRIORITY, URGENT_DISPLAY_PRIORITY or
 *		REALTIME_PRIORITY).
 * \param	nStackSize Stack size, 0 means default (currently 128k). Minimum
 *		stack size is currently 32k.
 * \sa Terminate()
 * \note	The thread is reset to a suspended state. To start a thread's
 *		execution, call Start().
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
void Thread::Initialize( const char *pzName, int nPriority, int nStackSize )
{
	if( m_iThread >= 0 )
		Terminate();

	m_iThread = spawn_thread( pzName, (void*)_Entry, nPriority, nStackSize, this );
	if( m_iThread < 0 )
		throw ThreadException ( "spawn_thread() failed", errno );
}

/** Temporarily suspend thread execution
 * \par	Description:
 *	Temporarily suspend thread execution. This method is only intended for
 *	use from the thread's own code.
 * \param nMicros Delay time in microseconds.
 * \note Although the time is specified in microseconds, you can't expect that
 *	kind of precision. The actual delay time depends on how the kernel's
 *	timer interrupts are configured.
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
void Thread::Delay( uint32 nMicros )
{
	snooze( nMicros );
}

/** Set priority
 * \par	Description:
 *	Set thread priority. Throws ThreadException if the thread is invalid.
 * \param nPriority Thread priority (IDLE_PRIORITY, LOW_PRIORITY,
 *	NORMAL_PRIORITY, DISPLAY_PRIORITY, URGENT_DISPLAY_PRIORITY or
 *	REALTIME_PRIORITY).
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
void Thread::SetPriority( int nPriority )
{
	if( m_iThread < 0 )
		throw ThreadException ( "Invalid thread ID", EINVALIDTHREAD );

	set_thread_priority( m_iThread, nPriority );
}

/** Get priority
 * \par	Description:
 *	Returns priority or throws ThreadException if the thread is invalid.
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
int Thread::GetPriority()
{
	if( m_iThread < 0 )
		throw ThreadException ( "Invalid thread ID", EINVALIDTHREAD );

	thread_info sTI;

	get_thread_info( m_iThread, &sTI );

	return sTI.ti_priority;
}

/** Get thread ID
 * \par	Description:
 *	Returns thread ID or -1 if the thread is invalid.
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
thread_id Thread::GetThreadId()
{
	return m_iThread;
}

/** Get process ID
 * \par	Description:
 *	Returns process ID or -1 if the thread is invalid.
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
proc_id Thread::GetProcessId()
{
	if( m_iThread < 0 )
		return -1;
	return get_thread_proc( m_iThread );
}
