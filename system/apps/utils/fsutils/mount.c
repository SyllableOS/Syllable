#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>
#include <getopt.h>
#include <limits.h>

#include <atheos/kernel.h>
#include <atheos/filesystem.h>

int g_nShowHelp    = 0;
int g_nShowVersion = 0;
int g_nReadOnly	   = 0;
int g_nVerbose	   = 0;

static struct option const long_opts[] =
{
  {"fs-opts", required_argument, NULL, 'o' },
  {"fs-type", required_argument, NULL, 't' },
  {"read-only", no_argument, &g_nReadOnly, 1 },
  {"read-write", no_argument, &g_nReadOnly, 0 },
  {"help", no_argument, &g_nShowHelp, 1},
  {"version", no_argument, &g_nShowVersion, 1},
  {"verbose", no_argument, &g_nVerbose, 1},
  {NULL, 0, NULL, 0}
};

void usage( const char* pzStr, int nLevel )
{
  printf( "Usage: %s [device] mountpnt [-hVvrwto]\n", pzStr );
  if ( nLevel > 0 ) {
    printf( "  -h --help           display this help and exit\n" );
    printf( "  -V --version        display version information and exit\n" );
    printf( "  -v --verbose        print extra information when mounting\n" );
    printf( "  -r --read-only      mount filesystem read-only\n" );
    printf( "  -w --read-write     mount filesystem read-write (default)\n" );
    printf( "  -t --fs-type=name   name of the filesystem to mount (afs,nfs,lfd,...)\n" );
    printf( "  -o --fs-opts=opts   filesystem dependent mount options\n" );
  }
}

static void remove_trailing_slashes( char* pzPath )
{
    int nLen = strlen( pzPath );
    while( nLen > 1 && pzPath[nLen - 1] == '/' ) {
	pzPath[--nLen] = '\0';
    }
}

int main( int argc, char** argv )
{
    int c;
    int nOpt;
    const char* pzFsType = NULL;
    char* pzFsArgs = NULL;
    char  zDevPath[PATH_MAX] = "";
    char  zDirPath[PATH_MAX] = "";
    int	nCount;
	int flags = 0;
  
    while( (c = getopt_long (argc, argv, "hvrwad:t:o:", long_opts, (int *) 0)) != EOF )
    {
	switch (c)
	{
	    case 0:
		break;
	    case 't':
		pzFsType = optarg;
		break;
	    case 'o':
		pzFsArgs = optarg;
		break;
	    case 'r':
		flags = MNTF_READONLY;
		break;
	    case 'v':
		g_nVerbose = true;
		break;
	    case 'V':
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
	printf( "Syllable mount V0.1.2 compiled %s\n", __DATE__ );
	exit( 0 );
    }
    nCount = argc - optind;
    if ( nCount != 1 && nCount != 2 ) {
	usage( argv[0], 0 );
	exit( 1 );
    }
    if ( nCount == 1 ) {
	strcpy( zDirPath, argv[optind++] );
	remove_trailing_slashes( zDirPath );
    } else {
	strcpy( zDevPath, argv[optind++] );
	strcpy( zDirPath, argv[optind++] );
	remove_trailing_slashes( zDevPath );
	remove_trailing_slashes( zDirPath );
    }

    if ( g_nVerbose ) {
	printf( "Mount %s %s as %s (fs: %s,fs-args: '%s')\n",
		(zDevPath[0]) ? zDevPath : "*nodev*", ((g_nReadOnly) ? "ro" : "rw"), zDirPath, pzFsType, (pzFsArgs) ? pzFsArgs : "" );
    }
    if ( mount( zDevPath, zDirPath, pzFsType, flags, pzFsArgs ) < 0 ) {
	fprintf( stderr, "Error: Failed to mount %s: %s\n", zDirPath, strerror( errno ) );
	return( 1 );
    }
    return( 0 );
}
