/*
 *  The AtheOS application server
 *  Copyright (C) 1999 - 2000  Kurt Skauen
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

#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

#include <atheos/types.h>
#include <atheos/kernel.h>

#include <appserver/keymap.h>

#include <macros.h>

#include <gui/guidefines.h>

#include "ddriver.h"
#include "server.h"
#include "swindow.h"
#include "keyboard.h"
#include "config.h"

static uint32 g_nQual = 0;

/*
 *     Raw key numbering for 101 keyboard:
 *                                                                                       [sys]       [brk]
 *                                                                                        0x7e        0x7f
 *[esc]       [ f1] [ f2] [ f3] [ f4] [ f5] [ f6] [ f7] [ f8] [ f9] [f10] [f11] [f12]    [prn] [scr] [pau]
 * 0x01        0x02  0x03  0x04  0x05  0x06  0x07  0x08  0x09  0x0a  0x0b  0x0c  0x0d     0x0e  0x0f  0x10   	   KEYPAD
 *
 *[ ` ] [ 1 ] [ 2 ] [ 3 ] [ 4 ] [ 5 ] [ 6 ] [ 7 ] [ 8 ] [ 9 ] [ 0 ] [ - ] [ = ] [bck]    [ins] [hme] [pup]  [num] [ / ] [ * ] [ - ]
 * 0x11  0x12  0x13  0x14  0x15  0x16  0x17  0x18  0x19  0x1a  0x1b  0x1c  0x1d  0x1e     0x1f  0x20  0x21   0x22  0x23  0x24  0x25
 *
 *[tab] [ q ] [ w ] [ e ] [ r ] [ t ] [ y ] [ u ] [ i ] [ o ] [ p ] [ [ ] [ ] ] [ \ ]    [del] [end] [pdn]  [ 7 ] [ 8 ] [ 9 ] [ + ]
 * 0x26  0x27  0x28  0x29  0x2a  0x2b  0x2c  0x2d  0x2e  0x2f  0x30  0x31  0x32  0x33     0x34  0x35  0x36   0x37  0x38  0x39  0x3a
 *
 *[cap] [ a ] [ s ] [ d ] [ f ] [ g ] [ h ] [ j ] [ k ] [ l ] [ ; ] [ ' ] [  enter  ]                       [ 4 ] [ 5 ] [ 6 ]
 * 0x3b  0x3c  0x3d  0x3e  0x3f  0x40  0x41  0x42  0x43  0x44  0x45  0x46     0x47                           0x48  0x49  0x4a
 *
 *[shift]     [ z ] [ x ] [ c ] [ v ] [ b ] [ n ] [ m ] [ , ] [ . ] [ / ]     [shift]         [ up]         [ 1 ] [ 2 ] [ 3 ] [ent]
 * 0x4b       0x4c  0x4d  0x4e  0x4f  0x50  0x51  0x52  0x53  0x54  0x55       0x56            0x57         0x58  0x59  0x5a  0x5b
 *
 *[ctr]    {opt}      [cmd]      [  space  ]      [cmd]    {opt}    {menu}    [ctr]     [lft] [dwn] [rgt]   [ 0 ] [ . ]
 * 0x5c     0x66       0x5d          0x5e          0x5f     0x67     0x68      0x60      0x61  0x62  0x63    0x64  0x65
 */
 

// Legend:
//   n = Normal
//   s = Shift
//   c = Control
//   C = CapsLock
//   o = Option

// Key n        s        c        o        os       C        Cs       Co       Cos

keymap g_sKeymap;

void SetQualifiers( int nKeyCode )
{
    switch( nKeyCode )
    {
	case 0x4b:
	    g_nQual |= QUAL_LSHIFT;
	    break;
	case 0x4b | 0x80:
	    g_nQual &= ~QUAL_LSHIFT;
	    break;

	case 0x56:
	    g_nQual |= QUAL_RSHIFT;
	    break;
	case 0x56 | 0x80:
	    g_nQual &= ~QUAL_RSHIFT;
	    break;

	case 0x5c:
	    g_nQual |= QUAL_LCTRL;
	    break;
	case 0x5c | 0x80:
	    g_nQual &= ~QUAL_LCTRL;
	    break;

	case 0x60:
	    g_nQual |= QUAL_RCTRL;
	    break;
	case 0x60 | 0x80:
	    g_nQual &= ~QUAL_RCTRL;
	    break;

	case 0x5d:
	    g_nQual |= QUAL_LALT;
	    break;
	case 0x5d | 0x80:
	    g_nQual &= ~QUAL_LALT;
	    break;

	case 0x5f:
	    g_nQual |= QUAL_RALT;
	    break;
	case 0x5f | 0x80:
	    g_nQual &= ~QUAL_RALT;
	    break;
    }
}

uint32 GetQualifiers()
{
    return( g_nQual );
}

int convert_key_code( char* pzDst, int nRawKey, int nQual )
{
    int	nTable = 0;
    int	nQ = 0;

    if ( nRawKey < 0 || nRawKey >= 128 ) {
	dbprintf( "convert_key_code() invalid keycode: %d\n", nRawKey );
	return( -1 );
    }
    
    if ( nQual & QUAL_SHIFT ) nQ |= 0x01;
    if ( nQual & QUAL_CTRL )  nQ |= 0x02;
    if ( nQual & QUAL_ALT )   nQ |= 0x04;

    switch ( nQ )
    {
	case 0x0:  nTable = 0; break;
	case 0x1:  nTable = 1; break;
	case 0x2:  nTable = 2; break;
	case 0x3:  nTable = 6; break;
	case 0x4:  nTable = 3; break;
	case 0x5:  nTable = 4; break;
	case 0x6:  nTable = 7; break;
	case 0x7:  nTable = 8; break;
    }
//    int nLen = unicode_to_utf8( pzDst, g_sKeymap.m_anMap[nRawKey][nTable] );
    int nLen;
    uint nChar = g_sKeymap.m_anMap[nRawKey][nTable];
    if ( nChar < 0x100 ) {
	nLen = 1;
    } else if ( nChar < 0x10000 ) {
	nLen = 2;
    } else if ( nChar < 0x1000000 ) {
	nLen = 3;
    } else {
	nLen = 4;
    }
    for ( int i = 0 ; i < nLen ; ++i ) {
	pzDst[i] = (nChar >> ((nLen-i-1)*8)) & 0xff;
    }
    pzDst[nLen] = '\0';
    return( nLen );
}

void HandleKeyboard()
{
    uint	nKeyDownTime = 0;
    uint8	nLastKey     = 0;
    uint8	nKeyCode;

    signal( SIGINT, SIG_IGN );
    signal( SIGQUIT, SIG_IGN );
    signal( SIGTERM, SIG_IGN );
	
    int nKbdDev = open( "/dev/keybd", O_RDONLY );

    if ( nKbdDev < 0 ) {
	dbprintf( "Panic : Could not open keyboard device!\n" );
    }
  
    for (;;) {
	snooze( 10000 );
	if ( read( nKbdDev, &nKeyCode, 1 ) == 1 ) {
	    SetQualifiers( nKeyCode );
			
	    if ( 0 != nKeyCode ) {
		if ( nKeyCode & 0x80 ) {
		    nKeyDownTime = 0;
		    nLastKey	= 0;
		} else {
		    nKeyDownTime = clock() + (AppserverConfig::GetInstance()->GetKeyDelay() * CLOCKS_PER_SEC / 1000);
		    nLastKey     = nKeyCode;
		}
		AppServer::GetInstance()->SendKeyCode( nKeyCode, g_nQual );
	    }
	    continue;
	}

	if ( 0 != nKeyDownTime ) {
	    uint nTime = clock();
	    if ( nTime > nKeyDownTime ) {
		nKeyDownTime = nTime + (AppserverConfig::GetInstance()->GetKeyRepeat() * CLOCKS_PER_SEC / 1000);
		AppServer::GetInstance()->SendKeyCode( nLastKey, g_nQual | QUAL_REPEAT );
	    }
	}
    }
}

void InitKeyboard( void )
{
    thread_id hKbdThread;

    FILE* hFile = fopen( AppserverConfig::GetInstance()->GetKeymapPath().c_str(), "r" );

    if ( hFile != NULL ) {
	if ( load_keymap( hFile, &g_sKeymap ) != 0 ) {
	    dbprintf( "Error: failed to load keymap\n" );
	    g_sKeymap = g_sDefaultKeyMap;
	}
	fclose( hFile );
    } else {
	dbprintf( "Failed to open keymap file '%s'\n", AppserverConfig::GetInstance()->GetKeymapPath().c_str() );
	g_sKeymap = g_sDefaultKeyMap;
    }
    hKbdThread = spawn_thread( "keyb_thread", HandleKeyboard, 120, 0, NULL );
    resume_thread( hKbdThread );
}
