/*  libatheos.so - the highlevel API library for AtheOS
 *  Copyright (C) 1999 - 2001 Kurt Skauen
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

#ifndef __F_ATHEOS_LOCKER_H__
#define __F_ATHEOS_LOCKER_H__

#include <stdio.h>
#include <errno.h>
#include <sys/debug.h>
#include <atheos/types.h>
#include <atheos/semaphore.h>
#include <atheos/threads.h>
#include <atheos/atomic.h>

namespace os
{
#if 0 // Fool Emacs auto-indent
}
#endif

/** 
 * \ingroup util
 * \par Description:
 *
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class Locker
{
public:
    Locker( const char* pzName, bool bRecursive = true, bool bInterruptible = false )
    {
	if ( !bRecursive ) bRecursive = false;
	m_hSema = create_semaphore( pzName, 0, /*(bRecursive) ? SEM_RECURSIVE :*/ 0 );
	atomic_set( &m_nLock, 0 );
	m_nNestCount = 0;
	  // FIXME: Check for errors.
	m_bInterruptible = bInterruptible;
    }
    ~Locker() { delete_semaphore( m_hSema ); }
    inline int    Lock() const;
    inline int    Lock( bigtime_t nTimeout ) const;
    inline int    Unlock() const;
    int		GetLockCount() const { return( m_nNestCount ); }
    thread_id	GetOwner() const { return( m_nOwner ); }
    inline bool 	IsLocked() const;
private:
    Locker( const Locker& );
    Locker& operator=(const Locker&);
    bool      m_bInterruptible;
    sem_id    m_hSema;
    mutable thread_id m_nOwner;
    mutable atomic_t  m_nLock;
    mutable int	      m_nNestCount;
};

/** 
 * \ingroup util
 * \par Description:
 *
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class AutoLocker
{
public:
    AutoLocker( Locker* pcMutex ) { m_pcMutex = pcMutex; pcMutex->Lock(); }
    ~AutoLocker() { m_pcMutex->Unlock(); }
private:
    Locker* m_pcMutex;
};

int Locker::Lock() const
{
    thread_id nMyThread = get_thread_id( NULL );
    int nError;

    if ( m_nOwner == nMyThread ) {
	m_nNestCount++;
	nError = 0;
    } else {
	if ( atomic_inc_and_read( &m_nLock ) > 0 ) {
	    for (;;) {
		nError = lock_semaphore( m_hSema );
		if ( nError >= 0 || errno != EINTR ) {
		    break;
		}
	    }
	    if ( nError < 0 ) {
		atomic_dec( &m_nLock );
	    } else {
		m_nOwner = nMyThread;
		m_nNestCount = 1;
	    }
	} else {
	    nError = 0;
	    m_nOwner = nMyThread;
	    m_nNestCount = 1;
	}
    }
    return( nError );
}

int Locker::Lock( bigtime_t nTimeout ) const
{
    thread_id nMyThread = get_thread_id( NULL );
    int nError;

    if ( m_nOwner == nMyThread ) {
	m_nNestCount++;
	nError = 0;
    } else {
	if ( atomic_inc_and_read( &m_nLock ) > 0 ) {
	    for (;;) {
		nError = lock_semaphore_x( m_hSema, 1, 0, nTimeout );
		if ( nError >= 0 || errno != EINTR ) {
		    break;
		}
	    }
	    if ( nError < 0 ) {
		atomic_dec( &m_nLock );
	    } else {
		m_nOwner = nMyThread;
		m_nNestCount = 1;
	    }
	} else {
	    nError = 0;
	    m_nOwner = nMyThread;
	    m_nNestCount = 1;
	}
    }
    return( nError );
}

int Locker::Unlock() const
{
#if 0
    return( unlock_semaphore( m_hSema ) );
#else
    int nError;

    if ( m_nNestCount > 1 ) {
	m_nNestCount--;
	return( 0 );
    } else {
	m_nOwner = -1;
	m_nNestCount = 0;
    
	if ( !atomic_dec_and_test( &m_nLock ) ) {
	    nError = unlock_semaphore( m_hSema );
	} else {
	    nError = 0;
	}
	return( nError );
    }
#endif
}

inline bool Locker::IsLocked() const
{
    return( m_nNestCount > 0 && m_nOwner == get_thread_id(NULL) );
}
/*
class gate
{
public:

  int Lock()       { return( lock_semaphore( m_hSema ) ); }
  int Unlock()     { return( unlock_semaphore( m_hSema ) ); }

  int Close()      { return( lock_semaphore_x( m_hSema, 100000000, 0, INFINITE_TIMEOUT ) ); }
  int Open()       { return( unlock_semaphore_x( m_hSema, 100000000, 0 ) ); }
  
private:
  Locker  m_cGateMutex;
  Locker  m_cMutex;
};

int gate::Lock()
{
    m_cMutex.Lock();

    while ( m_nRWLockCnt > 0 ) {
    
    }
  
    if ( m_nRLockCnt > 0 
  
	 m_cMutex.Unlock();
	 }
    */


/** 
 * \ingroup util
 * \par Description:
 *
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/
    
    class Gate
    {
    public:
	Gate( const char* pzName, bool bRecursive = true, bool bInterruptible = false )
	{
	    m_hSema = create_semaphore( pzName, 100000000, (bRecursive) ? SEM_RECURSIVE : 0 );
	      // FIXME: Check for errors.
	    m_bInterruptible = bInterruptible;
	}
	~Gate() { delete_semaphore( m_hSema ); }
	int Lock() const       { return( lock_semaphore( m_hSema ) ); }
	int Unlock() const     { return( unlock_semaphore( m_hSema ) ); }

	int Close() const     { return( lock_semaphore_x( m_hSema, 100000000, 0, INFINITE_TIMEOUT ) ); }
	int Open() const      { return( unlock_semaphore_x( m_hSema, 100000000, 0 ) ); }
  
    private:
	bool m_bInterruptible;
	sem_id m_hSema;
    };

}

#endif // __F_ATHEOS_LOCKER_H__


