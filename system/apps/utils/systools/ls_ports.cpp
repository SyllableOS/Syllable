#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <atheos/msgport.h>
#include <atheos/threads.h>
#include <util/optionparser.h>
#include <util/string.h>

using namespace os;

void usage( OptionParser& cOpts, bool bFull )
{
    std::string cCmd = cOpts.GetArgs()[0];
    uint nPos = cCmd.rfind( '/' );
    if ( nPos != std::string::npos ) {
	cCmd = std::string( cCmd.begin() + nPos + 1, cCmd.end() );
    }
    
    if ( bFull ) {
	printf( "\n" );
	printf( "List active message ports.\n" );
	printf( "\n" );
	cOpts.PrintHelpText();
    }
    exit( 0 );
}

int main( int argc, char** argv )
{
	bool bPrivate = false;
	bool bPublic = true;

    OptionParser cOpts( argc, argv );

	OptionParser::argmap asArgs[] = {
		{ "private",    'p', "List private ports." },
		{ "all",    	'a', "List all ports." },
		{ "help",    	'h', "Print this help message." },
		{ NULL, 0, NULL }
	};

    cOpts.AddArgMap( asArgs );
    cOpts.ParseOptions( "pah" );

	if ( cOpts.FindOption( 'h' ) ) {
		usage( cOpts, true );
	}

	if ( cOpts.FindOption( 'p' ) ) {
		bPrivate = true;
		bPublic = false;
	}

	if ( cOpts.FindOption( 'a' ) ) {
		bPrivate = true;
		bPublic = true;
	}

    port_info sInfo;

	if( bPublic ) {
		get_port_info( -1, &sInfo );
		int nError = 1;
		printf( "Public Message Ports\n" );
		printf( "--------------------\n" );
		printf( "PortID Owner  Max Cur Name\n" );
		for (  ; nError == 1 ; nError = get_next_port_info( &sInfo ) ) {
			if( sInfo.pi_flags & MSG_PORT_PUBLIC ) {
				printf( "%-06d %-06d %-03d %-03d %s\n", sInfo.pi_port_id, sInfo.pi_owner, sInfo.pi_max_count, sInfo.pi_cur_count, sInfo.pi_name);
			}
		}
	}

	if( bPrivate ) {
		get_port_info( -1, &sInfo );
		int nError = 1;
		printf( "Private Message Ports\n" );
		printf( "---------------------\n" );
		printf( "PortID Owner  Max Cur Name\n" );
		for (  ; nError == 1 ; nError = get_next_port_info( &sInfo ) ) {
			if( !(sInfo.pi_flags & MSG_PORT_PUBLIC) ) {
				printf( "%-06d %-06d %-03d %-03d %s\n", sInfo.pi_port_id, sInfo.pi_owner, sInfo.pi_max_count, sInfo.pi_cur_count, sInfo.pi_name);
			}
		}
	}
}


