#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <atheos/time.h>

time_t time(time_t *t)
{
  bigtime_t nTime;

  nTime = get_real_time();
  if ( t != NULL ) {
    *t = nTime / 1000000;
  }
  return( nTime / 1000000 );
}
