#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <atheos/filesystem.h>

void human( char* pzBuffer, off_t nValue )
{
    if ( nValue > (1024*1024*1024) ) {
	sprintf( pzBuffer, "%.1fG", ((double)nValue) / (1024.0*1024.0*1024.0) );
    } else if ( nValue > (1024*1024) ) {
	sprintf( pzBuffer, "%.1fM", ((double)nValue) / (1024.0*1024.0) );
    } else if ( nValue > 1024 ) {
	sprintf( pzBuffer, "%.1fk", ((double)nValue) / 1024.0 );
    } else {
	sprintf( pzBuffer, "%db", nValue );
    }
}

int main( int argc, char** argv )
{
    fs_info sInfo;
    char    zSize[64];
    char    zUsed[64];
    char    zAvail[64];
    
    if ( argc != 2 ) {
	fprintf( stderr, "USAGE: %s dev_path\n", argv[0] );
	return( 1 );
    }
    if ( probe_fs( argv[1], &sInfo ) < 0 ) {
	fprintf( stderr, "Error: failed to probe device: %s\n", strerror(errno) );
	return( 1 );
    }
    human( zSize, sInfo.fi_total_blocks * sInfo.fi_block_size );
    human( zUsed, (sInfo.fi_total_blocks - sInfo.fi_free_blocks) * sInfo.fi_block_size );
    human( zAvail, sInfo.fi_free_blocks * sInfo.fi_block_size );
    
    printf( "Type        Size  Used  Avail  Use%\n" );
    printf( "%-10s  %-5s %-5s %-6s %.1f%%\n", sInfo.fi_driver_name, zSize, zUsed, zAvail,
	    (1.0 - ((double)sInfo.fi_free_blocks) / ((double)sInfo.fi_total_blocks)) * 100.0 );
}
