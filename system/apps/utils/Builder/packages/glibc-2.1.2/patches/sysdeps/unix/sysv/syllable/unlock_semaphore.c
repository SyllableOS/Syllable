#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <atheos/semaphore.h>




status_t __unlock_semaphore( sem_id hSema )
{
  return( __unlock_semaphore_x( hSema, 1, 0 ) );
}

weak_alias (__unlock_semaphore, unlock_semaphore)

