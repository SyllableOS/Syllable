#include <string.h>
#include <ctype.h>

int __strnicmp(const char * cs,const char * ct,size_t count)
{
  register signed char __res = 0;

  while (count) {
    if ((__res = toupper( *cs ) - toupper( *ct++ ) ) != 0 || !*cs++)
      break;
    count--;
  }

  return __res;
}

weak_alias( __strnicmp, strnicmp )
