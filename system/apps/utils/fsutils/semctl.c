#include <stdio.h>
#include <errno.h>

#include <exec/semaphor.h>

int main( int argc, char** argv )
{
	if ( 2 == argc )
	{
		int	nSema = atoi( argv[1] );
		int	nError = unlock_semaphore( nSema );
		if ( 0 != nError ) {
			printf( "ReleaseSemaphore failed : %s\n", strerror( errno ) );
		}
	}
	else
	{
		printf( "Invalid argument count\n" );
	}
}
