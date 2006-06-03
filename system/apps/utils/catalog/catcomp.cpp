#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <util/optionparser.h>
#include <util/string.h>
#include <util/catalog.h>
#include <util/locale.h>
#include <storage/file.h>
#include <storage/path.h>

#include "ccatalog.h"

#include "catcomp.h"

#include <iostream>

using namespace os;
using namespace std;

/*

Syntax:

// Comment

MNEMONIC[=ID]	String
MNEMONIC[=ID]	String
...

*/

Catalog* g_pcStrings;

void usage( OptionParser& cOpts, bool bFull )
{
	std::string cCmd = cOpts.GetArgs()[0];
	uint nPos = cCmd.rfind( '/' );
	if ( nPos != std::string::npos ) {
		cCmd = std::string( cCmd.begin() + nPos + 1, cCmd.end() );
	}
	
	if ( bFull ) {
		printf( g_pcStrings->GetString( MSG_DESCR ).c_str() );
		printf( "\n" );
		cOpts.PrintHelpText();
	}
	exit( 0 );
}

bool ParseCatalogDescription( const char *pzFileName, CCatalog& cStrings )
{
	int i;
	FILE *fp;
	char bfr[ 1024 ], *p;
	char ident[ 256 ], *q;
	char string[ 1024 ];
	char temp[ 256 ];
	int line = 0;
	uint32 nID = 0;
	bool ret = true;
		
	fp = fopen( pzFileName, "r" );
	if( fp ) {
		while( ! feof( fp ) ) {
			p = fgets( bfr, sizeof(bfr), fp );
			if( p ) {
				line++;
				while( *p && isspace(*p) ) p++;
				if( *p == '/' ) {
					// comment
				} else if( *p ) {
					q = ident;
					while( *p && !isspace( *p ) && *p != '=' ) {
						*q++ = *p++;
					}
					*q = 0;
					while( *p && isspace(*p) ) p++;
					if( *p == '=' ) {
						p++;
						while( *p && isspace(*p) ) p++;
						if( *p ) {
							q = temp;
							while( *p && !isspace( *p ) ) {
								*q++ = *p++;
							}
							*q = 0;
							nID = atol( temp );
							while( *p && isspace(*p) ) p++;
						} else {
							printf( g_pcStrings->GetString( MSG_SYNTAX_ERROR ).c_str(), line );
							printf( "%s\n", g_pcStrings->GetString( MSG_ID_NUMBER_MISSING ).c_str() );
							ret = false;
						}
					}
					if( *p ) {
						q = string;
						while( *p && *p != '\n' && *p != '\r' ) {
							if( *p == '\\' ) {	// Handle escape sequences
								p++;
								switch( *p ) {
									case 'n':
										*q++ = '\n';
										p++;
										break;
									case 't':
										*q++ = '\t';
										p++;
										break;
									case '\n':
										p++;
										while( *p && isspace(*p) ) p++;
										*q++ = ' ';
										break;
									default:
										*q++ = *p++;
								}
							} else {
								*q++ = *p++;
							}
						}
						*q = 0;
						
						// Remove spaces from the end of the string
						for( i = strlen( string ); i>0; i-- ) {
							if( isspace( string[i] ) ) {
								string[i] = '\0';
							} else {
								break;
							}
						}
						
				//		cout << ident << "=" << string << " (" << nID << ")" << endl;
						cStrings.SetString( nID, string );
						cStrings.SetMnemonic( nID, ident );
						nID++;
					} else {
						printf( g_pcStrings->GetString( MSG_SYNTAX_ERROR ).c_str(), line );
						printf( "%s\n", g_pcStrings->GetString( MSG_NO_TEXT_STRING ).c_str() );
						ret = false;
					}
				}
			}
		}
		fclose( fp );
	} else {
		cout << "Can't open file!" << endl;		
		ret = false;
	}

	return ret;
}

void LoadCatalog()
{
	Locale cLocale;
//	cout << cLocale.GetName( ).c_str() << endl;

	StreamableIO* pcCatalogStream = cLocale.GetLocalizedResourceStream( "catcomp.catalog" );
	g_pcStrings = new Catalog();
	try {
		g_pcStrings->Load( pcCatalogStream );
	} catch(...) {
		cout << "Error: Can't find catalog" << endl;
		exit(100);
	}
	g_pcStrings->AddRef();
	delete pcCatalogStream;
}

int main( int argc, char** argv )
{
	int nMode = 0;

	LoadCatalog();

    OptionParser cOpts( argc, argv );

	OptionParser::argmap asArgs[] = {
		{ "compile",    'c', g_pcStrings->GetString( MSG_HELP_COMPILE ).c_str() },
		{ "translate",  't', g_pcStrings->GetString( MSG_HELP_TRANSLATE ).c_str() },
		{ "help",    	'h', g_pcStrings->GetString( MSG_HELP_HELP ).c_str() },
		{ NULL, 0, NULL }
	};

    cOpts.AddArgMap( asArgs );
    cOpts.ParseOptions( "cth" );

	if ( cOpts.FindOption( 'c' ) ) {
		nMode = 1;
	}

	if ( cOpts.FindOption( 't' ) ) {
		nMode = 2;
	}

	if ( cOpts.FindOption( 'g' ) ) {
		nMode = 3;
	}

	if ( cOpts.FindOption( 'h' ) ) {
		nMode = 0;
	}

	if( nMode == 1 && cOpts.GetFileArgs().size() > 0 ) {
		CCatalog cStrings;
		FILE *fp;
		char *p;
		char *q;
		char pzFileName[ 256 ];
		
		strcpy( pzFileName, cOpts.GetFileArgs()[0].c_str() );
		
		if( !ParseCatalogDescription( pzFileName, cStrings ) ) {
			return -1;
		}

		p = pzFileName;
		while( *p ) {
			if( *p == '.' ) *p = 0;
			p++;
		}

		cout << g_pcStrings->GetString( MSG_CREATING_HEADER_FILE ).c_str() << " "
			 << ( String( pzFileName ) + String( ".h" ) ).c_str() << endl;

		fp = fopen( ( String( pzFileName ) + String( ".h" ) ).c_str(), "w" );
		if( fp ) {
			fprintf( fp, "#ifndef __F_CAT_%s_H__\n", os::Path( pzFileName ).GetLeaf().c_str() );
			fprintf( fp, "#define __F_CAT_%s_H__\n", os::Path( pzFileName ).GetLeaf().c_str() );
			fprintf( fp, "#include <util/catalog.h>\n");
			fprintf( fp, "#ifndef CATALOG_NO_IDS\n" );

			Catalog::const_iterator ic;
			for( ic = cStrings.begin(); ic != cStrings.end(); ic++ ) {
				fprintf( fp, "#define ID_%s %d\n", cStrings.GetMnemonic((*ic).first).c_str(), (*ic).first );
			}

			fprintf( fp, "#endif\n" );

			fprintf( fp, "#ifndef CATALOG_NO_LSTRINGS\n" );

			for( ic = cStrings.begin(); ic != cStrings.end(); ic++ ) {
				fprintf( fp, "#define %s os::LString( %d )\n", cStrings.GetMnemonic((*ic).first).c_str(), (*ic).first, cStrings.GetString( (*ic).first ).c_str() );
			}

			fprintf( fp, "#endif\n" );

			fprintf( fp, "#endif /* __F_CAT_%s_H__ */\n", pzFileName );

			fclose( fp );
		}

		cout << g_pcStrings->GetString( MSG_CREATING_CATALOG_FILE ).c_str() << " "
			 << ( String( pzFileName ) + String( ".catalog" ) ).c_str() << endl;

		try {
			File	cFile( ( String( pzFileName ) + String( ".catalog" ) ), O_CREAT );
			cStrings.Save( &cFile );
		} catch( ... ) {
			cout << "Error: Can't create new file." << endl;
		}

	}

	if( nMode == 2 && cOpts.GetFileArgs().size() > 0 ) {
		CCatalog cStrings;
		FILE *fp;
		char *p;
		char *q;
		char pzFileName[ 256 ];
		
		strcpy( pzFileName, cOpts.GetFileArgs()[0].c_str() );
		
		if( !ParseCatalogDescription( pzFileName, cStrings ) ) {
			return -1;
		}

		p = pzFileName;
		while( *p ) {
			if( *p == '.' ) *p = 0;
			p++;
		}

		cout << g_pcStrings->GetString( MSG_CREATING_CATALOG_FILE ).c_str() << " "
			 << ( String( pzFileName ) + String( ".catalog" ) ).c_str() << endl;

		try {
			File	cFile( ( String( pzFileName ) + String( ".catalog" ) ), O_CREAT );
			cStrings.Save( &cFile );
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
	
	if( nMode == 0 ) {
		usage( cOpts, true );
	}

	g_pcStrings->Release();
}





