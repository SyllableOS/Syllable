#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <limits.h>
#include <stdlib.h>

#include <atheos/types.h>
#include <atheos/time.h>

#include <macros.h>

#define DISK_SIZE (80*18*2*512)
#define HEADER_MAGIC 0xa105f100



void write_int32( FILE* hFile, uint32 nValue )
{
    fwrite( &nValue, 1, sizeof(nValue), hFile );
}

int main( int argc, char** argv )
{
    FILE* hSrcFile;
    struct stat sStat;
    int nNumDisks;
    void* pBuffer;
    int   nSize;
    uint32 nImageID;
    int i;

    srand( get_real_time() );
    nImageID = random();
    
    if ( argc != 3 ) {
	fprintf( stderr, "Usage: %s image_file disk_prefix\n", argv[0] );
	exit( 1 );
    }

    if ( stat( argv[1], &sStat ) < 0 ) {
	fprintf( stderr, "Failed to stat '%s' : %s\n", argv[1], strerror(errno) );
	exit( 1 );
    }
    
    hSrcFile = fopen( argv[1], "r" );
    if ( hSrcFile == NULL ) {
	fprintf( stderr, "Failed to open '%s' : %s\n", argv[1], strerror(errno) );
	exit( 1 );
    }
    nSize = sStat.st_size;
    nNumDisks = (nSize + DISK_SIZE - 512 - 1) / (DISK_SIZE - 512);

    printf( "Write %d disk images\n", nNumDisks );

    pBuffer = malloc( DISK_SIZE );

    
    for ( i = 0 ; i < nNumDisks ; ++i ) {
	uint32 anHeader[128];
	char  zDstPath[PATH_MAX];
	FILE* hDstFile;
	int   nCurSize = min( DISK_SIZE - 512, nSize );
	
	sprintf( zDstPath, "%s.%02d", argv[2], i + 1 );

	printf( "Create image %s\n", zDstPath );

	hDstFile = fopen( zDstPath, "w" );
	if ( hDstFile == NULL ) {
	    fprintf( stderr, "Error: failed to create %s : %s\n", zDstPath, strerror(errno) );
	    exit( 1 );
	}
	
	memset( anHeader, 0, sizeof(anHeader) );
	anHeader[0] = HEADER_MAGIC;
	anHeader[1] = nNumDisks;
	anHeader[2] = i;
	anHeader[3] = sStat.st_size;
	anHeader[4] = nImageID;

	fwrite( anHeader, 1, 512, hDstFile );
	
	if ( fread( pBuffer, 1, nCurSize, hSrcFile ) != nCurSize ) {
	    fprintf( stderr, "Error: failed to read from %s : %s\n", argv[1], strerror(errno) );
	    exit( 1 );
	}
	
	if ( fwrite( pBuffer, 1, (nCurSize + 511) & ~511, hDstFile ) != ((nCurSize + 511) & ~511) ) {
	    fprintf( stderr, "Error: failed to read from %s : %s\n", zDstPath, strerror(errno) );
	    exit( 1 );
	}
	
	fclose( hDstFile );
	nSize -= nCurSize;
    }
    fclose( hSrcFile );
    return( 0 );
}
