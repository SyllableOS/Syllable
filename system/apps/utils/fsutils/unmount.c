#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <getopt.h>
#include <limits.h>

#include <atheos/kernel.h>

static int g_bForce = 0;
static int g_nShowHelp = 0;
static int g_nShowVersion = 0;

static struct option const long_opts[] =
{
    {"force", no_argument, &g_bForce, 1 },
    {"help", no_argument, &g_nShowHelp, 1},
    {"version", no_argument, &g_nShowVersion, 1},
    {NULL, 0, NULL, 0}
};

void usage( const char* pzStr, int nLevel )
{
    printf( "Usage: %s mountpnt [-hvF]\n", pzStr );
    if ( nLevel > 0 ) {
	printf( "  -h --help           display this help and exit\n" );
	printf( "  -v --version        display version information and exit\n" );
	printf( "  -F --force          unmount filesystem even if files are open\n" );
    }
}

int main( int argc, char** argv )
{
    int c;
    char zPath[PATH_MAX];
    int  nLen;
    
    while( (c = getopt_long (argc, argv, "hvF", long_opts, (int *) 0)) != EOF )
    {
	switch (c)
	{
	    case 0:
		break;
	    case 'F':
		g_bForce = true;
		break;
	    case 'v':
		g_nShowVersion = true;
		break;
	    case 'h':
		g_nShowHelp = true;
		break;
	    default:
		usage( argv[0], 0 );
		return( 1 );
	}
    }

    if ( g_nShowHelp ) {
	usage( argv[0], 1 );
	exit( 0 );
    }
    if ( g_nShowVersion ) {
	printf( "AtheOS unmount V0.1.1 compiled %s\n", __DATE__ );
	exit( 0 );
    }
  
    if ( optind >= argc ) {
	usage( argv[0], 0 );
	return( 1 );
    }
    strcpy( zPath, argv[optind] );
    nLen = strlen( zPath );
    while( nLen > 1 && zPath[nLen - 1] == '/' ) {
	zPath[--nLen] = '\0';
    }
    
    if ( g_bForce ) {
	printf( "Force-unmount %s\n", zPath );
    }
    if ( unmount( zPath, g_bForce ) < 0 ) {
	printf( "Error: Failed to unmount %s: %s\n", zPath, strerror( errno ));
    }
    return( 0 );
}
