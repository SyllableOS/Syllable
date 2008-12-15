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
#include <util/datetime.h>
#include <time.h>    



#include <appserver/dockplugin.h>
#include "Settings.h"
#include "messages.h"
                                                                                                                                                                                                        
using namespace os;
                                                                                                                                                                                                     
static os::Color32_s BlendColours( const os::Color32_s& sColour1, const os::Color32_s& sColour2, float vBlend )
{
	int r = int( (float(sColour1.red)   * vBlend + float(sColour2.red)   * (1.0f - vBlend)) );
	int g = int( (float(sColour1.green) * vBlend + float(sColour2.green) * (1.0f - vBlend)) );
	int b = int( (float(sColour1.blue)  * vBlend + float(sColour2.blue)  * (1.0f - vBlend)) );
	if ( r < 0 ) r = 0; else if (r > 255) r = 255;
	if ( g < 0 ) g = 0; else if (g > 255) g = 255;
	if ( b < 0 ) b = 0; else if (b > 255) b = 255;
	return os::Color32_s(r, g, b, sColour1.alpha);
}

/* From the Photon Decorator */
static os::Color32_s Tint( const os::Color32_s & sColor, float vTint )
{
	int r = int ( ( float ( sColor.red ) * vTint + 127.0f * ( 1.0f - vTint ) ) );
	int g = int ( ( float ( sColor.green ) * vTint + 127.0f * ( 1.0f - vTint ) ) );
	int b = int ( ( float ( sColor.blue ) * vTint + 127.0f * ( 1.0f - vTint ) ) );

	if( r < 0 )
		r = 0;
	else if( r > 255 )
		r = 255;
	if( g < 0 )
		g = 0;
	else if( g > 255 )
		g = 255;
	if( b < 0 )
		b = 0;
	else if( b > 255 )
		b = 255;
	return ( os::Color32_s( r, g, b, sColor.alpha ) );
}

                                                                                                                                                                                                        
class DockClockUpdater;




class DockClock : public View
{
	public:
		DockClock( os::DockPlugin* pcPlugin, os::Looper* pcDock );
		~DockClock();
		
		Point GetPreferredSize( bool bLargest ) const;
		
		virtual void AttachedToWindow();
		virtual void DetachedFromWindow();
		
        void DisplayTime( void );
        virtual void Paint( const Rect &cUpdateRect );
		
		virtual void MouseMove( const os::Point& cNewPos, int nCode, uint32 nButtons, os::Message* pcData );
		virtual void MouseUp( const os::Point & cPosition, uint32 nButton, os::Message * pcData );
		virtual void MouseDown( const os::Point& cPosition, uint32 nButtons );
        void HandleMessage(Message*);
	private:
		void LoadSettings();
		void SaveSettings();
		void UpdateFont();
		
		os::BitmapImage* m_pcIcon;
		os::DockPlugin* m_pcPlugin;
		os::Looper* m_pcDock;
		bool m_bCanDrag;
		bool m_bDragging;
		
		DockClockSettings* pcClockSettingsWindow;
		
		Color32_s m_cTextColor;
		bool m_bFrame;
		
		std::string m_zTimeFormat;
		std::string m_zDateFormat;
		bool m_bDisplayTime;
		float m_vFontSize;
		String m_cFontType;
		
		String m_zString;
		DockClockUpdater* m_pcThread;
		bigtime_t m_nClickTime;

		int32 m_cTZOffset;
		int32 m_cDst;

		Menu* pcContextMenu;
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

DockClock::DockClock( DockPlugin* pcPlugin, os::Looper* pcDock ) : View( os::Rect(), "dock_clock" )
{
	m_pcPlugin = pcPlugin;
	m_pcDock = pcDock;
	m_bCanDrag = m_bDragging = false;
	m_zString = "99:99:99";
	
	m_nClickTime = 0;
	m_bDisplayTime = true;
	pcClockSettingsWindow = NULL;
	
	
	/* Load default icons */
	os::File* pcFile = new os::File( pcPlugin->GetPath() );
	os::Resources cCol( pcFile );
	os::ResStream *pcStream = cCol.GetResourceStream( "icon48x48.png" );
	
	m_pcIcon = new os::BitmapImage( pcStream );
	delete( pcStream );
	delete( pcFile );
	
	/* Load settings */
	LoadSettings();
	
	pcContextMenu = new Menu(Rect(0,0,10,10),"",ITEMS_IN_COLUMN);
	pcContextMenu->AddItem("Preferences...",new Message(M_PREFS));
	pcContextMenu->AddItem("Date Time", new Message(M_DATE_TIME));
	pcContextMenu->AddItem(new MenuSeparator());	
	pcContextMenu->AddItem("About Clock...",new Message(M_CLOCK_ABOUT));
	pcContextMenu->SetTargetForItems(this);
	
}

void DockClock::HandleMessage(Message* pcMessage)
{
	switch (pcMessage->GetCode())
	{
		case M_PREFS:
		{
			if (pcClockSettingsWindow == NULL)
			{
				Message* pcMsg = new Message();
				pcMsg->AddString("date_format",m_zDateFormat);
				pcMsg->AddString("time_format",m_zTimeFormat);
				pcMsg->AddColor32("color",m_cTextColor);
				pcMsg->AddBool("frame",m_bFrame);
				pcMsg->AddString("font",m_cFontType);
				pcMsg->AddFloat("size",m_vFontSize);
				
				pcClockSettingsWindow = new DockClockSettings(pcMsg,this);
				pcClockSettingsWindow->CenterInScreen();
				pcClockSettingsWindow->Show();
				pcClockSettingsWindow->MakeFocus();
			}
			else
				pcClockSettingsWindow->MakeFocus();
		}
		break;
		case M_DATE_TIME:
		{
			if (fork() == 0)
				execlp("/Applications/Preferences/DateTime","/Applications/Preferences/DateTime",NULL);
		}
		break;
		case M_CLOCK_ABOUT:
		{
			//Display copyright notice
			Alert *pcAlert = new Alert("Copyright Notice",
				"Clock 1.0\n"
				"Copyright (C) 2006 Arno Klenke\n"
				"Copyright (C) 2006 Rick Caudill\n\n"
				"Clock comes with ABSOLUTELY NO WARRANTY.\n"
				"This is free software, and is distributed under the\n"
				"GNU General Public License.\n\n"
				"See http://www.gnu.org for details.",
			Alert::ALERT_INFO, 0, "Ok", NULL );
			pcAlert->Go(new Invoker());
		}
		break;
		case M_APPLY:
		{
			pcMessage->FindString("date_format",&m_zDateFormat);
			pcMessage->FindString("time_format",&m_zTimeFormat);
			pcMessage->FindColor32("color",&m_cTextColor);
			pcMessage->FindBool("frame",&m_bFrame);
			pcMessage->FindString("font",&m_cFontType);
			pcMessage->FindFloat("size",&m_vFontSize);
						
			pcClockSettingsWindow->PostMessage( os::M_TERMINATE );
			pcClockSettingsWindow = NULL;
			SaveSettings();
			UpdateFont();
			Flush();
			break;
		}
		
		case M_CANCEL:
		{
			pcClockSettingsWindow->PostMessage( os::M_TERMINATE );
			pcClockSettingsWindow = NULL;
		break;
		}		
	}
}

DockClock::~DockClock( )
{
	SaveSettings();
	delete( pcContextMenu );
	delete( m_pcIcon );
}

void DockClock::AttachedToWindow()
{
	/* Start updater */
	m_pcThread = new DockClockUpdater( this );
	m_pcThread->Start();
	DisplayTime();
	
	pcContextMenu->SetTargetForItems(this);
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
		os::Settings* pcSettings = new os::Settings( new os::File( zPath) );
		pcSettings->Load();
		
		if (pcSettings->FindString("date_format",&m_zDateFormat) != 0)
			m_zDateFormat = "%a %b %e, %Y";
			
		if (pcSettings->FindString("time_format",&m_zTimeFormat) != 0)
			m_zTimeFormat = "%H:%m:%s";
			
		if (pcSettings->FindColor32("color",&m_cTextColor) != 0)
			m_cTextColor = Color32_s(0,0,0,0);
			
		if (pcSettings->FindBool("frame",&m_bFrame) != 0)
			m_bFrame = true;
			
		if (pcSettings->FindString("font",&m_cFontType) != 0)
			m_cFontType = GetFont()->GetFamily ();
			
		if (pcSettings->FindFloat("size",&m_vFontSize) != 0)
			m_vFontSize = GetFont()->GetSize();
			
		delete( pcSettings );
	} catch( ... ) {
		
		m_vFontSize = GetFont()->GetSize();
		m_cFontType = GetFont()->GetFamily();
		m_cTextColor = Color32_s(0,0,0,0);
		m_zTimeFormat = "%H:%m:%s";
		m_zDateFormat = "%a %b %e, %Y";
		m_bFrame = true;
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
		os::File* cFile = new File( zPath, O_RDWR | O_CREAT );
	
		os::Settings* pcSettings = new os::Settings( cFile );
		pcSettings->SetBool( "frame", m_bFrame );
		pcSettings->SetColor32("color",m_cTextColor);
		pcSettings->SetString("time_format",m_zTimeFormat);
		pcSettings->SetString("date_format",m_zDateFormat);
		pcSettings->SetString("font",m_cFontType);
		pcSettings->SetFloat("size",m_vFontSize);
		pcSettings->Save();
		
		delete( pcSettings );
	} catch(...)
	{
	}
}

void DockClock::UpdateFont()
{
	Font *font = new Font();
	if( font->SetFamilyAndStyle(m_cFontType.c_str(),"Regular") != 0 )
	{
		if( font->SetFamilyAndStyle(m_cFontType.c_str(),"Roman") != 0 )
		{
			printf( "Unknown font!\n" );
			font->SetProperties( DEFAULT_FONT_REGULAR );
		}
	}

	font->SetFlags( FPF_SMOOTHED );
	font->SetSize(m_vFontSize);
	SetFont(font);
	font->Release();
}
	

void DockClock::DisplayTime( void )
{

	if ( m_bDisplayTime )
	{
    	std::string zTimeStr = m_zTimeFormat;
		// long nCurSysTime = get_real_time( ) / 1000000;
		// struct tm *psTime = gmtime( &nCurSysTime );
		time_t sTime;
		time( &sTime );
		tm *psTime = localtime( &sTime );
	    const char *azTimeToken[] = { "%d", "%H", "%h", "%M", "%m", "%s", "%y" };

 		long anTimes[] = 
    	{ 
      		psTime->tm_mday,
      		psTime->tm_hour,
			( psTime->tm_hour > 12 ) ? psTime->tm_hour - 12 : psTime->tm_hour,
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
    		if( nLoc != std::string::npos ) 
    		{
    			if (azTimeToken[n] == "%s" || azTimeToken[n] == "%m")
    				sprintf( zValue, "%02ld", anTimes[n] ); 
    			else if (anTimes[1] == 0 && azTimeToken[n] == "%h")
    				sprintf(zValue,"%ld",12);
    			else
    				sprintf( zValue, "%ld", anTimes[n] );
    				 
    			zTimeStr.replace( nLoc, 2, zValue );
    		}
    	}
    	m_zString = zTimeStr;
    }
    
    else
    {
    	std::string zDateStr = m_zDateFormat;
    	
		
		DateTime d = DateTime::Now();
		char bfr[256];
		struct tm *pstm;
		time_t nTime = d.GetEpoch();
		pstm = localtime( &nTime );
		strftime( bfr, sizeof( bfr ), zDateStr.c_str(), pstm );    	 
 		m_zString = String(bfr);
 	}
 	
	Invalidate( );
	Flush( );
}

void DockClock::Paint( const Rect &cUpdateRect )
{
	Rect cBounds = GetBounds();
	
	os::Color32_s sCurrentColor = BlendColours(get_default_color(os::COL_SHINE),  get_default_color(os::COL_NORMAL_WND_BORDER), 0.5f);
	SetFgColor( Tint( get_default_color( os::COL_SHADOW ), 0.5f ) );
	os::Color32_s sBottomColor = BlendColours(get_default_color(os::COL_SHADOW), get_default_color(os::COL_NORMAL_WND_BORDER), 0.5f);
	
	os::Color32_s sColorStep = os::Color32_s( ( sCurrentColor.red-sBottomColor.red ) / 30, 
											( sCurrentColor.green-sBottomColor.green ) / 30, 
											( sCurrentColor.blue-sBottomColor.blue ) / 30, 0 );
	
	if( cUpdateRect.DoIntersect( os::Rect( 0, 0, GetBounds().right, 29 ) ) )
	{
		sCurrentColor.red -= (int)cUpdateRect.top * sColorStep.red;
		sCurrentColor.green -= (int)cUpdateRect.top * sColorStep.green;
		sCurrentColor.blue -= (int)cUpdateRect.top * sColorStep.blue;
		for( int i = (int)cUpdateRect.top; i < ( (int)cUpdateRect.bottom < 30 ? (int)cUpdateRect.bottom + 1 : 30 ); i++ )
		{
			SetFgColor( sCurrentColor );
			DrawLine( os::Point( cUpdateRect.left, i ), os::Point( cUpdateRect.right, i ) );
			sCurrentColor.red -= sColorStep.red;
			sCurrentColor.green -= sColorStep.green;
			sCurrentColor.blue -= sColorStep.blue;
		}
	}
	
	float fWidth = GetStringWidth( m_zString );
	font_height sHeight;
	GetFontHeight( &sHeight );
	float fHeight = sHeight.descender;
	
	float x = ( cBounds.Width() / 2.0f) - (fWidth / 2.0f);
	float y = 4.0f + (cBounds.Height()+1.0f)*0.5f - fHeight*0.5f + sHeight.descender;

	if (m_bFrame)
		DrawFrame( cBounds, FRAME_RECESSED);

	SetFgColor( m_cTextColor );
	
	MovePenTo( x,y );
  	DrawString( m_zString );
}

Point DockClock::GetPreferredSize( bool bLargest ) const
{
	String cString = "Sat Mar 12, 2005";
	float fWidth = GetStringWidth(cString);
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
			os::Message cMsg;
			BeginDrag( &cMsg, os::Point( m_pcIcon->GetBounds().Width() / 2,
											m_pcIcon->GetBounds().Height() / 2 ), m_pcIcon->LockBitmap() );
			m_bCanDrag = false;
		}
	}
	os::View::MouseMove( cNewPos, nCode, nButtons, pcData );
}


void DockClock::MouseUp( const os::Point & cPosition, uint32 nButtons, os::Message * pcData )
{
	if( m_bDragging && ( cPosition.y > 30 ) )
	{
		/* Remove ourself from the dock */
		os::Message cMsg( os::DOCK_REMOVE_PLUGIN );
		cMsg.AddPointer( "plugin", m_pcPlugin );
		m_pcDock->PostMessage( &cMsg, m_pcDock );
		return;
	}
	m_bDragging = false;
	m_bCanDrag = false;
	os::View::MouseUp( cPosition, nButtons, pcData );
}

void DockClock::MouseDown( const os::Point& cPosition, uint32 nButtons )
{
	MakeFocus( true );
	
	if( nButtons == os::MOUSE_BUT_LEFT )
	{
/*		if (m_nClickTime + 500000 >= get_system_time())
		{
			if (fork() == 0)
				execlp("/Applications/Preferences/DateTime","/Applications/Preferences/DateTime",NULL);
		}
		else
		{
*/			m_bCanDrag = true;
			m_bDisplayTime = !m_bDisplayTime;
			Flush();
/*		}
		m_nClickTime = get_system_time();
*/	}	else if ( nButtons == 2 ) {
		/* Change time format */
		pcContextMenu->Open(ConvertToScreen(cPosition));
	}
	os::View::MouseDown( cPosition, nButtons );
}
//*************************************************************************************

class DockClockPlugin : public os::DockPlugin
{
public:
	DockClockPlugin()
	{
		m_pcView = NULL;
	}
	~DockClockPlugin()
	{
	}
	status_t Initialize()
	{
		m_pcView = new DockClock( this, GetApp() );
		AddView( m_pcView );
		return( 0 );
	}
	void Delete()
	{
		RemoveView( m_pcView );
	}
	os::String GetIdentifier()
	{
		return( "Clock" );
	}
private:
	DockClock* m_pcView;
};

extern "C"
{
DockPlugin* init_dock_plugin( os::Path cPluginFile, os::Looper* pcDock )
{
	return( new DockClockPlugin() );
}
}




