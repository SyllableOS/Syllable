#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <ctype.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

#include <appserver/keymap.h>

char *qualstrings[] = { "n", "s", "c", "o", "os", "C", "Cs", "Co", "Cos", NULL };

int parse_number( const char* pzString )
{
    int nValue;

    if ( pzString[0] == '\'' && pzString[1] == '\'' && (pzString[2] == '\0' || isspace(pzString[2])) ) {
	return( 0 );
    } else if ( pzString[0] == '\'' && pzString[2] == '\'' && (pzString[3] == '\0' || isspace(pzString[3])) ) {
	nValue = pzString[1];
    } else if ( pzString[0] == '\'' && pzString[1] == '\\' && pzString[3] == '\'' && (pzString[4] == '\0' || isspace(pzString[4])) ) {
	nValue = pzString[2];
    } else if ( pzString[0] == '0' && pzString[1] == 'x' ) {
	if ( sscanf( pzString, "%x", &nValue ) != 1 ) {
	    return( -1 );
	}
    } else if ( strcmp( pzString, "DEADKEY" ) == 0 ) {
    	return DEADKEY_ID;
    } else {
	if ( sscanf( pzString, "%d", &nValue ) != 1 ) {
	    return( -1 );
	}
    }
    return( nValue );
}

uint32 count_dead_keys( FILE* hFile )
{
    uint32 dead = 0;

    for (;;)
    {
	char zLineBuf[4096];
  
	if ( fgets( zLineBuf, 4096, hFile ) == NULL ) {
	    break;
	}

	if ( strncmp( zLineBuf, "DeadKey", 7 ) == 0 )
		dead++;
    }
    
    return dead;
}

int load_ascii_keymap( FILE* hFile, keymap* psKeymap )
{
    int nLine = 0;
    int nVersion = 0;
    int nDeadKeyIndex = 0;

    fseek( hFile, 0, SEEK_SET );

    memset( psKeymap, 0, sizeof(*psKeymap) );
    
    psKeymap->m_nCapsLock   = 0x3b;
    psKeymap->m_nScrollLock = 0x0f;
    psKeymap->m_nNumLock    = 0x22;
    psKeymap->m_nLShift     = 0x4b;
    psKeymap->m_nRShift     = 0x56;
    psKeymap->m_nLCommand   = 0x5d;
    psKeymap->m_nRCommand   = 0x67;
    psKeymap->m_nLControl   = 0x5c;
    psKeymap->m_nRControl   = 0x60;
    psKeymap->m_nLOption    = 0x66;
    psKeymap->m_nROption    = 0x5f;
    psKeymap->m_nMenu       = 0x68;
    
    nVersion = CURRENT_KEYMAP_VERSION;
    for (;;)
    {
	char zLineBuf[4096];

	nLine++;
    
	if ( fgets( zLineBuf, 4096, hFile ) == NULL ) {
	    break;
	}
	if ( zLineBuf[0] == '#' ) {
	    continue;
	}
	bool bEmpty = true;
	for ( int i = 0 ; zLineBuf[i] != 0 ; ++i ) {
	    if ( isspace(zLineBuf[i] ) == false ) {
		bEmpty = false;
		break;
	    }
	}
	if ( bEmpty ) {
	    continue;
	}
	if ( sscanf( zLineBuf, "Version = %d\n", &nVersion ) == 1 ) {
	    continue;
	}
	char zBuf[128];
    
	if ( sscanf( zLineBuf, "CapsLock = %7[0-9a-fx] \n", zBuf ) == 1 ) {
	    psKeymap->m_nCapsLock = parse_number( zBuf );
	    if ( psKeymap->m_nCapsLock == -1 ) {
		fprintf( stderr, "Syntax error at line %d\n", nLine );
		return( -1 );
	    }
	    continue;
	}
	if ( sscanf( zLineBuf, "ScrollLock = %7[0-9a-fx] \n", zBuf ) == 1 ) {
	    psKeymap->m_nScrollLock = parse_number( zBuf );
	    if ( psKeymap->m_nScrollLock == -1 ) {
		fprintf( stderr, "Syntax error at line %d\n", nLine );
		return( -1 );
	    }
	    continue;
	}
	if ( sscanf( zLineBuf, "NumLock = %7[0-9a-fx] \n", zBuf ) == 1 ) {
	    psKeymap->m_nNumLock = parse_number( zBuf );
	    if ( psKeymap->m_nNumLock == -1 ) {
		fprintf( stderr, "Syntax error at line %d\n", nLine );
		return( -1 );
	    }
	    continue;
	}
	if ( sscanf( zLineBuf, "LShift = %7[0-9a-fx] \n", zBuf ) == 1 ) {
	    psKeymap->m_nLShift = parse_number( zBuf );
	    if ( psKeymap->m_nLShift == -1 ) {
		fprintf( stderr, "Syntax error at line %d\n", nLine );
		return( -1 );
	    }
	    continue;
	}
	if ( sscanf( zLineBuf, "RShift = %7[0-9a-fx] \n", zBuf ) == 1 ) {
	    psKeymap->m_nRShift = parse_number( zBuf );
	    if ( psKeymap->m_nRShift == -1 ) {
		fprintf( stderr, "Syntax error at line %d\n", nLine );
		return( -1 );
	    }
	    continue;
	}
	if ( sscanf( zLineBuf, "LCommand = %7[0-9a-fx] \n", zBuf ) == 1 ) {
	    psKeymap->m_nLCommand = parse_number( zBuf );
	    if ( psKeymap->m_nLCommand == -1 ) {
		fprintf( stderr, "Syntax error at line %d\n", nLine );
		return( -1 );
	    }
	    continue;
	}
	if ( sscanf( zLineBuf, "RCommand = %7[0-9a-fx] \n", zBuf ) == 1 ) {
	    psKeymap->m_nRCommand = parse_number( zBuf );
	    if ( psKeymap->m_nRCommand == -1 ) {
		fprintf( stderr, "Syntax error at line %d\n", nLine );
		return( -1 );
	    }
	    continue;
	}
	if ( sscanf( zLineBuf, "LControl = %7[0-9a-fx] \n", zBuf ) == 1 ) {
	    psKeymap->m_nLControl = parse_number( zBuf );
	    if ( psKeymap->m_nLControl == -1 ) {
		fprintf( stderr, "Syntax error at line %d\n", nLine );
		return( -1 );
	    }
	    continue;
	}
	if ( sscanf( zLineBuf, "RControl = %7[0-9a-fx] \n", zBuf ) == 1 ) {
	    psKeymap->m_nRControl = parse_number( zBuf );
	    if ( psKeymap->m_nRControl == -1 ) {
		fprintf( stderr, "Syntax error at line %d\n", nLine );
		return( -1 );
	    }
	    continue;
	}
	if ( sscanf( zLineBuf, "LOption = %7[0-9a-fx] \n", zBuf ) == 1 ) {
	    psKeymap->m_nLOption = parse_number( zBuf );
	    if ( psKeymap->m_nLOption == -1 ) {
		fprintf( stderr, "Syntax error at line %d\n", nLine );
		return( -1 );
	    }
	    continue;
	}
	if ( sscanf( zLineBuf, "ROption = %7[0-9a-fx] \n", zBuf ) == 1 ) {
	    psKeymap->m_nROption = parse_number( zBuf );
	    if ( psKeymap->m_nROption == -1 ) {
		fprintf( stderr, "Syntax error at line %d\n", nLine );
		return( -1 );
	    }
	    continue;
	}
	if ( sscanf( zLineBuf, "Menu = %7[0-9a-fx] \n", zBuf ) == 1 ) {
	    psKeymap->m_nMenu = parse_number( zBuf );
	    if ( psKeymap->m_nMenu == -1 ) {
		fprintf( stderr, "Syntax error at line %d\n", nLine );
		return( -1 );
	    }
	    continue;
	}

	if ( strncmp( zLineBuf, "InitialLocks", 12 ) == 0 ) {
	    if ( sscanf( zLineBuf, "InitialLocks = %127[a-zA-Z] \n", zBuf ) == 1 ) {
		if ( strstr( zBuf, "CapsLock" ) != NULL ) psKeymap->m_nLockSetting |= KLOCK_CAPSLOCK;
		if ( strstr( zBuf, "NumLock" ) != NULL ) psKeymap->m_nLockSetting |= KLOCK_NUMLOCK;
		if ( strstr( zBuf, "ScrollLock" ) != NULL ) psKeymap->m_nLockSetting |= KLOCK_SCROLLLOCK;
	    }
	    continue;
	}
    
	if ( strncmp( zLineBuf, "LockSettings", 12 ) == 0 ) {
	    if ( sscanf( zLineBuf, "LockSettings = %127[a-zA-Z] \n", zBuf ) == 1 ) {
		if ( strstr( zBuf, "CapsLock" ) != NULL )   psKeymap->m_nLockSetting |= KLOCK_CAPSLOCK;
		if ( strstr( zBuf, "NumLock" ) != NULL )    psKeymap->m_nLockSetting |= KLOCK_NUMLOCK;
		if ( strstr( zBuf, "ScrollLock" ) != NULL ) psKeymap->m_nLockSetting |= KLOCK_SCROLLLOCK;
		
	    }
	    continue;
	}

	char pzKey[8];

	if ( strncmp( zLineBuf, "DeadKey", 7 ) == 0 ) {
	    char zSource[17], zResult[17];
	    if ( sscanf( zLineBuf, "DeadKey %16[^ ] %7[0-9a-fx] %16[^ ] = %16[^ ]\n", zBuf, pzKey, zSource, zResult ) == 4 ) {
	    	int i;
	    	bool found = false;
	    	
	    	psKeymap->m_sDeadKey[ nDeadKeyIndex ].m_nRawKey = parse_number( pzKey );
	    	for( i = 0; qualstrings[i]; i++ ) {
	    		if( strcmp( qualstrings[i], zBuf ) == 0 ) {
	    			found = true;
	    			break;
	    		}
	    	}
	    	if( !found ) {
			fprintf( stderr, "Error: Unknown constant '%s' at line %d\n", zBuf, nLine );
			fprintf( stderr, "%s\n", zLineBuf );
		}
	    	psKeymap->m_sDeadKey[ nDeadKeyIndex ].m_nQualifier = i ;
	    	psKeymap->m_sDeadKey[ nDeadKeyIndex ].m_nKey = parse_number( zSource );
	    	psKeymap->m_sDeadKey[ nDeadKeyIndex ].m_nValue = parse_number( zResult );
		nDeadKeyIndex++;
	    } else {
		fprintf( stderr, "Error: Syntax error at line %d\n", nLine );
		fprintf( stderr, "%s\n", zLineBuf );
	    }
	    continue;
	}
   
	int nCnt;
	char azBuf[9][20];
	nCnt = sscanf( zLineBuf,
		       "Key %7[0-9a-fx] = %16[^ ] %16[^ ] %16[^ ] %16[^ ] %16[^ ] %16[^ ] %16[^ ] %16[^ ] %16[^ ] \n",
		       pzKey, azBuf[0], azBuf[1], azBuf[2], azBuf[3], azBuf[4], azBuf[5], azBuf[6], azBuf[7], azBuf[8] );
	if ( nCnt  != 10 ) {
	    fprintf( stderr, "Error: Syntax error at line %d, only %d valid entries found\n", nLine, nCnt );
	    fprintf( stderr, "%s\n", zLineBuf );
	    return( -1 );
	}
	int nKey = parse_number( pzKey );
	if ( nKey < 0 ) {
	    fprintf( stderr, "Invalid key number at line %d\n", nLine );
	    return( -1 );
	}
	if ( nKey >= 128 ) {
	    fprintf( stderr, "Key number out of range at line %d\n", nLine );
	    return( -1 );
	}
	for ( int i = 0 ; i < 9 ; ++i ) {
	    int nValue = parse_number( azBuf[i] );
	    if ( nValue == -1 ) {
		fprintf( stderr, "\nError: Invalid field %d at line %d\n", i, nLine );
		return( -1 );
	    }
	    psKeymap->m_anMap[nKey][i] = nValue;
	}
    }
    return( 0 );
}

void write_key( FILE* hFile, int32 key )
{
    if ( key > 0x20 && key < 0x7f ) {
	fprintf( hFile, " '%c'       ", (int)key );
    } else {
	fprintf( hFile, " 0x%-8lx", key );
    }
}

int write_ascii_keymap( FILE* hFile, keymap* psKeymap )
{
    fprintf( hFile, "Version    = %d\n", CURRENT_KEYMAP_VERSION );
    
    fprintf( hFile, "\n" );
    
    fprintf( hFile, "CapsLock   = 0x%02x\n", psKeymap->m_nCapsLock );
    fprintf( hFile, "ScrollLock = 0x%02x\n", psKeymap->m_nScrollLock );
    fprintf( hFile, "NumLock    = 0x%02x\n", psKeymap->m_nNumLock );
    fprintf( hFile, "LShift     = 0x%02x\n", psKeymap->m_nLShift );
    fprintf( hFile, "RShift     = 0x%02x\n", psKeymap->m_nRShift );
    fprintf( hFile, "LCommand   = 0x%02x\n", psKeymap->m_nLCommand );
    fprintf( hFile, "RCommand   = 0x%02x\n", psKeymap->m_nRCommand );
    fprintf( hFile, "LControl   = 0x%02x\n", psKeymap->m_nLControl );
    fprintf( hFile, "RControl   = 0x%02x\n", psKeymap->m_nRControl );
    fprintf( hFile, "LOption    = 0x%02x\n", psKeymap->m_nLOption );
    fprintf( hFile, "ROption    = 0x%02x\n", psKeymap->m_nROption );
    fprintf( hFile, "Menu       = 0x%02x\n", psKeymap->m_nMenu );

    fprintf( hFile, "\n" );
    
    fprintf( hFile, "#InitialLocks = CapsLock NumLock ScrollLock\n" );
    fprintf( hFile, "InitialLocks =" );
    if ( psKeymap->m_nLockSetting & KLOCK_CAPSLOCK ) {
	fprintf( hFile, " CapsLock" );
    }
    if ( psKeymap->m_nLockSetting & KLOCK_NUMLOCK ) {
	fprintf( hFile, " NumLock" );
    }
    if ( psKeymap->m_nLockSetting & KLOCK_SCROLLLOCK ) {
	fprintf( hFile, " ScrollLock" );
    }
    fprintf( hFile, "\n\n" );
    
    fprintf( hFile, "# Qualifiers:\n" );
    fprintf( hFile, "#   n = Normal\n" );
    fprintf( hFile, "#   s = Shift\n" );
    fprintf( hFile, "#   c = Control\n" );
    fprintf( hFile, "#   C = Caps lock\n" );
    fprintf( hFile, "#   o = Option\n" );
    fprintf( hFile, "#\n" );
    fprintf( hFile, "#   Key     n          s          c          o          os         C          Cs         Co        Cos\n" );
    
    fprintf( hFile, "\n" );
    for ( int i = 0 ; i < 128 ; ++i ) {
	fprintf( hFile, "Key 0x%02x =", i );
	for ( int j = 0 ; j < 9 ; ++j ) {
	    if ( psKeymap->m_anMap[i][j] == DEADKEY_ID ) {
	    	fprintf( hFile, " DEADKEY   " );
	    } else if (  psKeymap->m_anMap[i][j] > 0x20 && psKeymap->m_anMap[i][j] < 0x7f ) {
		fprintf( hFile, " '%c'       ", (int)psKeymap->m_anMap[i][j] );
	    } else {
		fprintf( hFile, " 0x%-8lx", psKeymap->m_anMap[i][j] );
	    }
	}
	fprintf( hFile, "\n" );
    }

    fprintf( hFile, "\n# Dead keys:\n" );
    fprintf( hFile, "# DeadKey <qualifiers> <rawkey> <second_key> = <result>\n" );   
    fprintf( hFile, "# qualifier is one of the qualifiers listed above (n, s, c, o, os)\n" );
    fprintf( hFile, "# rawkey is the raw key code of the dead key\n" );
    fprintf( hFile, "# second_key is the translated key code (unicode) of the key following the dead key\n" );
    fprintf( hFile, "# result is the character to display when the key sequence is encountered\n" );
    fprintf( hFile, "#\n#       qual raw   second     = result\n\n");

    for ( int i = 0 ; i < psKeymap->m_nNumDeadKeys ; ++i ) {   	
	fprintf( hFile, "DeadKey %4s 0x%02x ", qualstrings[psKeymap->m_sDeadKey[i].m_nQualifier % 9], psKeymap->m_sDeadKey[i].m_nRawKey );
	write_key( hFile, psKeymap->m_sDeadKey[i].m_nKey );
	fprintf( hFile, "=" );
	write_key( hFile, psKeymap->m_sDeadKey[i].m_nValue );
	fprintf( hFile, "\n" );
    }
    
    return( 0 );
}


int ascii_to_bin( const char* pzDst, const char* pzSrc )
{
    FILE* hSrcFile;
    FILE* hDstFile;
    hSrcFile = fopen( pzSrc, "r" );
    if ( hSrcFile == NULL ) {
	fprintf( stderr, "Error: Failed to open source '%s': %s\n", pzSrc, strerror( errno ) );
	return( - 1 );
    }
    hDstFile = fopen( pzDst, "w" );
    if ( hDstFile == NULL ) {
	fprintf( stderr, "Error: Failed to open destination '%s': %s\n", pzSrc, strerror( errno ) );
	fclose( hSrcFile );
	return( - 1 );
    }

    uint32 nDeadKeys = count_dead_keys( hSrcFile );
    uint32 nSize = sizeof( keymap ) + sizeof( deadkey ) * nDeadKeys;
    
    printf( "DeadKey count: %d\n", nDeadKeys );

    keymap *psKeymap = (keymap *)malloc( nSize );
    int nError = load_ascii_keymap( hSrcFile, psKeymap );
    if ( nError >= 0 ) {
    	keymap_header	sHeader;
    	
    	sHeader.m_nMagic = KEYMAP_MAGIC;
    	sHeader.m_nVersion = CURRENT_KEYMAP_VERSION;
	sHeader.m_nSize = nSize;
	
	psKeymap->m_nNumDeadKeys = nDeadKeys;
	
	if ( fwrite( &sHeader, sizeof(sHeader), 1, hDstFile ) != 1 ) {
	    printf( "Error: failed to write '%s': %s\n", pzDst, strerror( errno ) );
	    nError = -1;
	} else if ( fwrite( psKeymap, nSize, 1, hDstFile ) != 1 ) {
	    printf( "Error: failed to write '%s': %s\n", pzDst, strerror( errno ) );
	    nError = -1;
	}
    }
    free( psKeymap );
    fclose( hDstFile );
    fclose( hSrcFile );
    return( nError );
}

int bin_to_ascii( const char* pzDst, const char* pzSrc )
{
    FILE* hSrcFile;
    FILE* hDstFile;
    hSrcFile = fopen( pzSrc, "r" );
    if ( hSrcFile == NULL ) {
	fprintf( stderr, "Error: Failed to open source '%s': %s\n", pzSrc, strerror( errno ) );
	return( - 1 );
    }
    hDstFile = fopen( pzDst, "w" );
    if ( hDstFile == NULL ) {
	fprintf( stderr, "Error: Failed to open destination '%s': %s\n", pzSrc, strerror( errno ) );
	fclose( hSrcFile );
	return( - 1 );
    }
    keymap *psKeymap = NULL;
    keymap_header	sHeader;

    int nError = 0;
    if ( fread( &sHeader, sizeof(sHeader), 1, hSrcFile ) != 1 ) {
	    printf( "Error: failed to read '%s': %s\n", pzSrc, strerror( errno ) );
	    nError = -1;
    } else {
    	if( sHeader.m_nVersion == 1 ) {
    		// For compatibility:
    		sHeader.m_nSize = sizeof( struct keymap ) - 4;
    		fseek( hSrcFile, 8, SEEK_SET );
	    	psKeymap = (keymap*)malloc( sHeader.m_nSize + 4 );
	    	if ( fread( psKeymap, sHeader.m_nSize, 1, hSrcFile ) != 1 ) {
		    printf( "Error: failed to write '%s': %s\n", pzSrc, strerror( errno ) );
		    nError = -1;
		}
		psKeymap->m_nNumDeadKeys = 0;
    	} else {
	    	psKeymap = (keymap*)malloc( sHeader.m_nSize );
	    	if ( fread( psKeymap, sHeader.m_nSize, 1, hSrcFile ) != 1 ) {
		    printf( "Error: failed to write '%s': %s\n", pzSrc, strerror( errno ) );
		    nError = -1;
		}
	}
    }
      
    if ( nError >= 0 ) {
        if( psKeymap->m_nNumDeadKeys ) {
		printf( "Dead keys present in keymap\n" );
	}

	nError = write_ascii_keymap( hDstFile, psKeymap );
	
    }
    if( psKeymap ) free( psKeymap );
    fclose( hDstFile );
    fclose( hSrcFile );
    return( nError );
}



int main( int argc, char** argv )
{
    if ( argc != 3 ) {
	fprintf( stderr, "Usage: %s srcfile dstfile\n", argv[0] );
	exit(1);
    }
    struct stat sStat;
    if ( stat( argv[2], &sStat ) == 0 ) {
	printf( "Destination %s exists. Do you want to overwrite? [yN]", argv[2] );
	if ( getc(stdin) != 'y' ) {
	    printf( "\nCanceled!\n" );
	    exit( 1 );
	}
	printf( "\n" );
    }
    FILE* hSrcFile = fopen( argv[1], "r" );
    if ( hSrcFile == NULL ) {
	fprintf( stderr, "Error: Failed to open source '%s': %s\n", argv[1], strerror( errno ) );
	exit( 1 );
    }
    uint32 anBuffer[2];
    if ( fread( anBuffer, sizeof(uint32), 2, hSrcFile ) != 2 ) {
	fprintf( stderr, "Error: failed to read '%s': %s\n", argv[1], strerror( errno ) );
	fclose( hSrcFile );
	exit(1);
    }
    fclose( hSrcFile );
    if ( anBuffer[0] == KEYMAP_MAGIC ) {
    	if ( anBuffer[1] == 1 ) {
    	    printf( "Source keymap version is 1 (Syllable 0.4.3 or older)\n" );
	    if ( bin_to_ascii( argv[2], argv[1] ) != 0 ) {
	        exit( 1 );
	    }
    	} else {
	    if ( anBuffer[1] != CURRENT_KEYMAP_VERSION ) {
		fprintf( stderr, "Error: Source keymap have unknown version %ld\n", anBuffer[1] );
		exit( 1 );
	    }
	    if ( bin_to_ascii( argv[2], argv[1] ) != 0 ) {
	        exit( 1 );
	    }
	}
    } else {
	if ( ascii_to_bin( argv[2], argv[1] ) != 0 ) {
	    exit( 1 );
	}
    }
    return( 0 );
}
