#include <atheos/kernel.h>

#include <util/application.h>
#include <util/message.h>
#include <util/messenger.h>
#include <appserver/protocol.h>

#include <getopt.h>

using namespace os;

void usage() {
	printf( "shutdown: shuts down or reboots the computer, or logs off the current user.\n" );
	printf( "\n" );
	printf( "Usage:\n" );
	printf( "\t shutdown [-s | -r | -l] [-f]\n" );
	printf( "\n" );
	printf( "Options:\n" );
	printf( "\t -s : shutdown  (this is the default)\n" );
	printf( "\t -r : reboot\n" );
	printf( "\t -l : log off\n" );
	printf( "\t -f : force shutdown or reboot - don't use the appserver, just shutdown or reboot immediately\n" );
	printf( "\t       (only works with -s  or -r)\n" );
}

int main( int nArgc, char* pzArgv[] )
{
	int nAction = 1;
	/* nAction: 0 = logout,  1 = shutdown,  2 = restart  (see Dock.cpp) */
	bool bForce = false;
	int nResult;
	int c;
	char* pzArg0 = strrchr( pzArgv[0], '/' );
	if( pzArg0 == NULL ) {
		pzArg0 = pzArgv[0];
	} else {
		pzArg0++;  /* skip the '/' */
	}
	if( !strcasecmp( "logoff", pzArg0 ) ) {
		nAction = 0;
	} else if( !strcasecmp( "reboot", pzArg0 ) ) {
		nAction = 2;
	}
	while( ( c = getopt( nArgc, pzArgv, "srlf?n:" ) ) != -1 )
	{
		switch ( c )
		{
		case 0:
			break;
		case 's':
			nAction = 1;
			break;
		case 'r':
			nAction = 2;
			break;
		case 'l':
			nAction = 0;
			break;
		case 'f':
			bForce = true;
			break;
		case 'n':  /* send specific action to the appserver... may be useful for debugging & testing */
			sscanf( optarg, "%i", &nAction );
			break;
		case '?':
		case ':':
		default:
			usage();
			return( 1 );
			break;
		}
	}

	if( nAction == 1 ) {
		printf( "Shutting down%s...\n", (bForce ? " (forced)" : "") );
	} else if( nAction == 2 ) {
		printf( "Rebooting%s...\n", (bForce ? " (forced)" : "") );
	} else if( nAction == 0 ) {
		printf( "Logging out...\n" );
	} else {
		printf( "Sending unknown action %i\n", nAction );
	}
	if( bForce && nAction == 1 ) { /* shutdown, forced */
		apm_poweroff();
		return( 1 );
	}
	if( bForce && nAction == 2 ) { /* reboot, forced */
		reboot();
		return( 1 );
	}
	Application* pcApp = new Application( "application/shutdown-util" );
	pcApp->Unlock();
	Messenger cMsg( pcApp->GetServerPort() );
	Message cMessage( DR_CLOSE_WINDOWS );
	cMessage.AddInt32( "action", nAction );
	nResult = cMsg.SendMessage( &cMessage );
	delete( pcApp );
	return( nResult );
}

