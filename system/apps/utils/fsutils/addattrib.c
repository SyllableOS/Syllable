#include <stdio.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>
#include <dirent.h>

#include <atheos/fs_attribs.h>

void print_usage( const char* pzName )
{
	printf( "usage: %s path attr_name [-f] value\n", pzName );
}

int main( int argc, char** argv )
{
	int nFile;
	int i;
	DIR* pDir;
	struct dirent* psEntry;
  
	if ( argc < 4 ) {
		print_usage( argv[0] );
		return( 1 );
	}

	nFile = open( argv[1], O_RDWR );

	if ( nFile < 0 ) {
		printf( "Failed to open %s: %s\n", argv[1], strerror( errno ) );
		return( 1 );
	}
  
	write_attr( nFile, argv[2], O_TRUNC, ATTR_TYPE_STRING, argv[3], 0, strlen( argv[3] ) );
  
	close( nFile );

	return( 0 );
}



