/*
    A Clock of the Syllable Dock
    Based on the Launcher Clock plugin
    Copyright (C) 2002 Andrew Kennan (bewilder@stresscow.org)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
                                                                                                                                                       
#include <vector>
#include <gui/button.h>
#include <gui/layoutview.h>
#include <gui/stringview.h>
#include <util/looper.h>
#include <util/resources.h>
#include <util/thread.h>
#include <util/settings.h>
#include <gui/requesters.h>
#include <gui/dropdownmenu.h>
#include <gui/menu.h>
#include <gui/checkbox.h>
#include <gui/desktop.h>
#include <gui/image.h>
#include <sys/types.h>
#include <unistd.h>
#include <atheos/kernel.h>
#include <atheos/time.h>
#include <posix/time.h>
#include <storage/file.h>
#include <time.h>    

#include "../../dock/dockplugin.h"                                                                                                                                              
                                                                                                                                                                                                        
using namespace os;
                                                                                                                                                                                                     
                                                                                                                                                                                                        
class DockClockUpdater;


class DockClock : public DockPlugin
{
	public:
		DockClock( os::Path cPath, os::Looper* pcDock );
		~DockClock();
		
		os::String GetIdentifier() ;
		Point GetPreferredSize( bool bLargest );
		os::Path GetPath() { return( m_cPath ); }
		
		virtual void AttachedToWindow();
		virtual void DetachedFromWindow();
		
        void DisplayTime( void );
        virtual void Paint( const Rect &cUpdateRect );
		
		virtual void MouseMove( const os::Point& cNewPos, int nCode, uint32 nButtons, os::Message* pcData );
		virtual void MouseUp( const os::Point & cPosition, uint32 nButton, os::Message * pcData );
		virtual void MouseDown( const os::Point& cPosition, uint32 nButtons );
        
	private:
		void LoadSettings();
		void SaveSettings();
		
		os::Path m_cPath;
		os::BitmapImage* m_pcIcon;
		os::Looper* m_pcDock;
		bool m_bCanDrag;
		bool m_bDragging;
		bool m_b24h;
		String m_zString;
		std::string m_zTimeFormat;
		DockClockUpdater* m_pcThread;
};


class DockClockUpdater : public Thread
{
public:
	DockClockUpdater( DockClock* pcClock ) : Thread( "dock_clock_update" ) 
	{
		m_pcClock = pcClock;
	}
	
	int32 Run()
	{
		while( 1 )
		{
			snooze( 1000000 );
			m_pcClock->DisplayTime();
		}
		return( 0 );
	}
private:
	DockClock* m_pcClock;
};


//*************************************************************************************

DockClock::DockClock( os::Path cPath, os::Looper* pcDock ) : DockPlugin()
{
	m_pcDock = pcDock;
	m_bCanDrag = m_bDragging = false;
	m_zString = "99:99:99";
	m_zTimeFormat = "%h:%m";
	m_cPath = cPath;
	m_b24h = false;
	
	/* Load default icons */
	os::File* pcFile = new os::File( cPath );
	os::Resources cCol( pcFile );
	os::ResStream *pcStream = cCol.GetResourceStream( "icon48x48.png" );
	
	m_pcIcon = new os::BitmapImage( pcStream );
	delete( pcFile );
	
	/* Load settings */
	LoadSettings();
	
}

DockClock::~DockClock( )
{
	delete( m_pcIcon );
}

String DockClock::GetIdentifier()
{
	return( "Clock" );
}

void DockClock::AttachedToWindow()
{
	
	/* Start updater */
	m_pcThread = new DockClockUpdater( this );
	m_pcThread->Start();
	DisplayTime();
}


void DockClock::DetachedFromWindow()
{
	m_pcThread->Terminate();
	delete( m_pcThread );
}

void DockClock::LoadSettings()
{
	/* Load settings */
	os::String zPath = getenv( "HOME" );
	zPath += "/Settings/Dock Clock/Settings";

	try
	{
		os::Settings* pcSettings = new os::Settings( new os::File( zPath ) );
		pcSettings->Load();
		m_b24h = pcSettings->GetBool( "24h", false );

		delete( pcSettings );
	} catch( ... ) {
	}
}

void DockClock::SaveSettings()
{
	/* Save settings */
	os::String zPath = getenv( "HOME" );
	zPath += "/Settings/Dock Clock";

	try
	{
		mkdir( zPath.c_str(), S_IRWXU | S_IRWXG | S_IRWXO );
		zPath += "/Settings";
		os::File cFile( zPath, O_RDWR | O_CREAT );
	
		os::Settings* pcSettings = new os::Settings( new os::File( zPath ) );
		pcSettings->SetBool( "24h", m_b24h );
		pcSettings->Save();
		delete( pcSettings );
	} catch(...)
	{
	}
}

void DockClock::DisplayTime( void )
{

    std::string zTimeStr = m_zTimeFormat;

	long nCurSysTime = get_real_time( ) / 1000000;

    struct tm *psTime = gmtime( &nCurSysTime );

    // Translate the Time Format.

    const char *azTimeToken[] = { "%d", "%H", "%h", "%M", "%m", "%s", "%y" };
 
    long anTimes[] = 
    { 
      psTime->tm_mday,
      psTime->tm_hour,
      (psTime->tm_hour > 12) ? psTime->tm_hour - 12 : psTime->tm_hour,
      psTime->tm_mon + 1,
      psTime->tm_min,
      psTime->tm_sec,
      psTime->tm_year - 100
    };
    
    long anTimes24h[] = 
    { 
      psTime->tm_mday,
      psTime->tm_hour,
      psTime->tm_hour,
      psTime->tm_mon + 1,
      psTime->tm_min,
      psTime->tm_sec,
      psTime->tm_year - 100
    };

    uint nLoc;
    char zValue[4];
    
    for( int n = 0; n < 7; n++ )
    {
    	nLoc = zTimeStr.find( azTimeToken[n] );
    	if( nLoc != std::string::npos ) {
    		if( m_b24h )
    			sprintf( zValue, "%02ld", anTimes24h[n] ); 
    		else
    			sprintf( zValue, "%02ld", anTimes[n] ); 
    		zTimeStr.replace( nLoc, 2, zValue );
    	}
    	
    } 
 
    m_zString = zTimeStr;
	Invalidate( );
	Flush( );
}

void DockClock::Paint( const Rect &cUpdateRect )
{
	Rect cBounds = GetBounds( );
	
    FillRect( cBounds, get_default_color( COL_NORMAL ) );
	
	float fWidth = GetStringWidth( m_zString );
	font_height sHeight;
	GetFontHeight( &sHeight );
	float fHeight = /*sHeight.ascender +*/ sHeight.descender;
	
	float x = ( cBounds.Width() / 2.0f) - (fWidth / 2.0f);
	float y = 4.0f + (cBounds.Height()+1.0f)*0.5f - fHeight*0.5f + sHeight.descender;

	DrawFrame( cBounds, FRAME_RECESSED);

	SetFgColor( 0,0,0 );
	SetBgColor( get_default_color( COL_NORMAL ) );
	MovePenTo( x,y );
  	DrawString( m_zString );
}

Point DockClock::GetPreferredSize( bool bLargest )
{
	
	float fWidth = GetStringWidth( m_zString );
	font_height sHeight;
	GetFontHeight( &sHeight );
	
	return Point( fWidth + 16, sHeight.ascender + sHeight.descender + sHeight.line_gap + 8 );
}

void DockClock::MouseMove( const os::Point& cNewPos, int nCode, uint32 nButtons, os::Message* pcData )
{
	if( nCode != MOUSE_ENTERED && nCode != MOUSE_EXITED )
	{
		/* Create dragging operation */
		if( m_bCanDrag )
		{
			m_bDragging = true;
			os::Message* pcMsg = new os::Message();
			BeginDrag( pcMsg, os::Point( m_pcIcon->GetBounds().Width() / 2,
											m_pcIcon->GetBounds().Height() / 2 ), m_pcIcon->LockBitmap() );
			m_bCanDrag = false;
		}
	}
	os::DockPlugin::MouseMove( cNewPos, nCode, nButtons, pcData );
}


void DockClock::MouseUp( const os::Point & cPosition, uint32 nButtons, os::Message * pcData )
{
	if( m_bDragging && ( cPosition.y > 30 ) )
	{
		/* Remove ourself from the dock */
		os::Message cMsg( os::DOCK_REMOVE );
		cMsg.AddPointer( "plugin", this );
		m_pcDock->PostMessage( &cMsg, m_pcDock );
		return;
	}
	m_bDragging = false;
	m_bCanDrag = false;
	os::DockPlugin::MouseUp( cPosition, nButtons, pcData );
}

void DockClock::MouseDown( const os::Point& cPosition, uint32 nButtons )
{
	MakeFocus( true );
	if( nButtons == os::MOUSE_BUT_LEFT )
		m_bCanDrag = true;
		
	if( nButtons == 2 ) {
		/* Change time format */
		m_b24h = !m_b24h;
		SaveSettings();
	}
	os::DockPlugin::MouseDown( cPosition, nButtons );
}
//*************************************************************************************

extern "C"
{
DockPlugin* init_dock_plugin( os::Path cPluginFile, os::Looper* pcDock )
{
	return( new DockClock( cPluginFile, pcDock ) );
}
}



















