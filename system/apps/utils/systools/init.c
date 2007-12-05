#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

int main( int argc, char **argv )
{
	char *pzProgram;
	char *pzAppserver = "/system/appserver", *pzASName;
	char *pzBootMode = "normal";
	int c;

	if( (pzProgram = strrchr( *argv, '/' )) != NULL )
		++pzProgram;
	else
		pzProgram = *argv;
	
	/* Parse command line */
	while( (c = getopt( argc, argv, "p:" )) != EOF )
	{
		switch( c )
		{
			case 'p':
				pzAppserver = optarg;
				break;
				
			default:
				break;
		}
	}
	
	argc -= optind;
	argv += optind;
	
	/* Grab boot mode if specified */
	if( *argv )
		pzBootMode = *argv;
	
	/* Determine name of appserver executable from path */
	if( (pzASName = strrchr( pzAppserver, '/' )) != NULL )
		++pzASName;
	else
		pzASName = pzAppserver;
	
	symlink( "boot", "/atheos" );
	symlink( "boot/system", "/system" );
	symlink( "boot/bin", "/bin" );
	symlink( "boot/usr", "/usr" );
	symlink( "boot/etc", "/etc" );
	symlink( "boot/home", "/home" );
	symlink( "boot/var", "/var" );
	symlink( "boot/tmp", "/tmp" );

	if( fork() == 0 ) {
		execl( pzAppserver, pzASName, NULL );
		exit( EXIT_FAILURE );
	}
	
	execl( "/system/init.sh", "init.sh", pzBootMode, NULL );
	printf( "Failed to run init script!\n" );
	
	return( EXIT_FAILURE );
}
