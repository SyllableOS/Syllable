
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

#include <posix/ioctl.h>

#include <sys/ioctl.h>

#include <appserver/keymap.h>

#include <macros.h>

#include <gui/guidefines.h>

#include "ddriver.h"
#include "server.h"
#include "swindow.h"
#include "keyboard.h"
#include "config.h"

using namespace os;

static uint32 g_nQual = 0;
static uint32 g_nLock = 0;
//static int g_nKbdDev = 0;

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

Gate g_cKeymapGate( "keymap_gate", false );
keymap *g_psKeymap;

void SetQualifiers( int nKeyCode )
{
	switch ( nKeyCode )
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

	case 0x3b:
		{
			g_nLock ^= KLOCK_CAPSLOCK;
			break;
		}

	case 0x22:
		{
			g_nLock ^= KLOCK_NUMLOCK;
			break;
		}

	case 0x0f:
		{
			g_nLock ^= KLOCK_SCROLLLOCK;	// ScrollLock doesn't do anything
			break;
		}
	}
}

uint32 GetQualifiers()
{
	return ( g_nQual );
}

int FindDeadKey( int nDeadKeyState, int nKey )
{
	uint8 nDeadRaw = ( nDeadKeyState >> 8 );
	uint8 nDeadQual = nDeadKeyState & 0xFF;

	switch ( nDeadQual )
	{
	case CS_CAPSL:
		nDeadQual = CM_NORMAL;
		break;
	case CS_SHFT_CAPSL:
		nDeadQual = CS_SHFT;
		break;
	case CS_CAPSL_OPT:
		nDeadQual = CS_OPT;
		break;
	case CS_SHFT_CAPSL_OPT:
		nDeadQual = CS_SHFT_OPT;
		break;
	}

	// Search the table for this particular key combination
	for( uint32 i = 0; i < g_psKeymap->m_nNumDeadKeys; i++ )
	{
		if( ( ( g_psKeymap->m_sDeadKey[i].m_nKey == nKey ) || ( g_psKeymap->m_sDeadKey[i].m_nKey == 0x00 ) ) && ( g_psKeymap->m_sDeadKey[i].m_nRawKey == nDeadRaw ) && ( g_psKeymap->m_sDeadKey[i].m_nQualifier == nDeadQual ) )
		{
			return g_psKeymap->m_sDeadKey[i].m_nValue;
		}
	}
	return 0x00;		// Truly a dead key: nothing about it exists in the keymap!
}

// Converts a UTF-8 character to a UTF-8 string
int CharToString( int nChar, char *pzString )
{
	int nLen = 0;

	if( nChar < 0x100 )
	{
		nLen = 1;
	}
	else if( nChar < 0x10000 )
	{
		nLen = 2;
	}
	else if( nChar < 0x1000000 )
	{
		nLen = 3;
	}
	else
	{
		nLen = 4;
	}
	for( int i = 0; i < nLen; ++i )
	{
		pzString[i] = ( nChar >> ( ( nLen - i - 1 ) * 8 ) ) & 0xff;
	}
	pzString[nLen] = '\0';

	return nLen;
}

int convert_key_code( char *pzDst, int nRawKey, int nQual, int *pnDeadKeyState )
{
	int nTable = 0;
	int nQ = 0;

	if( nRawKey < 0 || nRawKey >= 128 )
	{
		dbprintf( "convert_key_code() invalid keycode: %d\n", nRawKey );
		return ( -1 );
	}

	if( nQual & QUAL_SHIFT )
		nQ |= 0x01;
	if( nQual & QUAL_CTRL )
		nQ |= 0x02;
	if( nQual & QUAL_ALT )
		nQ |= 0x04;

	if( g_nLock & KLOCK_NUMLOCK )
		if( ( nRawKey >= 0x23 && nRawKey <= 0x25 ) || ( nRawKey >= 0x37 && nRawKey <= 0x3a ) || ( nRawKey >= 0x48 && nRawKey <= 0x4a ) || ( nRawKey >= 0x58 && nRawKey <= 0x5b ) || ( nRawKey >= 0x64 && nRawKey <= 0x65 ) )
			nQ |= 0x01;

	if( g_nLock & KLOCK_CAPSLOCK )
	{
		switch ( nQ )
		{
		case 0x0:
			nTable = CS_CAPSL;
			break;	// Caps
		case 0x1:
			nTable = CS_SHFT_CAPSL;
			break;	// Caps + Shift
		case 0x2:
			nTable = CS_CTRL;
			break;	// Ctrl (Not affected by the Capslock)
		case 0x4:
			nTable = CS_CAPSL_OPT;
			break;	// Caps + Alt
		case 0x5:
			nTable = CS_SHFT_CAPSL_OPT;
			break;	// Caps + Alt + Shift
		}

	}
	else
	{
		switch ( nQ )
		{
		case 0x0:
			nTable = CM_NORMAL;
			break;	// Normal
		case 0x1:
			nTable = CS_SHFT;
			break;	// Shift
		case 0x2:
			nTable = CS_CTRL;
			break;	// Ctrl
		case 0x4:
			nTable = CS_OPT;
			break;	// Alt
		case 0x5:
			nTable = CS_SHFT_OPT;
			break;	// Alt + Shift
		}
	}

	int nLen = 0;

	g_cKeymapGate.Lock();
	uint nChar = g_psKeymap->m_anMap[nRawKey][nTable];

	if( pnDeadKeyState )
	{			// Check for dead keys?
		if( nChar == DEADKEY_ID )
		{
			if( ( nChar = FindDeadKey( ( nRawKey << 8 ) | nTable, 0x00 ) ) )
			{
				*pnDeadKeyState = ( nRawKey << 8 ) | nTable;
				// Dead key found.
			}
		}
		else if( *pnDeadKeyState && nChar )
		{		// Last key was a dead one, but this one is not!
			uint nNewChar = FindDeadKey( *pnDeadKeyState, nChar );

			if( nNewChar )
			{
				nChar = nNewChar;
			}
			*pnDeadKeyState = 0;
		}
	}

	nLen = CharToString( nChar, pzDst );

	g_cKeymapGate.Unlock();

	return ( nLen );
}

void HandleKeyboard( bool bKeyEvent, int nKeyCode )
{
	static bigtime_t nKeyDownTime = 0;
	static uint8 nLastKey = 0;

	if( bKeyEvent )
	{
		SetQualifiers( nKeyCode );

		if( 0 != nKeyCode )
		{
			if( nKeyCode & 0x80 )
			{
				nKeyDownTime = 0;
				nLastKey = 0;
			}
			else
			{
				nKeyDownTime = get_system_time() + ( AppserverConfig::GetInstance(  )->GetKeyDelay(  ) * 1000 );
				nLastKey = nKeyCode;
			}
			AppServer::GetInstance()->SendKeyCode( nKeyCode, g_nQual );
		}

	}

	if( 0 != nKeyDownTime )
	{
		bigtime_t nTime = get_system_time();

		if( nTime > nKeyDownTime )
		{
			nKeyDownTime = nTime + ( AppserverConfig::GetInstance(  )->GetKeyRepeat(  ) * 1000 );
			AppServer::GetInstance()->SendKeyCode( nLastKey, g_nQual | QUAL_REPEAT );
		}
	}
}

bool SetKeymap( const std::string & cKeymapPath )
{
	FILE *hFile = fopen( cKeymapPath.c_str(), "r" );
	bool ok = false;

	g_cKeymapGate.Lock();

	if( g_psKeymap )
		free( g_psKeymap );
	g_psKeymap = NULL;

	if( hFile )
	{
		g_psKeymap = load_keymap( hFile );

		fclose( hFile );
	}

	if( g_psKeymap == NULL )
	{
		dbprintf( "Error: failed to load keymap\n" );
		g_psKeymap = ( keymap * ) malloc( sizeof( g_sDefaultKeyMap ) );
		if( g_psKeymap )
		{
			memcpy( g_psKeymap, &g_sDefaultKeyMap, sizeof( g_sDefaultKeyMap ) );
		}
		else
		{
			dbprintf( "Fatal: no mem for default keymap!\n" );
		}
	}
	else
	{
		ok = true;
	}

	g_cKeymapGate.Unlock();

	return ok;
}

void InitKeyboard( void )
{
	thread_id hKbdThread;

	SetKeymap( AppserverConfig::GetInstance()->GetKeymapPath(  ) );

	hKbdThread = spawn_thread( "keyb_thread", (void*)HandleKeyboard, 120, 0, NULL );
	resume_thread( hKbdThread );
}
