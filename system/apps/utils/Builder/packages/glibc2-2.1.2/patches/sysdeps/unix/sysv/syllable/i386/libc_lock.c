#include <bits/libc-lock.h>
#include <atheos/time.h>
#include <atheos/smp.h>
#include <atheos/atomic.h>

static void __create_sem( __libc_lock_t* lock, const char* pzName, int nCount, int nFlags )
{
    if ( atomic_swap( &lock->nIsInited, 1 ) == 0 ) {
	lock->hMutex = create_semaphore( pzName, nCount, 0 );
	if ( lock->hMutex < 0 ) {
	    atomic_swap( &lock->nIsInited, 0 );
	    return;
	}
    }
    while( lock->hMutex < 0 ) {
	snooze( 1000LL );
    }
}

void __lock_fini( __libc_lock_t* lock )
{
    if ( lock->hMutex != -1 ) {
	delete_semaphore( lock->hMutex );
	lock->hMutex = -1;
    }
}

int __lock_lock( __libc_lock_t* lock )
{
    int error;

    if ( lock->hMutex == -1 ) {
	__create_sem( lock, "libc_lock", 0, 0 );
    }
  
    if (atomic_add( &lock->nLock, 1 ) > 0) {
	for (;;) {
	    error = lock_semaphore( lock->hMutex );
	    if ( error >= 0 || errno != EINTR ) {
		break;
	    }
	}
    } else {
	error = 0;
    }
    return( error );
}


int __lock_lock_recursive( __libc_lock_t* lock )
{
    if ( lock->hMutex == -1 ) {
	__create_sem( lock, "libc_rlock", 1, SEM_REQURSIVE );
    }
    while( lock_semaphore( lock->hMutex ) < 0 && errno == EINTR );
    return( 0 );
}

int __lock_trylock( __libc_lock_t* lock )
{
    if ( lock->hMutex == -1 ) {
	__create_sem( lock, "libc_lock", 0, 0 );
    }
    if ( atomic_add( &lock->nLock, 1 ) > 0 ) {
	atomic_add( &lock->nLock, -1 );
	unlock_semaphore_x( lock->hMutex, 0, 0 );
	errno = EWOULDBLOCK;
	return( -1 );
    } else {
	return( 0 );
    }
}


void __lock_unlock( __libc_lock_t* lock )
{
    if ( atomic_add( &lock->nLock, -1 ) > 1) {
	unlock_semaphore( lock->hMutex );
    }
}

int __libc_key_create( __libc_key_t* key, void* destr )
{
    *key = alloc_tld( destr );
    return( (*key < 0) ? -1 : 0 );
}

void* __libc_getspecific( __libc_key_t key )
{
    return( get_tld( key ) );
}

int __libc_setspecific( __libc_key_t key, void* data )
{
    return( set_tld( key, data ) );
}
