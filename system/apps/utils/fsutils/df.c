#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <atheos/filesystem.h>
#include <atheos/types.h>

/* Display the banner at the top of the listing */
void print_banner( void )
{
	printf( "Filesystem\t\t1K-blocks\tUsed\t\tAvailable\tUse%%\tMounted on\n" );
}

/* Pretty-print the volume information */
status_t display_volume( fs_info sInfo, char *zVolume )
{
	status_t nError = EOK;

	printf( "%s\t", sInfo.fi_device_path );

	/* Calculate the block values. Take into account non-1k block volumes */
	uint64 nSizes[3];
	enum sizes{ TOTAL, USED, FREE };

	nSizes[TOTAL] = sInfo.fi_total_blocks;
	nSizes[FREE] = sInfo.fi_free_blocks;

	int nScale = sInfo.fi_block_size % 1024;
	if( nScale > 0 )
	{
		nSizes[TOTAL] *= nScale;
		nSizes[FREE] *= nScale;
	}	
	nSizes[USED] = nSizes[TOTAL] - nSizes[FREE];

	/* Print the sizes, column aligned */
	char zNumber[128];
	int n;
	for( n = 0; n < 3; n++ )
	{
		sprintf( zNumber, "%Ld\t", nSizes[n] );
		printf( zNumber );
		if( strlen( zNumber ) < 9 )
			printf( "\t" );
	}

	double vPercentage = ( 100.0f - ( ( (double)sInfo.fi_free_blocks / (double)sInfo.fi_total_blocks ) * 100.0f ) );
	printf( "%.1f%%\t", vPercentage );

	printf( "%s\n", zVolume );

	return nError;
}

/* Display details for all mounted volumes */
status_t all_volumes( void )
{
	status_t nError = EOK;
	int nMountCount, nFd, n;
	char zVolume[PATH_MAX];
	fs_info sInfo;

	print_banner();

	nMountCount = get_mount_point_count();
	for( n = 0; n < nMountCount; n++ )
	{
		if( get_mount_point( n, zVolume, PATH_MAX ) < 0 )
			continue;

		nFd = open( zVolume, O_RDONLY );
		if( nFd < 0 )
			continue;
		else
		{
			nError = get_fs_info( nFd, &sInfo );
			if( nError == 0 )
				display_volume( sInfo, zVolume );
			close( nFd );
		}
	}

	return nError;
}

/* Display details only for the volume the given path resides on */
status_t one_volume( char *zPath )
{
	status_t nError = ENODEV;
	int nMountCount, nFd, n;
	char zVolume[PATH_MAX];
	fs_info sInfo;
	struct stat sStat;

	/* Stat the node at zPath and get it's volume device ID */
	if( stat( zPath, &sStat ) < 0 )
		nError = errno;
	else
	{
		nMountCount = get_mount_point_count();
		for( n = 0; n < nMountCount; n++ )
		{
			if( get_mount_point( n, zVolume, PATH_MAX ) < 0 )
				continue;

			nFd = open( zVolume, O_RDONLY );
			if( nFd < 0 )
				continue;
			else
			{
				nError = get_fs_info( nFd, &sInfo );
				if( nError < 0 )
					nError = ENXIO;
				else if( sStat.st_dev == sInfo.fi_dev )
				{
					/* The volume and node device handles match, this must be our volume */
					print_banner();
					display_volume( sInfo, zVolume );
					break;
				}
			}
			close( nFd );
		}
	}

	return nError;
}

/* This is a basic implementation of the df utility. No switches are supported. The
   one and only argument is expected to be a filesystem path. If no arguments are
   given, all mounted volumes are displayed. */
int main( int argc, char *argv[] )
{
	status_t nError;

	if( argc == 1 )
		nError = all_volumes();
	else
		nError = one_volume( argv[1] );

	return ( ( nError == 0 ) ? EXIT_SUCCESS : EXIT_FAILURE );
}

