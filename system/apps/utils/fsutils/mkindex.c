#include <stdio.h>
#include <errno.h>
#include <atheos/filesystem.h>



int parse_type( const char* pzType )
{
      if ( strcasecmp( pzType, "INT" ) == 0 ) {
	  return( ATTR_TYPE_INT32 );
      } else if ( strcasecmp( pzType, "INT32" ) == 0 ) {
	  return( ATTR_TYPE_INT32 );
      } else if ( strcasecmp( pzType, "INT64" ) == 0 ) {
	  return( ATTR_TYPE_INT64 );
      } else if ( strcasecmp( pzType, "FLOAT" ) == 0 ) {
	  return( ATTR_TYPE_FLOAT );
      } else if ( strcasecmp( pzType, "DOUBLE" ) == 0 ) {
	  return( ATTR_TYPE_DOUBLE );
      } else if ( strcasecmp( pzType, "STRING" ) == 0 ) {
	  return( ATTR_TYPE_STRING );
      } else {
	  return( -1 );
      }
}



int main( int argc, char** argv )
{
    int nType;
    int nError;
    
    if ( argc != 3 ) {
	fprintf( stderr, "Usage: mkindex volume indexname {INT|INT32|INT64|FLOAT|DOUBLE|STRING}\n" );
	exit( 1 );
    }
    nType = parse_type( argv[3] );
    if ( nType < 0 ) {
	fprintf( stderr, "Usage: mkindex volume indexname {INT|INT32|INT64|FLOAT|DOUBLE|STRING}\n" );
	exit( 1 );
    }
    nError = create_index( argv[1], argv[2], nType, 0 );
    if ( nError < 0 ) {
	fprintf( stderr, "Error: failed to create index: %s\n", strerror( errno ) );
	exit( 2 );
    }
    return( 0 );
}
