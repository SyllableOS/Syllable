#include <string.h>
#include <stdio.h>
#include <signal.h>

#include <atheos/types.h>
#include <atheos/threads.h>

int main( int argc, char** argv )
{
  if (  argc > 1 )
  {
    thread_id		hThread = -1; /* atoi( argv[1] ); */
    int					nLevel;

    if ( argc == 3 ) {
      nLevel =	atoi( argv[2] );
    } else {
      nLevel = 1;
    }
		
    sscanf( argv[1], "%d", &hThread );

    if ( get_thread_proc( hThread ) != -1 ) {
      set_strace_level( hThread, nLevel );
    } else {
      printf( "Thread %d does not exist" );
    }
  }
  else
  {
    printf( "Usage : %s thread_id [level]\n", argv[0] );
  }
}
