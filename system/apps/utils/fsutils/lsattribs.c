#include <stdio.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>

#include <atheos/fs_attribs.h>

void print_usage( const char* pzName )
{
    printf( "usage: %s path\n", pzName );
}

void read_string_attr( int nFile, const char *pzName, char *pzBuffer, int nSize )
{
    int nLen;

    if ( ( nLen = read_attr( nFile, pzName, ATTR_TYPE_STRING, pzBuffer, 0, nSize - 1 ) ) > 0 )
    {
	pzBuffer[ nLen + 1 ] = '\0';
    } else {
	strcpy( pzBuffer, "**error**" );
    }
}

int main( int argc, char** argv )
{
    DIR* pDir;
    struct dirent* psEntry;
    int    nFile;
  
    if ( argc < 2 ) {
	print_usage( argv[0] );
	return( 1 );
    }

    nFile = open( argv[1], O_RDWR | O_NOTRAVERSE );

    if ( nFile < 0 ) {
	printf( "Failed to open %s: %s\n", argv[1], strerror( errno ) );
	return( 1 );
    }
  
    pDir = open_attrdir( nFile );

    if ( NULL == pDir ) {
	printf( "Failed to open attrib dir\n" );
	return( 1 );
    }
  
    while( (psEntry = read_attrdir( pDir )) )
    {
	char zBuffer[1024];
    
	attr_info_s sInfo;
    
	if ( stat_attr( nFile, psEntry->d_name, &sInfo ) == 0 )
	{
	    printf( "%s - size = %d, type = %d ", psEntry->d_name, (int)sInfo.ai_size, sInfo.ai_type );

	    switch( sInfo.ai_type )
	    {
	    	case ATTR_TYPE_STRING:
	    		read_string_attr( nFile, psEntry->d_name, zBuffer, sizeof( zBuffer ) );
	    		printf( "[ATTR_TYPE_STRING] - \"%s\"", zBuffer );
	    		break;
	    	case ATTR_TYPE_NONE:
	    		printf( "[ATTR_TYPE_NONE (Raw binary data)]" );
	    		break;
	    	case ATTR_TYPE_INT32:
	    		printf( "[ATTR_TYPE_INT32]" );
	    		break;
	    	case ATTR_TYPE_INT64:
	    		printf( "[ATTR_TYPE_INT64]" );
	    		break;
	    	case ATTR_TYPE_FLOAT:
	    		printf( "[ATTR_TYPE_FLOAT]" );
	    		break;
	    	case ATTR_TYPE_DOUBLE:
	    		printf( "[ATTR_TYPE_DOUBLE]" );
	    		break;
	    	default:
	    		printf( "[Unknown]" );
	    		break;
	    }
	    printf( "\n" );
	} else {
	    printf( "Failed to stat attrib %s::%s\n", argv[1], psEntry->d_name );
	}
    }

    close_attrdir( pDir );
    return( 0 );
}







