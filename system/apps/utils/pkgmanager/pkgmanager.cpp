#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <util/optionparser.h>
#include <util/regexp.h>
#include <storage/directory.h>
#include <storage/symlink.h>

using namespace os;

static bool g_bVerbose    = false;
static bool g_bQuiet      = false;
static bool g_bRunScripts = true;

static const char* g_pzLinkDir = "/atheos/autolnk";

static void usage( OptionParser& cOpts, bool bFull )
{
    std::string cCmd = cOpts.GetArgs()[0];
    uint nPos = cCmd.rfind( '/' );
    if ( nPos != std::string::npos ) {
	cCmd = std::string( cCmd.begin() + nPos + 1, cCmd.end() );
    }
    printf( "USAGE: %s {--add|-a}|{--remove|-r}[-s][-q|-v] PATH1 [PATH2 ... PATHn]\n", cCmd.c_str() );
    if ( bFull ) {
	printf( "\n" );
	printf( "Run '%s -a packet_path' after unpacking a command-line or library\n" \
		"software package to make the package files accessible to the system.\n\n" \

		"%s will then recurse down the package directory tree building a mirror\n" \
		"using symlinks inside \"/atheos/autolnk/*\". After the directory tree has been\n" \
		"mirrored %s will check if the package directory contain a script or\n" \
		"executable named pkgsetup.sh and if it is found it will be started with the\n" \
		"package directory as the first argument\n\n" \
		"You should also run %s to remove the symlinks again before\n" \
		"uninstalling the package.\n" \
		"Run '%s -r packet_path' before removing the package directory to remove\n" \
		"all symlinks pointing into the package directory. After all the symlinks has\n" \
		"been removed %s will check if the package directory contain a script or\n" \
		"executable named pkgcleanup.sh and if it is found it will be started with the\n" \
		"package directory as the first argument\n\n" \
		"It is also possible to run '%s -r' after the actual package directory\n" \
		"been removed but if the package had a pkgcleanup.sh script it will obviously\n" \
		"not be executed.\n\n" \
		"OPTIONS:\n", cCmd.c_str(), cCmd.c_str(), cCmd.c_str(), cCmd.c_str(),
		cCmd.c_str(), cCmd.c_str(), cCmd.c_str() );
	cOpts.PrintHelpText();
    }
    
}

status_t create_dir( const std::string& cPath )
{
    struct stat sStat;
    if ( lstat( cPath.c_str(), &sStat ) >= 0 ) {
	if ( S_ISDIR( sStat.st_mode ) == false ) {
	    fprintf( stderr, "Error: %s already exists but is not a directory\n", cPath.c_str() );
	    return( -1 );
	} else {
	    return( 0 );
	}
    } else {
	if ( errno != ENOENT ) {
	    fprintf( stderr, "Error: failed to stat %s : %s\n", cPath.c_str(), strerror(errno) );
	    return( -1 );
	} else {
	    if ( mkdir( cPath.c_str(), 0777 ) < 0 ) {
		fprintf( stderr, "Error: failed to create directory %s : %s\n", cPath.c_str(), strerror(errno) );
		return( -1 );
	    } else {
		if ( g_bQuiet == false ) {
		    printf( "Make directory %s\n", cPath.c_str() );
		}
		return( 0 );
	    }
	}
    }
}

static status_t link_directory( const std::string& cSrc, const std::string& cDst )
{
    FSNode cDstNode;

    if ( cDstNode.SetTo( cDst, O_RDONLY | O_NOTRAVERSE ) < 0 ) {
	if ( errno == ENOENT ) {
	    if ( symlink( cSrc.c_str(), cDst.c_str() ) < 0 ) {
		fprintf( stderr, "Error: failed to create symlink %s : %s\n", cDst.c_str(), strerror(errno) );
		return( -1 );
	    } else {
		return( 0 );
	    }
	} else {
	    fprintf( stderr, "Error: failed to open destination directory %s : %s\n", cDst.c_str(), strerror(errno) );
	    return( -1 );
	}
    } 

else {
	if ( cDstNode.IsLink() ) {
	    if ( g_bQuiet == false ) {
		printf( "Replace symlink %s with directory\n", cDst.c_str() );
	    }
	    Directory cDstDir;
	    if ( cDstDir.SetTo( cDst ) < 0 ) {
		fprintf( stderr, "Error: failed to open destination directory %s : %s\n", cDst.c_str(), strerror(errno) );
		return( -1 );
	    }
	    os::String cRealPath;
	    if ( cDstDir.GetPath( &cRealPath ) < 0 ) {
		fprintf( stderr, "Error: failed to get real path to destination directory %s : %s\n", cDst.c_str(), strerror(errno) );
		return( -1 );
	    }
	    if ( unlink( cDst.c_str() ) < 0 ) {
		fprintf( stderr, "Error: failed to delete destination symlink %s : %s\n", cDst.c_str(), strerror(errno) );
		return( -1 );
	    }
	    if ( mkdir( cDst.c_str(), 0777 ) < 0 ) {
		fprintf( stderr, "Error: failed to create destination directory %s : %s\n", cDst.c_str(), strerror(errno) );
		return( -1 );
	    }
	    if ( link_directory( cRealPath.str(), cDst ) < 0 ) {
		fprintf( stderr, "Error: failed to convert destination symlink to directory %s : %s\n", cDst.c_str(), strerror(errno) );
		return( -1 );
	    }
	}
    }

    if ( create_dir( cDst ) < 0 ) {
	return( -1 );
    }

    try {
	Directory cDir( cSrc );
	if ( g_bVerbose ) {
	    printf( "Link files from %s\n", cSrc.c_str() );
	}
	os::String cName;
	while( cDir.GetNextEntry( &cName ) == 1 ) {
	    if ( cName == "." || cName == ".." ) {
		continue;
	    }
	    struct stat sStat;
	    try {
		FSNode cNode( cDir, cName, O_RDONLY | O_NONBLOCK | O_NOTRAVERSE );
		cNode.GetStat( &sStat, false );
	    } catch( os::errno_exception& cExc ) {
		fprintf( stderr, "Error: failed to open file %s : %s\n", (cSrc + "/" + cName.str()).c_str(), cExc.error_str() );
		continue;
	    } catch(...) {
		fprintf( stderr, "Error: failed to open file %s : unknown error\n", (cSrc + "/" + cName.str()).c_str() );
		continue;
	    }
	    std::string cSrcPath = cSrc + "/" + cName.str();
	    std::string cDstPath = cDst + "/" + cName.str();
	    if ( S_ISDIR( sStat.st_mode ) ) {
		link_directory( cSrcPath, cDstPath );
	    }  else {
		bool bReplace = false;
		struct stat sDstStat;
		if ( lstat( cDstPath.c_str(), &sDstStat ) < 0 ) {
		    if ( errno != ENOENT ) {
			fprintf( stderr, "Error: failed to stat %s : %s\n", cDstPath.c_str(), strerror(errno) );
			continue;
		    }
		} else {
		    if ( S_ISLNK( sDstStat.st_mode ) == false ) {
			fprintf( stderr, "Warning: %s already exists but is not a symlink\n", cDstPath.c_str() );
			continue;
		    } else {
			bReplace = true;
			if ( unlink( cDstPath.c_str() ) < 0 ) {
			    fprintf( stderr, "Error: failed to remove old link %s : %s\n", cDstPath.c_str(), strerror(errno) );
			    continue;
			}
		    }
		}
		if ( symlink( cSrcPath.c_str(), cDstPath.c_str() ) < 0 ) {
		    fprintf( stderr, "Error: failed to create symlink %s : %s\n", cDstPath.c_str(), strerror(errno) );
		} else if ( g_bVerbose ) {
		    if ( bReplace ) {
			printf( "  %s -> %s (R)\n", cSrcPath.c_str(), cDstPath.c_str() );
		    } else {
			printf( "  %s -> %s\n", cSrcPath.c_str(), cDstPath.c_str() );
		    }
		}
	    }
	}
    } catch( os::errno_exception& cExc ) {
	fprintf( stderr, "Error: failed to open directory %s : %s\n", cSrc.c_str(), cExc.error_str() );
	return( -1 );
    } catch(...) {
	fprintf( stderr, "Error: failed to open directory %s : unknown error\n", cSrc.c_str() );
	return( -1 );
    }
    return( 0 );
}
/*
std::string operator+(const char* pzStr1, const std::string& cStr2 )
{
    return( std::string(pzStr1) + cStr2 );
}
*/

status_t add_packages( OptionParser& cOpts )
{
    const std::vector<os::String>& cDirs = cOpts.GetFileArgs();
    
    if ( cDirs.empty() ) {
	fprintf( stderr, "Error: no package directories specified.\n" );
	return( -1 );
    }
    if ( create_dir( g_pzLinkDir ) < 0 ) {
	return( -1 );
    }
    
    RegExp cIgnoreRe( "autolnk|bin|sbin|lib|libexec|man|var|etc|share|local|include|info|src|glibc.*|.*\\.bak|.*\\.old|.*\\.new|.*~", true, true );
    
    for ( uint i = 0 ; i < cDirs.size() ; ++i ) {
	std::string cPath = cDirs[i];

	while( cPath.empty() == false && cPath[cPath.size()-1] == '/' ) {
	    cPath.resize( cPath.size() - 1 );
	}
	
	std::string cName = cPath;
	if ( cName.rfind( '/' ) != std::string::npos ) {
	    cName = cName.substr( cName.rfind( '/' ) + 1 );
	}
	if ( cName == "." || cName == ".." || cIgnoreRe.Match( cName ) ) {
	    if ( g_bQuiet == false ) {
		printf( "Ignoring directory %s\n", cDirs[i].c_str() );
	    }
	    continue;
	}
	try {
	    Directory cDir( cPath );

	    os::String cName;
	    while( cDir.GetNextEntry( &cName ) == 1 ) {
		if ( cName == ".." || cName == "." ) {
		    continue;
		}
		struct stat sStat;
		os::String cSource = os::String( cPath ) + os::String( "/" ) + cName;
		if ( stat( cSource.c_str(), &sStat ) < 0 ) {
		    fprintf( stderr, "Error: Failed to stat %s : %s\n", cSource.c_str(), strerror(errno) );
		    continue;
		}
		if ( S_ISDIR( sStat.st_mode ) == false ) {
		    continue;
		}
//		if ( cName == "bin" || cName == "sbin" || cName == "lib" || cName == "libexec" ||
//		     cName == "include" || cName == "man" || cName == "info" ) {
		    std::string cDest = g_pzLinkDir;
		    cDest += '/';
		    cDest += cName;
		    Directory( cSource ).GetPath( &cSource );

		    link_directory( cSource, cDest );

			if( cName == "man" )
			{
				std::string cScriptArgs = "manaddpackage.sh ";
				cScriptArgs += cPath;
				cScriptArgs += "/man";

				if( g_bQuiet == false )
					printf( "Adding manual pages: %s\n", cScriptArgs.c_str() );

				system( cScriptArgs.c_str() );
			}

//		}
	    }
	    if ( g_bRunScripts ) {
		std::string cScriptArgs = cPath;
		cScriptArgs += "/pkgsetup.sh";
		struct stat sStat;
		if ( stat( cScriptArgs.c_str(), &sStat ) >= 0 ) {
		    if ( g_bQuiet == false ) {
			printf( "Run setup script: %s\n", cScriptArgs.c_str() );
		    }
		    cScriptArgs += " " + cPath + "/";
		    system( cScriptArgs.c_str() );
		}
	    }
	} catch( os::errno_exception& cExc ) {
	    fprintf( stderr, "Error: failed to open directory %s : %s\n", cDirs[i].c_str(), cExc.error_str() );
	} catch(...) {
	    fprintf( stderr, "Error: failed to open directory %s : unknown error\n", cDirs[i].c_str() );
	}
    }
    return( 0 );
}

static status_t unlink_directory( const std::string& cPath, const std::string& cBasePath )
{
    try {
	Directory cDir( cPath, O_RDONLY | O_NOTRAVERSE );
	os::String cName;

	std::vector<os::String> cFiles;
	
	while( cDir.GetNextEntry( &cName ) == 1 ) {
	    if ( cName == "." || cName == ".." ) {
		continue;
	    }
	    cFiles.push_back( cName );
	}
	bool bLinksRemoved = false;
	for ( uint i = 0 ; i < cFiles.size() ; ++i ) {
	    cName = cFiles[i];
	    struct stat sStat;
	    try {
		FSNode cNode( cDir, cName, O_RDONLY | O_NONBLOCK | O_NOTRAVERSE );
		cNode.GetStat( &sStat, false );
	    } catch( os::errno_exception& cExc ) {
		fprintf( stderr, "Error: failed to open file %s : %s\n", (cPath + "/" + cName.str()).c_str(), cExc.error_str() );
		continue;
	    } catch(...) {
		fprintf( stderr, "Error: failed to open file %s : unknown error\n", (cPath + "/" + cName.str()).c_str() );
		continue;
	    }
	    std::string cDstPath = cPath + "/" + cName.str();
	    if ( S_ISDIR( sStat.st_mode ) ) {
		unlink_directory( cDstPath, cBasePath );
	    }  else if ( S_ISLNK( sStat.st_mode ) ) {
		try {
		    SymLink cLink( cDstPath );
		    std::string cLnkPath = cLink.ReadLink();
		    if ( strncmp( cBasePath.c_str(), cLnkPath.c_str(), cBasePath.size() ) == 0 ) {
			if ( g_bQuiet == false ) {
			    printf( "Remove link %s (%s)\n", cDstPath.c_str(), cLnkPath.c_str() );
			}
			if ( unlink( cDstPath.c_str() ) < 0 ) {
			    fprintf( stderr, "Error: failed to remove old link %s : %s\n", cDstPath.c_str(), strerror(errno) );
			    continue;
			}
			bLinksRemoved = true;
		    }
		} catch( os::errno_exception& cExc ) {
		    fprintf( stderr, "Error: failed to open symlink %s : %s\n", cDstPath.c_str(), cExc.error_str() );
		} catch(...) {
		    fprintf( stderr, "Error: failed to open symlink %s : unknown error\n", cDstPath.c_str() );
		}
	    } else {
		fprintf( stderr, "Warning: %s is not a symbolic link or directory. Not removing\n", cDstPath.c_str() );
	    }
	}
	if ( bLinksRemoved ) {
	    if ( rmdir( cPath.c_str() ) < 0 ) {
		if ( errno != ENOTEMPTY ) {
		    fprintf( stderr, "Error: failed to remove directory %s : %s\n", cPath.c_str(), strerror(errno) );
		}
	    } else {
		if ( g_bQuiet == false ) {
		    printf( "Remove directory %s\n", cPath.c_str() );
		}
	    }
	}
    } catch( os::errno_exception& cExc ) {
	fprintf( stderr, "Error: failed to open directory %s : %s\n", cPath.c_str(), cExc.error_str() );
	return( -1 );
    } catch(...) {
	fprintf( stderr, "Error: failed to open directory %s : unknown error\n", cPath.c_str() );
	return( -1 );
    }
    return( 0 );
}

status_t remove_packages( OptionParser& cOpts )
{
    const std::vector<os::String>& cDirs = cOpts.GetFileArgs();
    
    if ( cDirs.empty() ) {
	fprintf( stderr, "Error: no package directories specified.\n" );
	return( -1 );
    }
    
    for ( uint i = 0 ; i < cDirs.size() ; ++i ) {
	std::string cPath = cDirs[i];

	while( cPath.empty() == false && cPath[cPath.size()-1] == '/' ) {
	    cPath.resize( cPath.size() - 1 );
	}
	if ( cPath.empty() ) {
	    if ( g_bQuiet == false ) {
		printf( "Ignoring directory %s\n", cDirs[i].c_str() );
	    }
	    continue;
	}
	std::string cName = cPath;
	cPath += '/';
	
	if ( cName.rfind( '/' ) != std::string::npos ) {
	    cName = cName.substr( cName.rfind( '/' ) + 1 );
	}
	if ( cName == "." || cName == ".." ) {
	    if ( g_bQuiet == false ) {
		printf( "Ignoring directory %s\n", cDirs[i].c_str() );
	    }
	    continue;
	}
	if ( unlink_directory( g_pzLinkDir, cPath ) >= 0 ) {
	    if ( g_bRunScripts ) {
		std::string cScriptArgs = cPath;
		cScriptArgs += "pkgcleanup.sh";
		struct stat sStat;
		if ( stat( cScriptArgs.c_str(), &sStat ) >= 0 ) {
		    if ( g_bQuiet == false ) {
			printf( "Run cleanup script: %s\n", cScriptArgs.c_str() );
		    }
		    cScriptArgs += " " + cPath;
		    system( cScriptArgs.c_str() );
		}
	    }
	}
    }
    return( 0 );
}

int main( int argc, char** argv )
{
    OptionParser cOpts( argc, argv );

    OptionParser::argmap asArgs[] = {
	{ "add",	'a',	"Add package directory." },
	{ "remove",	'r',	"Remove package directory." },
	{ "no-scripts",	's',	"Don't run the optional pkgsetup.sh (for --add) or " \
	  "pkgcleanup.sh (for --remove) after adding or removing the symbolic links." },
	{ "verbose",	'v',	"Print more progress information." },
	{ "quiet",	'q',	"Dont print any progress information." },
	{ "help",	'h',	"Print this help screen and exit." },
	{ "version",	'V',	"Print version information and exit." },
	{ NULL, 0, NULL }
    };
    cOpts.AddArgMap( asArgs );
    cOpts.ParseOptions( "arsvqhV" );

    g_bQuiet = cOpts.FindOption( 'q' ) != NULL;

    if ( g_bQuiet == false ) {
	g_bVerbose = cOpts.FindOption( 'v' ) != NULL;
    }

    g_bRunScripts = cOpts.FindOption( 's' ) == NULL;
    
    if ( cOpts.FindOption( 'h' ) ) {
	usage( cOpts, true );
	return( 0 );
    } else if ( cOpts.FindOption( 'V' ) ) {
	printf( "%s version 0.1\n", argv[0] );
	return( 0 );
    } else if ( cOpts.FindOption( 'a' ) ) {
	if ( add_packages( cOpts ) < 0 ) {
	    return( 1 );
	}
    } else if ( cOpts.FindOption( 'r' ) ) {
	if ( remove_packages( cOpts ) < 0 ) {
	    return( 1 );
	}	    
    } else {
	fprintf( stderr, "Error: no action specified\n" );
	return( 1 );
    }
    return(  0 );
}
