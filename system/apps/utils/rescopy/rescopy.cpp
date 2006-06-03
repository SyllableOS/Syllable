/*
 *  AtheOS resource manipulation tool
 *  Copyright (C) 2001 Kurt Skauen
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of version 2 of the GNU Library
 *  General Public License as published by the Free Software
 *  Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <termios.h>

#include <storage/file.h>
#include <storage/tempfile.h>
#include <storage/path.h>
#include <util/resources.h>
#include <util/optionparser.h>
#include <util/string.h>

#include <vector>
#include <map>
#include <set>
#include <algorithm>

using namespace os;

struct ResourceDesc
{
    std::string m_cPath;
    std::string m_cName;
    std::string m_cType;
};

static void error( const String& cError )
{
    fprintf( stderr, cError.c_str() );
    exit( 1 );
}

static status_t copy_stream( StreamableIO& cDest, StreamableIO& cSource )
{
    for (;;) {
	char pBuffer[65536];
	ssize_t nLen = cSource.Read( pBuffer, sizeof(pBuffer) );
	if ( nLen < 0 ) {
	    return( -1 );
	}
	if ( cDest.Write( pBuffer, nLen ) != nLen ) {
	    return( -1 );
	}
	if ( nLen != sizeof(pBuffer) ) {
	    return( 0 );
	}
    }
}

static void list_resources( const std::string& cPath )
{
    try {
	File cResFile( cPath );
	try {
	    Resources cResources( &cResFile );
	    cResources.DetachStream();

	    printf( "NAME                     SIZE  TYPE\n" );

	    std::vector<String> cList;
	    for ( int i = 0 ; i < cResources.GetResourceCount() ; ++i ) {
		String cStr;
		cStr.Format( "%-20s %8d  <%s>\n",
			     cResources.GetResourceName( i ).c_str(),
			     cResources.GetResourceSize( i ),
			     cResources.GetResourceType( i ).c_str() );
		cList.push_back( cStr );
	    }
	    std::sort( cList.begin(), cList.end() );
	    for ( uint i = 0 ; i < cList.size() ; ++i ) {
		write( 1, cList[i].c_str(), cList[i].size() );
	    }
	} catch( errno_exception& cExc ) {
	    if ( cExc.error() == ENOENT ) {
		printf( "No resource section found.\n" );
	    } else {
		throw;
	    }
	}
    } catch( errno_exception& cExc ) {
	error( String().Format( "Error: failed to open '%s' : %s\n", cPath.c_str(), cExc.error_str() ) );
    } catch( ... ) {
	error( String().Format( "Error: failed to open '%s' : unknown error\n", cPath.c_str() ) );
    }
}

static status_t embed_resources( const std::string& cTarget, const std::string& cSource )
{
    int nError = system( String().Format( "objcopy -R .aresources --add-section \".aresources=%s\" \"%s\"\n",
					   cSource.c_str(), cTarget.c_str() ).c_str() );
    if ( nError == -1 ) {
	fprintf( stderr, "Error: failed to execute \"objcopy\" : %s\n", strerror(errno) );
	return( -1 );
    } else if ( nError != 0 ) {
	errno = ENOENT;
	return( -1 );
    } else {
	return( 0 );
    }
}

static status_t create_resource_file( SeekableIO* pcFile, const std::vector<ResourceDesc>& cResList, Resources* pcSource )
{
    try {
	Resources cRes( pcFile, 0LL, true );
	cRes.DetachStream();

	if ( pcSource != NULL ) {
	    for ( int i = 0 ; i < pcSource->GetResourceCount() ; ++i ) {
		std::string cName = pcSource->GetResourceName( i );
		bool	    bFound = false;
		for ( uint j = 0 ; j < cResList.size() ; ++j ) {
		    if ( cResList[j].m_cName == cName ) {
			bFound = true;
			break;
		    }
		}
		if ( bFound == false ) {
		    ResStream* pcSrc = pcSource->GetResourceStream( i );
		    if ( pcSrc == NULL ) {
			fprintf( stderr, "Error: failed to read source archive: %s\n", strerror(errno) );
			return( -1 );
		    }
		    ResStream* pcDst = cRes.CreateResource( pcSrc->GetName(), pcSrc->GetType(), pcSrc->GetSize() );
		    if ( pcDst == NULL ) {
			fprintf( stderr, "Error: failed to create resource '%s' : %s\n", cResList[i].m_cName.c_str(), strerror(errno) );
			return( -1 );
		    }
		    status_t nError = copy_stream( *pcDst, *pcSrc );
		    delete pcDst;
		    delete pcSrc;
		    if ( nError < 0 ) {
			fprintf( stderr, "Error: failed to copy '%s' from original archive : %s\n", cName.c_str(), strerror(errno) );
			return( -1 );
		    }
		}
	    }
	}
	
	for ( uint i = 0 ; i < cResList.size() ; ++i ) {
	    try {
		File cSource( cResList[i].m_cPath );

		ResStream* pcDst = cRes.CreateResource( cResList[i].m_cName, cResList[i].m_cType, cSource.GetSize( false ) );
		if ( pcDst == NULL ) {
		    fprintf( stderr, "Error: failed to create resource '%s' : %s\n", cResList[i].m_cName.c_str(), strerror(errno) );
		    return( -1 );
		}
		if ( copy_stream( *pcDst, cSource ) < 0 ) {
		    fprintf( stderr, "Error: failed to import file '%s' : %s\n", cResList[i].m_cPath.c_str(), strerror(errno) );
		    delete pcDst;
		    return( -1 );
		}
		delete pcDst;
	    } catch ( errno_exception& cExc ) {
		fprintf( stderr, "Error: failed to open '%s' : %s\n", cResList[i].m_cPath.c_str(), cExc.error_str() );
		return( -1 );
	    } catch( ... ) {
		fprintf( stderr, "Error: failed to open '%s' : unknown error\n", cResList[i].m_cPath.c_str() );
		return( -1 );
	    }
	}
    } catch ( errno_exception& cExc ) {
	fprintf( stderr, "Error: failed to create archive: %s\n", cExc.error_str() );
	return( -1 );
    } catch( ... ) {
	fprintf( stderr, "Error: failed to create archive: unknown error\n" );
	return( -1 );
    }
    return( 0 );
}

static void add_resources( const std::vector<os::String>& cFiles, bool bForce, bool bReplace )
{
    std::string cTargetPath = cFiles[0];

    std::vector<ResourceDesc> cResList;
    for ( uint i = 1 ; i < cFiles.size() ; ++i ) {
	ResourceDesc sEntry;
	int o1 = cFiles[i].const_str().find( '=' );
	uint o2 = cFiles[i].const_str().find( ':' );

	sEntry.m_cType = "application/octet-stream";
	if ( uint(o1) == std::string::npos ) {
	    o1 = -1;
	    if ( o2 == std::string::npos ) {
		sEntry.m_cName = cFiles[i];
	    } else {
		sEntry.m_cName = std::string( cFiles[i].begin(),cFiles[i].begin()+o2 );
	    }
	    uint nPos = sEntry.m_cName.rfind( '/' );
	    if ( nPos != std::string::npos ) {
		sEntry.m_cName = std::string( sEntry.m_cName.begin() + nPos + 1, sEntry.m_cName.end() );
	    }	    
	} else {
	    sEntry.m_cName = std::string( cFiles[i].begin(),cFiles[i].begin()+o1 );
	}
	if ( o2 == std::string::npos ) {
	    sEntry.m_cPath = std::string( cFiles[i].begin()+o1+1,cFiles[i].end() );
	} else {
	    sEntry.m_cPath = std::string( cFiles[i].begin()+o1+1, cFiles[i].begin()+o2 );
	    sEntry.m_cType = std::string( cFiles[i].begin()+o2+1, cFiles[i].end() );
	}
	cResList.push_back( sEntry );
    }
    struct stat sStat;

    if ( bForce ) {
	if ( unlink( cTargetPath.c_str() ) < 0 ) {
	    if ( errno != ENOENT ) {
		error( String().Format( "Error: failed to delete target '%s' : %s\n", cTargetPath.c_str(), strerror(errno) ) );
	    }
	}
    }
    if ( bForce || stat( cTargetPath.c_str(), &sStat ) < 0 ) {
	if ( bForce == false && errno != ENOENT ) {
	    error( String().Format( "Error: failed to open '%s' : %s\n", cTargetPath.c_str(), strerror(errno) ) );
	}
	try {
	    File cDestFile( cTargetPath, O_RDWR | O_CREAT | O_TRUNC );
	    if ( create_resource_file( &cDestFile, cResList, NULL ) < 0 ) {
		unlink( cTargetPath.c_str() );
		exit( 1 );
	    }
	} catch ( errno_exception& cExc ) {
	    error( String().Format( "Error: failed to create resource file '%s' : %s\n", cTargetPath.c_str(), cExc.error_str() ) );
	} catch( ... ) {
	    error( String().Format( "Error: failed to create resource file '%s' : unknown error\n", cTargetPath.c_str() ) );
	}
    } else {
	try {
	    TempFile cDestFile( "rcp", Path( cTargetPath.c_str() ).GetDir().GetPath() );
	    Resources* pcTarget = NULL;
	    if ( bReplace == false ) {
		File* pcFile = new File( cTargetPath, O_RDONLY );
		try {
		    pcTarget = new Resources( pcFile );
		} catch( ... ) {
		    delete pcFile;
		    throw;
		}
	    }
	    status_t nError = create_resource_file( &cDestFile, cResList, pcTarget );
	    delete pcTarget;
	    
	    if ( nError < 0 ) {
		exit( 1 );
	    }
	    cDestFile.Flush();
	    bool bArchive = false;
	    try {
		File cFile( cTargetPath );
		int nMagic;
		if ( cFile.Read( &nMagic, sizeof(int) ) == sizeof(int) && nMagic == Resources::RES_MAGIC ) {
		    bArchive = true;
		}
	    } catch (...) {
	    }
	    if ( bArchive ) {
		if ( rename( cDestFile.GetPath().c_str(), cTargetPath.c_str() ) < 0 ) {
		    cDestFile.Unlink();
		    error( String().Format( "Error: failed to overwrite '%s' : %s\n", cTargetPath.c_str(), strerror(errno) ) );
		} else {
		    cDestFile.Detatch();
		}
	    } else {
		embed_resources( cTargetPath, cDestFile.GetPath() );
	    }
	} catch ( errno_exception& cExc ) {
	    error( String().Format( "Error: failed to add resources to '%s' : %s\n", cTargetPath.c_str(), cExc.error_str() ) );
	} catch( ... ) {
	    error( String().Format( "Error: failed to add resources to '%s' : unknown error\n", cTargetPath.c_str() ) );
	}
    }
}

static status_t extract_resources( const std::vector<os::String>& cFiles, bool bPrint )
{
    std::string cSourcePath = cFiles[0];
    status_t    nError = 0;
    try {
	File cSrcFile( cSourcePath );
	Resources cRes( &cSrcFile );
	cRes.DetachStream();
	
	for ( uint i = 1 ; i < cFiles.size() ; ++i ) {
	    uint o1 = cFiles[i].const_str().find( '=' );

	    std::string cName;
	    std::string cPath;
	
	    if ( o1 == std::string::npos ) {
		cPath = cFiles[i];
		cName = cFiles[i];
	    } else {
		cPath = std::string( cFiles[i].begin(),cFiles[i].begin()+o1 );
		cName = std::string( cFiles[i].begin()+o1+1,cFiles[i].end() );
	    }

	    ResStream* pcSrc = cRes.GetResourceStream( cName );
	    if ( pcSrc == NULL ) {
		fprintf( stderr, "Error: failed to open resource '%s' : %s\n", cName.c_str(), strerror(errno) );
		nError = -1;
		continue;
	    }
	    try {
		File cDstFile;
		if ( bPrint ) {
		    if ( cDstFile.SetTo( dup( 1 ) ) < 0 ) {
			throw( errno_exception( "" ) );
		    }
		} else {
		    if ( cDstFile.SetTo( cPath, O_WRONLY | O_CREAT | O_TRUNC ) < 0 ) {
			throw( errno_exception( "" ) );
		    }
		}
		status_t nResult = copy_stream( cDstFile, *pcSrc );
		if ( nResult < 0 ) {
		    fprintf( stderr, "Error: failed to extract '%s' to '%s' : %s\n", cName.c_str(), cPath.c_str(), strerror(errno) );
		    nError = nResult;
		}
	    } catch( errno_exception& cExc ) {
		fprintf( stderr, "Error: could not create '%s' : %s\n", cPath.c_str(), cExc.error_str() );
		nError = -1;
	    } catch( ... ) {
		fprintf( stderr, "Error: could not create '%s' : unknown error\n", cPath.c_str() );
		nError = -1;
	    }
	    delete pcSrc;
	}
    } catch( errno_exception& cExc ) {
	fprintf( stderr, "Error: failed to open archive '%s' : %s\n", cSourcePath.c_str(), cExc.error_str() );
	errno = cExc.error();
	nError = -1;
    } catch( ... ) {
	fprintf( stderr, "Error: failed to open archive '%s' : unknown error\n", cSourcePath.c_str() );
	errno = EIO;
	nError = -1;
    }
    return( nError );
}




void usage( OptionParser& cOpts, bool bFull )
{
    std::string cCmd = cOpts.GetArgs()[0];
    uint nPos = cCmd.rfind( '/' );
    if ( nPos != std::string::npos ) {
	cCmd = std::string( cCmd.begin() + nPos + 1, cCmd.end() );
    }
    printf( "Usage:\n" );    
    printf( "  %s {--list|-l}         ARCHIVE\n", cCmd.c_str() );
    printf( "  %s {--add|-a} [-f]     ARCHIVE [R1=]P1[:T1] [R2=]P2[:T2] ... [Rn=]Pn[:Tn]\n", cCmd.c_str() );
    printf( "  %s {--replace|-r} [-f] ARCHIVE [R1=]P1[:T1] [R2=]P2[:T2] ... [Rn=]Pn[:Tn]\n", cCmd.c_str() );
    printf( "  %s {--extract|-x}      ARCHIVE [PATH1=]RES1 [PATH2=]RES2 ... [PATHn=]RESn\n", cCmd.c_str() );
    printf( "  %s {--cat|-c}          ARCHIVE RES1 RES2 ... RESn\n", cCmd.c_str() );
    printf( "  %s {--help|-h}\n", cCmd.c_str() );
    printf( "  %s {--version|-v]}\n", cCmd.c_str() );
    
    if ( bFull ) {
	printf( "\n" );
	printf( "Add, extract, and list resources embeded in executables or resouce files.\n" );
	printf( "\n" );
	cOpts.PrintHelpText();
    }
    exit( 0 );
}

int main( int argc, char** argv )
{
    OptionParser cOpts( argc, argv );

    OptionParser::argmap asArgs[] = {
	{ "list",    'l', "List resources." },
	{ "add",     'a', "Add resources." },
	{ "replace", 'r', "Replace resources." },
	{ "extract", 'x', "Extract resources from an executable, DLL or resource file." },
	{ "cat",     'c', "Copy resources to standard out." },
	{ "force",   'f', "Force overwriting of destination file." },
	{ "help",    'h', "Print this help screen." },
	{ "version", 'v', "Print version information and exit." },
	{ NULL, 0, NULL }
    };

    cOpts.AddArgMap( asArgs );
    cOpts.ParseOptions( "larxcfhv" );

    if ( cOpts.FindOption( 'h' ) ) {
	usage( cOpts, true );
    }
    if ( cOpts.FindOption( 'v' ) ) {
	printf( "%s version 0.1\n", argv[0] );
	exit( 0 );
    }
    
    if ( cOpts.FindOption( 'l' ) ) {
	if ( cOpts.GetFileArgs().size() < 1 ) {
	    error( "Error: no source file specified\n" );
	} else if ( cOpts.GetFileArgs().size() > 1 ) {
	    error( "Error: to many arguments\n" );
	}
	list_resources( cOpts.GetFileArgs()[0] );
    } else if ( cOpts.FindOption( 'a' ) || cOpts.FindOption( 'r' ) ) {
	if ( cOpts.GetFileArgs().size() < 2 ) {
	    error( "Error: To few arguments\n" );
	}
	add_resources( cOpts.GetFileArgs(), cOpts.FindOption( 'f' ) != NULL, cOpts.FindOption( 'r' ) != NULL );
    } else if ( cOpts.FindOption( 'x' ) || cOpts.FindOption( 'c' ) ) {
	if ( extract_resources( cOpts.GetFileArgs(), cOpts.FindOption( 'c' ) != NULL ) < 0 ) {
	    return( 1 );
	}
    }
    
    return( 0 );
}


