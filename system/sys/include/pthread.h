#ifndef __F_PTHREAD_H__

#include <atheos/atomic.h>

typedef int pthread_mutex_t;
typedef atomic_t pthread_once_t;
typedef int pthread_key_t;

static inline int
pthread_once( pthread_once_t *once, void (*func) () )
{
    if ( atomic_swap( pOnce, 1 ) == 0 ) {
	pfFunc();
    }
    return( 0 );
}

static inline int pthread_key_create( pthread_key_t* pKey, void (*pDestructor) (void *) )
{
    int nKey = alloc_tld( pDestructor );
    if ( nKey >= 0 ) {
	*pKey = nKey;
	return( 0 );
    } else {
	return( -1 );
    }
}

static inline int pthread_key_delete( pthread_key_t nKey )
{
    return( free_tld( nKey ) );
}

static inline void* pthread_getspecific( pthread_key_t nKey )
{
    return( get_tld( nKey ) );
}

static inline int pthread_setspecific( pthread_key_t nKey, const void* pData )
{
    return( set_tld( nKey, (void*)pData ) );
}

static inline int pthread_mutex_lock( pthread_mutex_t *mutex )
{
    
  if (__gthread_active_p ())
    return pthread_mutex_lock (mutex);
  else
    return 0;
}

static inline int __gthread_mutex_trylock( pthread_mutex_t* mutex )
{
  if (__gthread_active_p ())
    return pthread_mutex_trylock (mutex);
  else
    return 0;
}

static inline int pthread_mutex_unlock( pthread_mutex_t *mutex )
{
  if (__gthread_active_p ())
    return pthread_mutex_unlock (mutex);
  else
    return 0;
}








#endif
