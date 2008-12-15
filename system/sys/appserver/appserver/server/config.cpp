
/*
 *  The AtheOS application server
 *  Copyright (C) 1999 - 2001 Kurt Skauen
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
#include <assert.h>
#include <ctype.h>
#include <malloc.h>
#include <string.h>

#include "config.h"
#include "server.h"
#include "desktop.h"
#include "keyboard.h"

using namespace os;

AppserverConfig *AppserverConfig::s_pcInstance = NULL;

AppserverConfig::AppserverConfig()
{
	s_pcInstance = this;

	m_cKeymapPath = "/system/keymaps/American";
	m_nKeyDelay = 300LL;
	m_nKeyRepeat = 30LL;
	m_nDoubleClickDelay = 500000LL;
	m_nMouseSpeed = 1.5f;
	m_nMouseAcceleration = 0.0f;
	m_bMouseSwapButtons = false;
	m_bPopupSelectedWindows = false;

	AddFontConfig( DEFAULT_FONT_REGULAR, os::font_properties( "Nimbus Sans L", "Regular", FPF_SYSTEM | FPF_SMOOTHED, 8.0f ) );
	AddFontConfig( DEFAULT_FONT_BOLD, os::font_properties( "Nimbus Sans L", "Bold", FPF_SYSTEM | FPF_SMOOTHED, 8.0f ) );
	AddFontConfig( DEFAULT_FONT_FIXED, os::font_properties( "Syllable-Console", "Regular", FPF_SYSTEM | FPF_MONOSPACED | FPF_SMOOTHED, 8.0f ) );
	AddFontConfig( DEFAULT_FONT_WINDOW, os::font_properties( "Nimbus Sans L", "Regular", FPF_SYSTEM | FPF_SMOOTHED, 8.0f ) );
	AddFontConfig( DEFAULT_FONT_TOOL_WINDOW, os::font_properties( "Nimbus Sans L", "Regular", FPF_SYSTEM | FPF_SMOOTHED, 7.0f ) );

	m_bDirty = false;
}

AppserverConfig::~AppserverConfig()
{
}

AppserverConfig *AppserverConfig::GetInstance()
{
	return ( s_pcInstance );
}

bool AppserverConfig::IsDirty() const
{
	return ( m_bDirty );
}

void AppserverConfig::SetConfig( const Message * pcConfig )
{
	std::string cPath;
	if( pcConfig->FindString( "keymap", &cPath ) == 0 )
	{
		SetKeymapPath( cPath );
	}
	if( pcConfig->FindString( "window_decorator", &cPath ) == 0 )
	{
		SetWindowDecoratorPath( cPath );
	}
	bool bPopup;

	if( pcConfig->FindBool( "popoup_sel_win", &bPopup ) == 0 )
	{
		SetPopoupSelectedWindows( bPopup );
	}
	bigtime_t nDelay;

	if( pcConfig->FindInt64( "doubleclick_delay", &nDelay ) == 0 )
	{
		SetDoubleClickTime( nDelay );
	}
	if( pcConfig->FindInt64( "key_delay", &nDelay ) == 0 )
	{
		SetKeyDelay( nDelay );
	}
	if( pcConfig->FindInt64( "key_repeat", &nDelay ) == 0 )
	{
		SetKeyRepeat( nDelay );
	}

	float nMouse;
	if( pcConfig->FindFloat( "mouse_speed", &nMouse ) == 0 )
	{
		SetMouseSpeed( nMouse );
	}
	if( pcConfig->FindFloat( "mouse_acceleration", &nMouse ) == 0 )
	{
		SetMouseAcceleration( nMouse );
	}
	
	bool bSwapButtons;
	if( pcConfig->FindBool( "mouse_swap_buttons", &bSwapButtons ) == 0 )
	{
		SetMouseSwapButtons( bSwapButtons );
	}

	Message cColorConfig;

	if( pcConfig->FindMessage( "color_config", &cColorConfig ) == 0 )
	{
		for( int i = 0; i < COL_COUNT; ++i )
		{
			Color32_s sColor;

			if( cColorConfig.FindColor32( "color_table", &sColor, i ) != 0 )
			{
				break;
			}
			__set_default_color( static_cast < default_color_t > ( i ), sColor );
		}
		m_bDirty = true;
	}
}

void AppserverConfig::GetConfig( Message * pcConfig )
{
	const char *pzKeymap = strrchr( m_cKeymapPath.c_str(), '/' );

	if( pzKeymap == NULL )
	{
		pzKeymap = m_cKeymapPath.c_str();
	}
	else
	{
		pzKeymap++;
	}
	pcConfig->AddString( "keymap", pzKeymap );
	pcConfig->AddString( "window_decorator", m_cWindowDecoratorPath );
	pcConfig->AddBool( "popoup_sel_win", m_bPopupSelectedWindows );
	pcConfig->AddInt64( "doubleclick_delay", m_nDoubleClickDelay );
	pcConfig->AddInt64( "key_delay", m_nKeyDelay );
	pcConfig->AddInt64( "key_repeat", m_nKeyRepeat );
	pcConfig->AddFloat( "mouse_speed", m_nMouseSpeed );
	pcConfig->AddFloat( "mouse_acceleration", m_nMouseAcceleration );
	pcConfig->AddBool(  "mouse_swap_buttons", m_bMouseSwapButtons );

	Message cColorConfig;

	cColorConfig.AddFloat( "shine_tint", 0.9f );
	cColorConfig.AddFloat( "shadow_tint", 0.9f );
	for( int i = 0; i < COL_COUNT; ++i )
	{
		cColorConfig.AddColor32( "color_table", get_default_color( static_cast < default_color_t > ( i ) ) );
	}
	pcConfig->AddMessage( "color_config", &cColorConfig );

}


int AppserverConfig::SetKeymapPath( const std::string & cPath )
{
	if( SetKeymap( cPath ) )
	{
		m_cKeymapPath = cPath;
		m_bDirty = true;
		return 0;
	}
	return -1;

/*    int nError;

    FILE* hFile = fopen( cPath.c_str(), "r" );

    if ( hFile == NULL ) {
	return( -1 );
    }
    
    g_cKeymapGate.Lock();
    
    g_psKeymap = load_keymap( hFile );
    if ( !g_psKeymap ) {
	dbprintf( "Error: AppserverConfig::SetKeymapPath() failed to load keymap\n" );
	g_psKeymap = g_sDefaultKeyMap;
    } else {
	m_cKeymapPath = cPath;
	m_bDirty = true;
    }

    g_cKeymapGate.Unlock();

    fclose( hFile );
    return( nError );*/
}

std::string AppserverConfig::GetKeymapPath() const
{
	return ( m_cKeymapPath );
}

int AppserverConfig::SetWindowDecoratorPath( const std::string & cPath )
{
	if( m_cWindowDecoratorPath == cPath )
	{
		return ( 0 );
	}

	if( AppServer::GetInstance()->LoadWindowDecorator( cPath ) == 0 )
	{
		m_cWindowDecoratorPath = cPath;
		m_bDirty = true;
		return ( 0 );
	}
	else
	{
		return ( 1 );
	}
}

std::string AppserverConfig::GetWindowDecoratorPath() const
{
	return ( m_cWindowDecoratorPath );
}

void AppserverConfig::SetDesktopScreenMode( int nDesktop, const screen_mode & sMode )
{
	g_cLayerGate.Close();
	set_desktop_screenmode( nDesktop, sMode );
	g_cLayerGate.Open();
	m_bDirty = true;
}

void AppserverConfig::SetDefaultColor( default_color_t eIndex, const Color32_s & sColor )
{
	__set_default_color( eIndex, sColor );
	m_bDirty = true;
}

void AppserverConfig::SetDoubleClickTime( bigtime_t nDelay )
{
	if( nDelay != m_nDoubleClickDelay )
	{
		m_nDoubleClickDelay = nDelay;
		m_bDirty = true;
	}
}

bigtime_t AppserverConfig::GetDoubleClickTime() const
{
	return ( m_nDoubleClickDelay );
}

void AppserverConfig::SetKeyDelay( bigtime_t nDelay )
{
	if( nDelay != m_nKeyDelay )
	{
		m_nKeyDelay = nDelay;
		m_bDirty = true;
	}
}
void AppserverConfig::SetKeyRepeat( bigtime_t nDelay )
{
	if( nDelay != m_nKeyRepeat )
	{
		m_nKeyRepeat = nDelay;
		m_bDirty = true;
	}
}

bigtime_t AppserverConfig::GetKeyDelay() const
{
	return ( m_nKeyDelay );
}

bigtime_t AppserverConfig::GetKeyRepeat() const
{
	return ( m_nKeyRepeat );
}

void AppserverConfig::SetMouseSpeed( float nSpeed )
{
	if( nSpeed != m_nMouseSpeed )
	{
		m_nMouseSpeed = nSpeed;
		m_bDirty = true;
	}
}

float AppserverConfig::GetMouseSpeed() const
{
	return (m_nMouseSpeed);
}

void AppserverConfig::SetMouseAcceleration( float nAccel )
{
	if( nAccel != m_nMouseAcceleration )
	{
		m_nMouseAcceleration = nAccel;
		m_bDirty = true;
	}
}

float AppserverConfig::GetMouseAcceleration() const
{
	return (m_nMouseAcceleration);
}

void AppserverConfig::SetMouseSwapButtons( bool bSwap )
{
	if( bSwap != m_bMouseSwapButtons );
	{
		m_bMouseSwapButtons = bSwap;
		m_bDirty = true;
	}
}

bool AppserverConfig::GetMouseSwapButtons() const
{
	return (m_bMouseSwapButtons);
}

void AppserverConfig::SetPopoupSelectedWindows( bool bPopup )
{
	if( bPopup != m_bPopupSelectedWindows )
	{
		m_bPopupSelectedWindows = bPopup;
		m_bDirty = true;
	}
}

bool AppserverConfig::GetPopoupSelectedWindows() const
{
	return ( m_bPopupSelectedWindows );
}

void AppserverConfig::GetDefaultFontNames( os::Message * pcArchive )
{
	std::map <std::string, os::font_properties >::iterator i;

	for( i = m_cFontSettings.begin(); i != m_cFontSettings.end(  ); ++i )
	{
		pcArchive->AddString( "config_names", ( *i ).first );
	}
}

const os::font_properties * AppserverConfig::GetFontConfig( const std::string & cName )
{
	std::map <std::string, os::font_properties >::iterator i = m_cFontSettings.find( cName );

	if( i == m_cFontSettings.end() )
	{
		return ( NULL );
	}
	else
	{
		return ( &( *i ).second );
	}
}

status_t AppserverConfig::SetFontConfig( const std::string & cName, const font_properties & cProps )
{
	std::map <std::string, os::font_properties >::iterator i = m_cFontSettings.find( cName );

	if( i == m_cFontSettings.end() )
	{
		return ( -ENOENT );
	}
	else
	{
		( *i ).second = cProps;
		m_bDirty = true;
		return ( 0 );
	}
}
status_t AppserverConfig::AddFontConfig( const std::string & cName, const font_properties & cProps )
{
	std::map <std::string, os::font_properties >::iterator i = m_cFontSettings.find( cName );

	if( i == m_cFontSettings.end() )
	{
		m_cFontSettings.insert( std::pair < std::string, os::font_properties > ( cName, cProps ) );
		return ( 0 );
	}
	else
	{
		( *i ).second = cProps;
		m_bDirty = true;
		return ( 0 );
	}
}
status_t AppserverConfig::DeleteFontConfig( const std::string & cName )
{
	std::map <std::string, os::font_properties >::iterator i = m_cFontSettings.find( cName );

	if( i == m_cFontSettings.end() )
	{
		return ( -ENOENT );
	}
	else
	{
		m_cFontSettings.erase( i );
		m_bDirty = true;
		return ( 0 );
	}

}

static bool match_name( const char *pzName, const char *pzBuffer )
{
	return ( strncmp( pzBuffer, pzName, strlen( pzName ) ) == 0 );
}

int AppserverConfig::LoadConfig( FILE *hFile, bool bActivateConfig )
{
	int nLine = 0;

	for( ;; )
	{
		char zLineBuf[4096];

		nLine++;

		if( fgets( zLineBuf, 4096, hFile ) == NULL )
		{
			break;
		}
		if( zLineBuf[0] == '#' )
		{
			continue;
		}
		bool bEmpty = true;

		for( int i = 0; zLineBuf[i] != 0; ++i )
		{
			if( isspace( zLineBuf[i] ) == false )
			{
				bEmpty = false;
				break;
			}
		}
		if( bEmpty )
		{
			continue;
		}
		if( match_name( "Desktop", zLineBuf ) )
		{
			int nDesktop, nWidth, nHeight;
			screen_mode sMode;
			char zBackDropPath[1024];
			char zColorSpace[16];
			bool bValid = false;

			if( sscanf( zLineBuf, "Desktop %d = %d %d %f %f %f %f %f %15s %1023s\n", &nDesktop, &nWidth, &nHeight, &sMode.m_vHPos, &sMode.m_vVPos, &sMode.m_vHSize, &sMode.m_vVSize, &sMode.m_vRefreshRate, zColorSpace, zBackDropPath ) == 10 )
			{
				bValid = true;
			}
			else if( sscanf( zLineBuf, "Desktop %d = %d %d %15s %1023s\n", &nDesktop, &nWidth, &nHeight, zColorSpace, zBackDropPath ) == 5 )
			{
				sMode.m_vHPos = 80.0f;
				sMode.m_vVPos = 50.0f;
				sMode.m_vHSize = 70.0f;
				sMode.m_vVSize = 80.0f;
				sMode.m_vRefreshRate = 60.0f;

				bValid = true;
			}
			if( bValid )
			{
				sMode.m_nWidth = nWidth;
				sMode.m_nHeight = nHeight;

				if( strcmp( zColorSpace, "RGB32" ) == 0 )
				{
					sMode.m_eColorSpace = CS_RGB32;
				}
				else if( strcmp( zColorSpace, "RGB24" ) == 0 )
				{
					sMode.m_eColorSpace = CS_RGB24;
				}
				else if( strcmp( zColorSpace, "RGB16" ) == 0 )
				{
					sMode.m_eColorSpace = CS_RGB16;
				}
				else if( strcmp( zColorSpace, "RGB15" ) == 0 )
				{
					sMode.m_eColorSpace = CS_RGB15;
				}
				else if( strcmp( zColorSpace, "CMAP8" ) == 0 )
				{
					sMode.m_eColorSpace = CS_CMAP8;
				}
				else if( strcmp( zColorSpace, "GRAY8" ) == 0 )
				{
					sMode.m_eColorSpace = CS_GRAY8;
				}
				else if( strcmp( zColorSpace, "GRAY1" ) == 0 )
				{
					sMode.m_eColorSpace = CS_GRAY1;
				}
				else if( strcmp( zColorSpace, "YUV422" ) == 0 )
				{
					sMode.m_eColorSpace = CS_YUV422;
				}
				else if( strcmp( zColorSpace, "YUV411" ) == 0 )
				{
					sMode.m_eColorSpace = CS_YUV411;
				}
				else if( strcmp( zColorSpace, "YUV420" ) == 0 )
				{
					sMode.m_eColorSpace = CS_YUV420;
				}
				else if( strcmp( zColorSpace, "YUV444" ) == 0 )
				{
					sMode.m_eColorSpace = CS_YUV444;
				}
				else if( strcmp( zColorSpace, "YUV9" ) == 0 )
				{
					sMode.m_eColorSpace = CS_YUV9;
				}
				else if( strcmp( zColorSpace, "YUV12" ) == 0 )
				{
					sMode.m_eColorSpace = CS_YUV12;
				}
				else
				{
					dbprintf( "Invalid color-space at line %d in appserver config file\n", nLine );
					sMode.m_eColorSpace = CS_RGB16;
				}
				set_desktop_config( nDesktop, sMode, zBackDropPath );
			}
			else
			{
				dbprintf( "Error: Syntax error in appserver config file at line %d\n", nLine );
			}
		}
		if( match_name( "DefaultFont:", zLineBuf ) )
		{
			char zName[1024];
			char zFamily[512];
			char zStyle[512];
			uint32 nFlags;
			float vSize;
			float vShare;
			float vRotation;

			if( sscanf( zLineBuf, "DefaultFont: \"%1023[^\"]\" \"%511[^\"]\" \"%511[^\"]\" %x %f %f %f\n", zName, zFamily, zStyle, &nFlags, &vSize, &vShare, &vRotation ) == 7 )
			{
				AddFontConfig( zName, os::font_properties( zFamily, zStyle, nFlags, vSize, vShare, vRotation ) );
			}
			else
			{
				dbprintf( "Error: Syntax error in appserver config file at line %d\n", nLine );
			}
		}
		if( match_name( "WndDecorator", zLineBuf ) )
		{
			char zPath[1024];

			if( sscanf( zLineBuf, "WndDecorator = %1023s\n", zPath ) == 1 )
			{
				if( bActivateConfig )
				{
					SetWindowDecoratorPath( zPath );
				}
				else
				{
					m_cWindowDecoratorPath = zPath;
				}
			}
			else
			{
				dbprintf( "Error: Syntax error in appserver config file at line %d\n", nLine );
			}
		}
		if( match_name( "Keymap", zLineBuf ) )
		{
			char zPath[1024];

			if( sscanf( zLineBuf, "Keymap = %1023s\n", zPath ) == 1 )
			{
				if( bActivateConfig )
				{
					SetKeymapPath( zPath );
				}
				else
				{
					m_cKeymapPath = zPath;
				}
			}
			else
			{
				dbprintf( "Error: Syntax error in appserver config file at line %d\n", nLine );
			}
		}
		if( match_name( "PopupSelWin", zLineBuf ) )
		{
			char zValue[64];

			if( sscanf( zLineBuf, "PopupSelWin = %63s\n", zValue ) == 1 )
			{
				m_bPopupSelectedWindows = strcmp( zValue, "true" ) == 0;
			}
			else
			{
				dbprintf( "Error: Syntax error in appserver config file at line %d\n", nLine );
			}
		}

		if( match_name( "DoubleClickDelay", zLineBuf ) )
		{
			bigtime_t nValue;

			if( sscanf( zLineBuf, "DoubleClickDelay = %Ld\n", &nValue ) == 1 )
			{
				m_nDoubleClickDelay = nValue;
			}
			else
			{
				dbprintf( "Error: Syntax error in appserver config file at line %d\n", nLine );
			}
		}
		if( match_name( "KeyDelay", zLineBuf ) )
		{
			bigtime_t nValue;

			if( sscanf( zLineBuf, "KeyDelay = %Ld\n", &nValue ) == 1 )
			{
				m_nKeyDelay = nValue;
			}
			else
			{
				dbprintf( "Error: Syntax error in appserver config file at line %d\n", nLine );
			}
		}
		if( match_name( "KeyRepeat", zLineBuf ) )
		{
			bigtime_t nValue;

			if( sscanf( zLineBuf, "KeyRepeat = %Ld\n", &nValue ) == 1 )
			{
				m_nKeyRepeat = nValue;
			}
			else
			{
				dbprintf( "Error: Syntax error in appserver config file at line %d\n", nLine );
			}
		}
		if( match_name( "MouseSpeed", zLineBuf ) )
		{
			float nValue;

			if( sscanf( zLineBuf, "MouseSpeed = %f\n", &nValue ) == 1 )
			{
				m_nMouseSpeed = nValue;
			}
			else
			{
				dbprintf( "Error: Syntax error in appserver config file at line %d\n", nLine );
			}
		}
		if( match_name( "MouseAcceleration", zLineBuf ) )
		{
			float nValue;

			if( sscanf( zLineBuf, "MouseAcceleration = %f\n", &nValue ) == 1 )
			{
				m_nMouseAcceleration = nValue;
			}
			else
			{
				dbprintf( "Error: Syntax error in appserver config file at line %d\n", nLine );
			}
		}
		if( match_name( "MouseSwapButtons", zLineBuf ) )
		{
			char zValue[64];

			if( sscanf( zLineBuf, "MouseSwapButtons = %63s\n", zValue ) == 1 )
			{
				m_bMouseSwapButtons = strcmp( zValue, "true" ) == 0;
			}
			else
			{
				dbprintf( "Error: Syntax error in appserver config file at line %d\n", nLine );
			}

		}
		if( match_name( "Color", zLineBuf ) )
		{
			int i, r, g, b, a;

			if( sscanf( zLineBuf, "Color %d = %d %d %d %d\n", &i, &r, &g, &b, &a ) == 5 )
			{
				__set_default_color( default_color_t( i ), Color32_s( r, g, b, a ) );
			}
			else
			{
				dbprintf( "Error: Syntax error in appserver config file at line %d\n", nLine );
			}
		}

	}
	m_bDirty = false;
	return ( 0 );
}



int AppserverConfig::SaveConfig()
{
	FILE *hFile = fopen( "/system/config/appserver.new", "w" );

	for( int i = 0; i < 32; ++i )
	{
		screen_mode sMode;

		std::string cBackdropPath;
		const char *pzColorSpace;

		get_desktop_config( &i, &sMode, &cBackdropPath );

		switch ( sMode.m_eColorSpace )
		{
		case CS_RGB32:
			pzColorSpace = "RGB32";
			break;
		case CS_RGB24:
			pzColorSpace = "RGB24";
			break;
		case CS_RGB16:
			pzColorSpace = "RGB16";
			break;
		case CS_RGB15:
			pzColorSpace = "RGB15";
			break;
		case CS_CMAP8:
			pzColorSpace = "CMAP8";
			break;
		case CS_GRAY8:
			pzColorSpace = "GRAY8";
			break;
		case CS_GRAY1:
			pzColorSpace = "GRAY1";
			break;
		case CS_YUV422:
			pzColorSpace = "YUV422";
			break;
		case CS_YUV411:
			pzColorSpace = "YUV411";
			break;
		case CS_YUV420:
			pzColorSpace = "YUV420";
			break;
		case CS_YUV444:
			pzColorSpace = "YUV444";
			break;
		case CS_YUV9:
			pzColorSpace = "YUV9";
			break;
		case CS_YUV12:
			pzColorSpace = "YUV12";
			break;
		default:
			pzColorSpace = "INV";
			break;
		}

		fprintf( hFile, "Desktop %02d = %4d %4d %.2f %.2f %.2f %.2f %.2f %6s \"%s\"\n", i, sMode.m_nWidth, sMode.m_nHeight, sMode.m_vHPos, sMode.m_vVPos, sMode.m_vHSize, sMode.m_vVSize, sMode.m_vRefreshRate, pzColorSpace, cBackdropPath.c_str() );
	}

	fprintf( hFile, "\n" );

	std::map <std::string, os::font_properties >::iterator cIterator;

	for( cIterator = m_cFontSettings.begin(); cIterator != m_cFontSettings.end(  ); ++cIterator )
	{
		fprintf( hFile, "DefaultFont: \"%s\" \"%s\" \"%s\" %08lx %.2f %.2f %.2f\n", ( *cIterator ).first.c_str(), ( *cIterator ).second.m_cFamily.c_str(  ), ( *cIterator ).second.m_cStyle.c_str(  ), (long unsigned int)( *cIterator ).second.m_nFlags, ( *cIterator ).second.m_vSize, ( *cIterator ).second.m_vShear, ( *cIterator ).second.m_vRotation );
	}


	fprintf( hFile, "\n" );

	fprintf( hFile, "WndDecorator     = %s\n", m_cWindowDecoratorPath.c_str() );
	fprintf( hFile, "Keymap           = %s\n", m_cKeymapPath.c_str() );
	fprintf( hFile, "PopupSelWin      = %s\n", ( m_bPopupSelectedWindows ) ? "true" : "false" );
	fprintf( hFile, "DoubleClickDelay = %Ld\n", m_nDoubleClickDelay );
	fprintf( hFile, "KeyDelay         = %Ld\n", m_nKeyDelay );
	fprintf( hFile, "KeyRepeat        = %Ld\n", m_nKeyRepeat );
	fprintf( hFile, "MouseSpeed         = %f\n", m_nMouseSpeed );
	fprintf( hFile, "MouseAcceleration  = %f\n", m_nMouseAcceleration );
	fprintf( hFile, "MouseSwapButtons   = %s\n", ( m_bMouseSwapButtons ) ? "true" : "false" );

	fprintf( hFile, "\n" );

	for( int i = 0; i < COL_COUNT; ++i )
	{
		Color32_s sColor = get_default_color( default_color_t( i ) );

		fprintf( hFile, "Color %02d = %03d %03d %03d %03d\n", i, sColor.red, sColor.green, sColor.blue, sColor.alpha );
	}

	fclose( hFile );

	rename( "/system/config/appserver.new", "/system/config/appserver" );
	m_bDirty = false;
	return ( 0 );
}
