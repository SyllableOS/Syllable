#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <util/optionparser.h>
#include <util/string.h>
#include <util/catalog.h>
#include <storage/file.h>
#include <iostream>
#include "ccatalog.h"

using namespace os;
using namespace std;

void usage( OptionParser& cOpts, bool bFull )
{
	std::string cCmd = cOpts.GetArgs()[0];
	uint nPos = cCmd.rfind( '/' );
	if ( nPos != std::string::npos ) {
		cCmd = std::string( cCmd.begin() + nPos + 1, cCmd.end() );
	}
	
	if ( bFull ) {
		printf( "\n" );
		printf( "List string catalog contents.\n" );
		printf( "\n" );
		cOpts.PrintHelpText();
	}
	exit( 0 );
}

int main( int argc, char** argv )
{
	int nMode = 0;

    OptionParser cOpts( argc, argv );

	OptionParser::argmap asArgs[] = {
		{ "list",       'l', "List strings in catalog." },
		{ "set",        's', "Set string in catalog." },
		{ "get",        'g', "Get string in catalog." },
		{ "id",         'i', "List IDs in catalog." },
		{ "help",    	'h', "Print this help message." },
		{ NULL, 0, NULL }
	};

    cOpts.AddArgMap( asArgs );
    cOpts.ParseOptions( "lsgih" );

	if ( cOpts.FindOption( 'l' ) ) {
		nMode = 1;
	}

	if ( cOpts.FindOption( 's' ) ) {
		nMode = 2;
	}

	if ( cOpts.FindOption( 'g' ) ) {
		nMode = 3;
	}

	if ( cOpts.FindOption( 'i' ) ) {
		nMode = 4;
	}

	if ( cOpts.FindOption( 'h' ) ) {
		nMode = 0;
	}

	if( nMode == 1 && cOpts.GetFileArgs().size() > 0 ) {
		Catalog c;
		Catalog::const_iterator i;
		try {
			File	cFile( cOpts.GetFileArgs()[0] );
	
			c.Load( &cFile );

			for(i = c.begin(); i != c.end(); i++) {
				cout << (*i).first << "\t";
				cout << (*i).second.c_str() << endl;
			}
		} catch( ... ) {
			cout << "Error: Can't open file." << endl;
		}
	}

	if( nMode == 2 && cOpts.GetFileArgs().size() > 2 ) {
		Catalog c;
		
		try {
			File	cFile( cOpts.GetFileArgs()[0] );

			c.Load( &cFile );
		} catch( ... ) {
			cout << "Creating new catalog file." << endl;
		}

		c.SetString( atol(cOpts.GetFileArgs()[1].c_str()), cOpts.GetFileArgs()[2] );
		cout << atol(cOpts.GetFileArgs()[1].c_str()) << " = " << cOpts.GetFileArgs()[2].c_str() << endl;

		try {
			File	cFile( cOpts.GetFileArgs()[0], O_CREAT );
			c.Save( &cFile );
		} catch( ... ) {
			cout << "Error: Can't create new file." << endl;
		}
	}

	if( nMode == 3 && cOpts.GetFileArgs().size() > 1 ) {
		Catalog c;
		
		try {
			File	cFile( cOpts.GetFileArgs()[0] );
			c.Load( &cFile );
			cout << c.GetString( atol(cOpts.GetFileArgs()[1].c_str()) ).c_str() << endl;
		} catch( ... ) {
			cout << "Can't open file." << endl;
		}
	}

	if( nMode == 4 && cOpts.GetFileArgs().size() > 0 ) {
		CCatalog c;
		CCatalog::const_iterator i;
		try {
			File	cFile( cOpts.GetFileArgs()[0] );
	
			c.Load( &cFile );

			for(i = c.begin(); i != c.end(); i++) {
				cout << c.GetMnemonic((*i).first).c_str() << "\t";
				cout << (*i).second.c_str() << endl;
			}
		} catch( ... ) {
			cout << "Error: Can't open file." << endl;
		}
	}

	if( nMode == 0 ) {
		usage( cOpts, true );
	}
}

