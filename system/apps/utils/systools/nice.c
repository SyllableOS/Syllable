#include <string.h>
#include <stdio.h>

#include <atheos/types.h>
#include <atheos/threads.h>

int main( int argc, char** argv )
{
  if ( 3 == argc )
  {
    thread_id		hThread = -1; /* atoi( argv[1] ); */
    int					nPri =	atoi( argv[2] );

    sscanf( argv[1], "%d", &hThread );

    if ( get_thread_proc( hThread ) != -1 ) {
      set_thread_priority( hThread, nPri );
    } else {
      printf( "Thread %ld does not exist" );
    }
  }
  else
  {
    printf( "Usage : %s ThreadID Priority\n", argv[0] );
  }
}
