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
	int nFile, nDataFile;
	int i, nPos = 0;
	char zBuf[1024];
  
	if ( ( argc < 4 ) || ( argc == 5 && strcasecmp( argv[3], "-f" ) ) || (argc > 5) ) {
		print_usage( argv[0] );
		return( 1 );
	}

	nFile = open( argv[1], O_RDWR | O_NOTRAVERSE );

	if ( nFile < 0 ) {
		printf( "Failed to open %s: %s\n", argv[1], strerror( errno ) );
		return( 1 );
	}
	
	if( argc == 5 ) {
		/* Passed -f parameter; write data from file to attribute */
		nDataFile = open( argv[4], O_RDONLY );
		if( nDataFile < 0 ) {
			printf( "Failed to open data file %s: %s\n", argv[4], strerror( errno ) );
			close( nFile );
			return( 1 );
		}
		do {
			i = read( nDataFile, zBuf, sizeof( zBuf ) );
			write_attr( nFile, argv[2], 0, ATTR_TYPE_NONE, zBuf, nPos, i );
			nPos += i;
		} while( i == sizeof( zBuf ) );
	}
	else
	{
		/* Write a string to the attr */
		write_attr( nFile, argv[2], O_TRUNC, ATTR_TYPE_STRING, argv[3], 0, strlen( argv[3] ) );
	}
  
	close( nFile );

	return( 0 );
}




