#include <stdio.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>

#include <atheos/fs_attribs.h>



void print_usage( const char* pzName )
{
  printf( "usage: %d path", pzName );
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

  nFile = open( argv[1], O_RDWR );

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
      if ( read_attr( nFile, psEntry->d_name, 0, zBuffer, 0, sInfo.ai_size ) == sInfo.ai_size )
      {
	zBuffer[ sInfo.ai_size ] = '\0';
//	printf( "Value : '%s'\n", zBuffer );
      } else {
	strcpy( zBuffer, "**error**" );
      }
      printf( "%s - size=%d type=%d val=%s\n", psEntry->d_name, (int)sInfo.ai_size, sInfo.ai_type, zBuffer );
    } else {
      printf( "Failed to stat attrib %s::%s\n", argv[1], psEntry->d_name );
    }
  }
  close_attrdir( pDir );
  return( 0 );
}
