#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <limits.h>

#include <atheos/types.h>
#include <macros.h>

void write_int8( FILE* hFile, uint8 nValue )
{
    fwrite( &nValue, 1, sizeof(nValue), hFile );
}
void write_int32( FILE* hFile, uint32 nValue )
{
    fwrite( &nValue, 1, sizeof(nValue), hFile );
}

bool build_archive( FILE* hFile, const char* pzPath )
{
    DIR* hDir = opendir( pzPath );
    struct dirent* psEntry;
    
    if ( hDir == NULL ) {
	fprintf( stderr, "Error: failed to open directory '%s' : %s\n", pzPath, strerror(errno) );
	return( false );
    }
    while( (psEntry = readdir( hDir )) != NULL ) {
	char zFullPath[PATH_MAX];
	int  nPathLen = strlen( pzPath );
	int  nNameLen = strlen( psEntry->d_name );
	struct stat sStat;
	if ( strcmp( psEntry->d_name, "." ) == 0 || strcmp( psEntry->d_name, ".." ) == 0  ) {
	    continue;
	}
	memcpy( zFullPath, pzPath, nPathLen + 1 );
	if ( zFullPath[nPathLen-1] != '/' ) {
	    zFullPath[nPathLen++] = '/';
	    zFullPath[nPathLen] = '\0';
	}
	strcat( zFullPath, psEntry->d_name );
	if ( lstat( zFullPath, &sStat ) < 0 ) {
	    fprintf( stderr, "Error: failed to stat '%s' : %s\n", zFullPath, strerror(errno) );
	    closedir( hDir );
	    return( false );
	}
	write_int32( hFile, sStat.st_mode );
	write_int8( hFile, nNameLen );
	fwrite( psEntry->d_name, 1, nNameLen, hFile );
	
	if ( S_ISDIR( sStat.st_mode ) ) {
	    if ( build_archive( hFile, zFullPath ) == false ) {
		closedir( hDir );
		return( false );
	    }
	} else if ( S_ISLNK( sStat.st_mode ) ) {
	    char zBuffer[PATH_MAX];
	    int  nSize;
	    printf( "Add link %s\n", zFullPath );
	    nSize = readlink( zFullPath, zBuffer, PATH_MAX );
	    if ( nSize < 0 ) {
		fprintf( stderr, "Error: failed to read link '%s' : %s\n", zFullPath, strerror(errno) );
		closedir( hDir );
		return( false );
	    }
	    write_int32( hFile, nSize );
	    fwrite( zBuffer, 1, nSize, hFile );
	} else if ( S_ISREG( sStat.st_mode ) ) {
	    FILE* hSrcFile;
	    int   nSize = sStat.st_size;
	    
	    printf( "Add file %s\n", zFullPath );
	    write_int32( hFile, nSize );
	    hSrcFile = fopen( zFullPath, "r" );
	    if ( hSrcFile == NULL ) {
		fprintf( stderr, "Error: failed to open '%s' : %s\n", zFullPath, strerror(errno) );
		closedir( hDir );
		return( false );
	    }
	    while( nSize > 0 ) {
		int nCurSize = min( 8192, nSize );
		uint8 anBuffer[8192];
		fread( anBuffer, 1, nCurSize, hSrcFile );
		fwrite( anBuffer, 1, nCurSize, hFile );
		nSize -= nCurSize;
	    }
	    fclose( hSrcFile );
	} else {
	    printf( "Warning: Unknown node %s\n", zFullPath );
	}
    }
    closedir( hDir );
    write_int32( hFile, ~0 );
    return( true );
}

int test_image( char* pzPath, uint8* pBuffer, int nSize )
{
    int nBytesRead = 0;
    
    while( nSize > 3 ) {
	char zFullPath[PATH_MAX];
	char zName[256];
	uint32 nMode  = *((uint32*)pBuffer);
	uint32 nNameLen;
	pBuffer += 4;
	nSize   -= 4;
	nBytesRead += 4;

	if ( nMode == ~0 ) {
	    break;
	}
	
	nNameLen = *pBuffer;
	pBuffer += 1;
	nSize   -= 1;
	nBytesRead += 1;
	
	if ( nNameLen > nSize ) {
	    printf( "Warning: RAM disk image is corrupted\n" );
	    return( -EINVAL );
	}
	memcpy( zName, pBuffer, nNameLen );
	zName[nNameLen] = '\0';
	pBuffer += nNameLen;
	nSize   -= nNameLen;
	nBytesRead += nNameLen;

	strcpy( zFullPath, pzPath );
	strcat( zFullPath, "/" );
	strcat( zFullPath, zName );
	
	if ( S_ISDIR( nMode ) ) {
	    int nLen = test_image( zFullPath, pBuffer, nSize );
	    if ( nLen < 0 ) {
		return( nLen );
	    }
	    pBuffer    += nLen;
	    nSize      -= nLen;
	    nBytesRead += nLen;
	} else if ( S_ISREG( nMode ) || S_ISLNK(nMode) ) {
	    uint32 nFileSize;
	    if ( nSize < 4 ) {
		printf( "Warning: RAM disk image is corrupted\n" );
		return( -EINVAL );
	    }
	    nFileSize = *((uint32*)pBuffer);
	    pBuffer += 4; nSize -= 4; nBytesRead += 4;
	    if ( nSize < nFileSize ) {
		printf( "Warning: RAM disk image is corrupted\n" );
		return( -EINVAL );
	    }
	    pBuffer += nFileSize; nSize -= nFileSize; ; nBytesRead += nFileSize;
	    printf( "%s (%ld)\n", zFullPath, nFileSize );
	}
    }
    return( nBytesRead );
}

int main( int argc, char** argv )
{
    FILE*  hArchive;

    if ( argc == 2 ) {
	struct stat sStat;
	void* pBuffer;
	
	if ( stat( argv[1], &sStat ) < 0 ) {
	    fprintf( stderr, "Error: failed to stat archive '%s' : %s\n", argv[1], strerror(errno) );
	    return( 1 );
	}
	hArchive = fopen( argv[1], "r" );
	if ( hArchive == NULL ) {
	    fprintf( stderr, "Error: failed to open archive '%s' : %s\n", argv[1], strerror(errno) );
	    return( 1 );
	}
	pBuffer = malloc( sStat.st_size );
	if ( pBuffer == NULL ) {
	    fprintf( stderr, "No memory for disk image\n" );
	    return( 1 );
	}
	fread( pBuffer, 1, sStat.st_size, hArchive );
	fclose( hArchive );
	test_image( "", pBuffer, sStat.st_size );
	return( 0 );
    }
    if ( argc < 3 ) {
	fprintf( stderr, "Usage: %s rootdir archive\n", argv[0] );
	return( 1 );
    }
    hArchive = fopen( argv[2], "w" );
    if ( hArchive == NULL ) {
	fprintf( stderr, "Error: failed to open archive '%s' : %s\n", argv[2], strerror(errno) );
	return( 1 );
    }
    build_archive( hArchive, argv[1] );
    fclose( hArchive );
    return( 0 );
}
