#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <atheos/fs_attribs.h>

int main( int argc, char** argv )
{
	DIR* pDir;
	struct dirent* psEntry;
	int nFile;
  
	if ( argc < 2 ) {
		printf( "usage: %s path\n", argv[0] );
		return( 1 );
	}

	nFile = open( argv[1], O_RDONLY | O_NOTRAVERSE );

	if ( nFile < 0 ) {
		printf( "Failed to open %s: %s\n", argv[1], strerror( errno ) );
		return( 1 );
	}

	pDir = open_attrdir( nFile );

	if ( NULL == pDir ) {
		printf( "Failed to open attrib dir\n" );
		close ( nFile );
		return( 1 );
	}
  
	while( ( psEntry = read_attrdir( pDir ) ) )
	{
		char zBuffer[1024];
		attr_info_s sInfo;

		union {
			int32 m_nInt32;
			int64 m_nInt64;
			float m_vFloat;
			double m_vDouble;
		} sTypes;

		register ssize_t nLen;

		if ( stat_attr( nFile, psEntry->d_name, &sInfo ) == 0 )
		{
			printf( "%s - size = %d, type = %d ", psEntry->d_name, (int)sInfo.ai_size, sInfo.ai_type );

			switch( sInfo.ai_type )
			{
				case ATTR_TYPE_NONE:
	    			printf( "[ATTR_TYPE_NONE (Raw binary data)]" );
	    			break;

	    		case ATTR_TYPE_INT32:
				{
					if ( ( sInfo.ai_size == sizeof( int32 ) ) && ( ( nLen = read_attr( nFile, psEntry->d_name, ATTR_TYPE_INT32, &(sTypes.m_nInt32), 0, sInfo.ai_size ) ) == sInfo.ai_size ) )
						sprintf( zBuffer, "%li", sTypes.m_nInt32 );
					else
						strcpy(zBuffer, "**error**");
	    			printf( "[ATTR_TYPE_INT32] - \"%s\"", zBuffer );
	    			break;
				}

	    		case ATTR_TYPE_INT64:
				{
					if ( ( sInfo.ai_size == sizeof( int64 ) ) && ( ( nLen = read_attr( nFile, psEntry->d_name, ATTR_TYPE_INT64, &(sTypes.m_nInt64), 0, sInfo.ai_size ) ) == sInfo.ai_size ) )
						sprintf( zBuffer, "%lli", sTypes.m_nInt64 );
					else
						strcpy(zBuffer, "**error**");
	    			printf( "[ATTR_TYPE_INT64] - \"%s\"", zBuffer );
	    			break;
				}

	    		case ATTR_TYPE_FLOAT:
				{
					if ( ( sInfo.ai_size == sizeof( float ) ) && ( ( nLen = read_attr( nFile, psEntry->d_name, ATTR_TYPE_FLOAT, &(sTypes.m_vFloat), 0, sInfo.ai_size ) ) == sInfo.ai_size ) )
						sprintf( zBuffer, "%f", sTypes.m_vFloat );
					else
						strcpy(zBuffer, "**error**");
	    			printf( "[ATTR_TYPE_FLOAT] - \"%s\"", zBuffer );
	    			break;
	    		}

	    		case ATTR_TYPE_DOUBLE:
				{
					if ( ( sInfo.ai_size == sizeof( double ) ) && ( ( nLen = read_attr( nFile, psEntry->d_name, ATTR_TYPE_DOUBLE, &(sTypes.m_vDouble), 0, sInfo.ai_size ) ) == sInfo.ai_size ) )
						sprintf( zBuffer, "%lf", sTypes.m_vDouble );
					else
						strcpy(zBuffer, "**error**");
	    			printf( "[ATTR_TYPE_DOUBLE] - \"%s\"", zBuffer );
	    			break;
				}

				case ATTR_TYPE_STRING:
				{
					if ( ( nLen = read_attr( nFile, psEntry->d_name, ATTR_TYPE_STRING, zBuffer, 0, sizeof( zBuffer ) - 1 ) ) == sInfo.ai_size )
						zBuffer[nLen] = '\0';
					else
						strcpy(zBuffer, "**error**");
					printf( "[ATTR_TYPE_STRING] - \"%s\"", zBuffer );
					break;
				}

	    		default:
	    			printf( "[UNKNOWN]" );
	    			break;
			}

			printf( "\n" );

		} else
			printf( "Failed to stat attrib %s::%s\n", argv[1], psEntry->d_name );
	}

	close_attrdir( pDir );
	close( nFile );
	return( 0 );  
}
