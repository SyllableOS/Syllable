#ifndef __STDIO_LOCK_H__
#define __STDIO_LOCK_H__

#include <bits/libc-lock.h>


#define _IO_lock_initializer MUTEX_INITIALIZER

#define _IO_lock_init( lock )	__libc_lock_init( lock )
#define _IO_lock_fini( lock )	__libc_lock_fini( lock )
#define _IO_lock_lock( lock )	__libc_lock_lock( lock )
#define _IO_lock_unlock( lock )	__libc_lock_unlock( lock )

#define _IO_cleanup_region_start( f, fp )
#define _IO_cleanup_region_end( a )

#define _IO_lock_t __libc_lock_t

#if 0
typedef struct {
  sem_id mutex;
} _IO_lock_t;

#define _IO_lock_initializer {-1}

#define _IO_lock_init( lock ) do { (lock).mutex = -1; } while(0)
#define _IO_lock_fini( lock )		\
 do {					\
  if ((lock).mutex != -1 ) {		\
    delete_semaphore( (lock).mutex );	\
    (lock).mutex = -1;			\
  }					\
} while(0)

#define _IO_lock_lock( lock )					\
do {								\
  if ( (lock).mutex == -1 ) {					\
    (lock).mutex = create_semaphore( "libc_io_lock", 1, 0 );	\
  }								\
  if ( (lock).mutex < 0 ) {					\
    abort();							\
  }								\
  lock_semaphore( (lock).mutex );				\
} while(0)

#define _IO_lock_unlock( lock ) do{ unlock_semaphore( (lock).mutex ); } while(0)

#define _IO_cleanup_region_start( f, fp )
#define _IO_cleanup_region_end( a )



#endif

/*
static inline void _IO_lock_init( _IO_lock_t* pSem )
{
  *pSem = create_semaphore( "libc_io_lock", 1, 0 );
}

static inline void _IO_lock_fini( _IO_lock_t* pSem )
{
  delete_semaphore( *pSem );
}

create_semaphore( "libc_stdf_lock", 1, 0 )
*/
#endif /*__STDIO_LOCK_H__*/
