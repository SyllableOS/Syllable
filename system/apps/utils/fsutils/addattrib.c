#include <stdio.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>
#include <dirent.h>

#include <atheos/fs_attribs.h>



void print_usage( const char* pzName )
{
  printf( "usage: %d path attr_name [-f] value", pzName );
}

int main( int argc, char** argv )
{
  int nFile;
  int i;
  DIR* pDir;
  struct dirent* psEntry;
  
  if ( argc < 2 ) {
    print_usage( argv[0] );
    return( 1 );
  }

  nFile = open( argv[1], O_RDWR );

  if ( nFile < 0 ) {
    printf( "Failed to open %s: %s\n", argv[1], strerror( errno ) );
    return( 1 );
  }

  for ( i = 0 ; i < 10000 ; ++i )
  {
    char zName[256];
    char zValue[256];

    sprintf( zName,  "n%08d", i );
//    sprintf( zValue, "*%012d*", i );
    sprintf( zValue, "v%010d", i );

    printf( "Create %s\r", zName );
    write_attr( nFile, zName, O_TRUNC, 0x00ff00, zValue, 0, strlen( zValue ) );
//    fs_write_attr( nFile, zName, 0, 0, zValue, 4, strlen( zValue ) );
  }
/*    
  pDir = fs_open_attr_dir( argv[1] );

  if ( NULL == pDir ) {
    printf( "Failed to open attrib dir\n" );
    return( 1 );
  }
  while( (psEntry = fs_read_attr_dir( pDir )) )
  {
    char zBuffer[1024];
    
    attr_info_s sInfo;
    
    if ( fs_stat_attr( nFile, psEntry->d_name, &sInfo ) == 0 )
    {
      if ( fs_read_attr( nFile, psEntry->d_name, 0, zBuffer, 0, sInfo.ai_size ) == sInfo.ai_size )
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
  fs_close_attr_dir( pDir );
  */  
  return( 0 );
}
