#include <stdio.h>
#include <unistd.h>




int main( int argc, const char** argv )
{
  const char* pzPassWd;
  
  if ( argc != 2 ) {
      printf( "Usage: crypt passwd\n" );
  }
  pzPassWd = crypt(  argv[1], "$1$" );
  if ( pzPassWd == NULL ) {
    perror( "crypt()" );
    return( 1 );
  }
  printf( "Password: '%s'\n", pzPassWd  );
  return( 0 );
}

