#include <stdio.h>
#include <errno.h>

#include <atheos/kernel.h>

void usage( const char* pzStr )
{
  printf( "%s devicepath fstype volumename [fsargs]\n", pzStr );
}

int main( int argc, char** argv )
{
  if ( argc < 4 ) {
    usage( argv[0] );
    return( 1 );
  }
  if ( initialize_fs( argv[1], argv[2], argv[3], NULL, 0 ) < 0 ) {
    printf( "Error: Failed to format %s: %s\n", argv[1], strerror( errno ) );
  }
  return( 0 );
}
