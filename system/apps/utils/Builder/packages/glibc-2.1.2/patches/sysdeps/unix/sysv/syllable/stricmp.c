#include <string.h>
#include <ctype.h>


int __stricmp(const char * cs,const char * ct)
{
  register signed char __res;

  while (1) {
    if ((__res = toupper( *cs ) - toupper( *ct++ )) != 0 || !*cs++)
      break;
  }

  return __res;
}

weak_alias( __stricmp, stricmp )
