/*  thread.h - C++ wrapper for Syllable threads
 *  Copyright (C) 2002 Henrik Isaksson
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
 
#ifndef __F_UTIL_THREAD_H__
#define __F_UTIL_THREAD_H__

#include <string>
#include <atheos/threads.h>
#include <util/exceptions.h>

namespace os {
#if 0
} // Fool Emacs autoindent
#endif

/** Thread class
 * \ingroup util
 * \par Description:
 *	This class is a wrapper class for Syllable threads, meant to simplify
 *	the use of threads.
 * \par
 *	To use it, you create a subclass of Thread, and override the Run()
 *	method with the code you want to run in the thread. Then simply
 *	instansiate your new class, and call Start() on it.
 * \par Example:
 * \code
 * #include <util/thread.h>
 * #include <stdio.h>	// printf()
 * #include <unistd.h>	// sleep()
 *
 * using namespace os;
 *
 * class MyThread : public Thread {
 *	public:
 *	MyThread() : Thread( "MyThread" ) {}
 *
 *	int32 Run() {
 *		for( int i = 0; i < 10000; i++ ) {
 *			Delay( 1000000 );
 *			printf( "In the loop: %d\n", i );
 *		}
 *		return 0;
 *	}
 * };
 *
 * int main(void)
 * {
 *	MyThread thread;
 *	thread.Start();
 *	printf( "The thread is running now!\n");
 *	sleep( 10 );	// Let it run for 10 secs
 *	thread.Terminate();
 *	return 0;
 * }
 * \endcode
 *
 * \sa Run(), Start(), Terminate()
 * \author	Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
class Thread
{
    private: // Unimplemented (can't copy a thread)
    Thread( const Thread& );
    Thread& operator=( const Thread& );

    public:
    Thread( const char *pzName, int nPriority = NORMAL_PRIORITY, int nStackSize = 0 );
    virtual ~Thread();

    void		Start();
    void		Stop();

    void		WaitFor();

    void		Terminate();
    void		Initialize( const char *pzName, int nPriority = NORMAL_PRIORITY, int nStackSize = 0 ) ;

    void		SetPriority( int nPriority = IDLE_PRIORITY );
    int			GetPriority();
    
    thread_id		GetThreadId();
    proc_id		GetProcessId();

    /** Thread code
     * \par	Description:
     *		The code that is executed by the thread. This method must be
     *		overridden and implemented in a subclass.
     * \author Henrik Isaksson (henrik@boing.nu)
     *****************************************************************************/
    virtual int32	Run( void ) = 0;

    protected:
    void		Delay( uint32 nMicros );

    private:
    static int32	_Entry( void* pvoid );
    thread_id		m_iThread;
};

typedef errno_exception ThreadException;

#define EINVALIDTHREAD	0

} // end of namespace

#endif // __F_UTIL_THREAD_H__


