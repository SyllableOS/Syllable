#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>

#include <sys/stat.h>

#include <atheos/device.h>



int stat_dev( const char* pzPath )
{
    int nDev = open( pzPath, O_RDONLY );
    struct stat sStat;
    device_geometry sGeom;
    if ( nDev < 0 ) {
	printf( "Error: Failed to open device %s: %s\n", pzPath, strerror( errno ) );
	return( -1 );
    }
    if ( fstat( nDev, &sStat ) < 0 ) {
	printf( "Error: Failed to stat device %s: %s\n", pzPath, strerror( errno ) );
	close( nDev );
	return( -1 );
    }
    if ( /*S_ISBLK( sStat.st_mode ) &&*/ ioctl( nDev, IOCTL_GET_DEVICE_GEOMETRY, &sGeom ) >= 0 ) {
	printf( "Size:             %Ld (%LdMb)\n", sGeom.sector_count * sGeom.bytes_per_sector, (sGeom.sector_count * sGeom.bytes_per_sector / (1024*1024)) );
	printf( "Cylinders:        %d\n", sGeom.cylinder_count );
	printf( "Sectors:          %d\n", sGeom.sectors_per_track );
	printf( "Heads:            %d\n", sGeom.head_count );
	printf( "Bytes per sector: %d\n", sGeom.bytes_per_sector );
	printf( (sGeom.read_only) ? "Read only\n" : "Read/write\n" );
	printf( (sGeom.removable) ? "Removable disk\n" : "Fixed disk\n" );
    } else {
	printf( "Size: %Ld\n", sStat.st_size );
    }
    close( nDev );
    return( 0 );
}

int main( int argc, char** argv )
{
    int i;
    
    for ( i = 1 ; i < argc ; ++i ) {
	stat_dev( argv[i] );
    }
    return( 0 );
}
