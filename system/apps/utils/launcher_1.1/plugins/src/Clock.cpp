/*
    Launcher - A program launcher for AtheOS
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

#include "../../include/launcher_plugin.h"
                                                                                                                                                                                                        
#include <vector>
#include <gui/button.h>
#include <gui/layoutview.h>
#include <gui/stringview.h>
#include <util/looper.h>
#include <gui/requesters.h>
#include <gui/dropdownmenu.h>
#include <gui/menu.h>
#include <gui/checkbox.h>
#include <gui/desktop.h>
#include <sys/types.h>
#include <unistd.h>
#include <atheos/time.h>
#include <posix/time.h>
#include <time.h>
                                                                                                                                                                                                        
#define PLUGIN_NAME    "Launcher Clock"
#define PLUGIN_VERSION "1.0"
#define PLUGIN_AUTHOR  "Andrew Kennan"
#define PLUGIN_DESC    "A Clock For The Launcher"
                                                                                                                                                                                                        
                                                                                                                                                                                                        
                                                                                                                                                                                                        
using namespace os;
                                                                                                                                                                                                     
                                                                                                                                                                                                        
class LauncherClock;


std::vector<LauncherClock *>g_apcLauncherClock;
uint32 g_nOldSize = 0;
LauncherMessage *g_pcInfoMessage;

class LauncherClock : View
{
	public:
		LauncherClock( string zName, LauncherMessage *pcPrefs );
		~LauncherClock( );
		void SetPrefs( LauncherMessage *pcPrefs );
		LauncherMessage *GetPrefs( void );
		void RemoveView( void );
		void HandleMessage( LauncherMessage *pcMessage );
        void MouseDown( const Point &cPosition, uint32 nButtons  );
        void AttachedToWindow( void );
        void DisplayTime( void );
        virtual void Paint( const Rect &cUpdateRect );
        virtual Point GetPreferredSize( bool bLargest ) const;
        View *GetPrefsView( void );
        
	private:
		string m_zString;
		LauncherPlugin *m_pcPlugin;
		string m_zName;
		string m_zTimeFormat;
		Menu *m_pcMenu;
        DropdownMenu *m_pcDDTimeFormat;
		LayoutView *m_pcPrefsLayout;
};




//*************************************************************************************

/*
** name:       init
** purpose:    Initialises a new instance of a plugin
** parameters: A pointer to a LauncherMessage containing config info for the plugin
** returns:    An ID number for the instance of the plugin or INIT_FAIL if the plugin
**             could not be created.
*/
int init( LauncherMessage *pcPrefs )
{
	if( ! g_pcInfoMessage ) {
		g_pcInfoMessage = new LauncherMessage( );
    	g_pcInfoMessage->AddString( "Name",        PLUGIN_NAME    );
	    g_pcInfoMessage->AddString( "Version",     PLUGIN_VERSION );
    	g_pcInfoMessage->AddString( "Author",      PLUGIN_AUTHOR  );
	    g_pcInfoMessage->AddString( "Description", PLUGIN_DESC    );
	}
    
    char zID[8];
    sprintf( zID, "%ld", g_nOldSize );
    string zName = (string)"LauncherClock_" + (string)zID;
    g_apcLauncherClock.push_back( new LauncherClock( zName, pcPrefs ) );
    
    if( g_apcLauncherClock.size( ) <= g_nOldSize )
      return INIT_FAIL;
      
    g_nOldSize = g_apcLauncherClock.size( );
    
    return g_nOldSize - 1;
}


/*
** name:       finish
** purpose:    Deletes an instance of a plugin
** parameters: The ID number returned by init()
** returns:    none.
*/
void finish( int nId )
{
	
	if( g_apcLauncherClock[nId] )
    	delete g_apcLauncherClock[nId];
}


/*
** name:       get_info
** purpose:    Returns information about the plugin.
** parameters: none
** returns:    A pointer to a LauncherMessage containing info about the plugin.
*/
LauncherMessage *get_info( void )
{
	return g_pcInfoMessage;
}


/*
** name:       get_view
** purpose:    Returns a View* to be displayed in a window.
** parameters: The ID number of the plugin.
** returns:    A pointer to the plugin's View.
*/
View *get_view( int nId )
{
	return (View *)g_apcLauncherClock[nId];
}


/*
** name:       remove_view
** purpose:    Instructs a plugin to remove it's View from the window.
** parameters: The ID number of the plugin.
** returns:    nothing.
*/
void remove_view( int nId )
{
	g_apcLauncherClock[nId]->RemoveView( );
}


/*
** name:       get_prefs
** purpose:    For getting the Preferences of the plugin.
** parameters: The ID number of the plugin.
** returns:    A pointer to a LauncherMessage.
*/
LauncherMessage *get_prefs( int nId )
{
	return g_apcLauncherClock[nId]->GetPrefs( );
}



/*
** name:       set_prefs
** purpose:    Sets the preferences of the plugin.
** parameters: The plugin's ID and a LauncherMessage containing prefs info.
** returns:    nothing.
*/
void set_prefs( int nId, LauncherMessage *pcPrefs )
{
	g_apcLauncherClock[nId]->SetPrefs( pcPrefs );
}



/*
** name:       handle_message
** purpose:    Passes messages to the plugin.
** parameters: The plugin's ID and a LauncherMessage.
** returns:    nothing.
*/
void handle_message( int nId, LauncherMessage *pcMessage )
{
	g_apcLauncherClock[nId]->HandleMessage( pcMessage );
}

/*
** name:       get_prefs_view
** purpose:    Returns a View* to be displayed in the PrefsWindow
** parameters: The ID number of the plugin.
** returns:    A pointer to the plugin's Prefs View.
*/
View *get_prefs_view( int nId )
{
	return g_apcLauncherClock[nId]->GetPrefsView( );
}

//*************************************************************************************

LauncherClock::LauncherClock( string zName, LauncherMessage *pcPrefs ) : View( Rect( ), zName.c_str() )
{
	
    m_pcPlugin = pcPrefs->GetPlugin( );
    
	m_zName = zName;
	
	m_zTimeFormat = "%h:%m:%s";
	
	SetPrefs( pcPrefs );
		
    m_pcMenu = new Menu( Rect(0,0,10,10), "PopUp", ITEMS_IN_COLUMN );
    m_pcMenu->AddItem( "About Launcher Clock", new LauncherMessage( LW_PLUGIN_OPEN_ABOUT, m_pcPlugin ) );
   	m_pcMenu->AddItem( "Preferences", new LauncherMessage( LW_PLUGIN_OPEN_PREFS, m_pcPlugin ) );
    m_pcMenu->AddItem( new MenuSeparator( ) );
   	m_pcMenu->AddItem( "Move Up", new LauncherMessage( LW_PLUGIN_MOVE_UP, m_pcPlugin ) );
    m_pcMenu->AddItem( "Move Down", new LauncherMessage( LW_PLUGIN_MOVE_DOWN, m_pcPlugin ) );
   	m_pcMenu->AddItem( new MenuSeparator( ) );
   	m_pcMenu->AddItem( "Delete", new LauncherMessage( LW_PLUGIN_DELETE, m_pcPlugin ) );
   	Message cCloseMessage( LW_ZOOM_UNLOCK );
    m_pcMenu->SetCloseMessage( cCloseMessage );

	HLayoutNode *pcRoot = new HLayoutNode( "clock_prefs_root" );
	pcRoot->AddChild( new VLayoutSpacer( "spacer_01", 1.0f ) );
	pcRoot->AddChild( new StringView( Rect(0,0,0,0), "label", "Time Format", ALIGN_CENTER ) );

	m_pcDDTimeFormat = new DropdownMenu( Rect(), "TimeFormat" );
	m_pcDDTimeFormat->SetCurrentString( m_zTimeFormat );
	m_pcDDTimeFormat->AppendItem( m_zTimeFormat.c_str() );
	m_pcDDTimeFormat->AppendItem( "%h:%m:%s" );
	m_pcDDTimeFormat->AppendItem( "%h:%m" );
	m_pcDDTimeFormat->AppendItem( "%h:%m %d/%M" );
	m_pcDDTimeFormat->AppendItem( "%h:%m:%s %d/%M" );
	m_pcDDTimeFormat->AppendItem( "%h:%m %d/%M/%y" );
	m_pcDDTimeFormat->AppendItem( "%h:%m:%s %d/%M/%y" );
	m_pcDDTimeFormat->AppendItem( "%h:%m:%s %M/%d/%y" );
	pcRoot->AddChild( m_pcDDTimeFormat );

	m_pcPrefsLayout = new LayoutView( Rect( ), "clock_prefs_layout", pcRoot );
     
}

LauncherClock::~LauncherClock( )
{
}


View *LauncherClock::GetPrefsView( void )
{
	m_pcDDTimeFormat->SetCurrentString( m_zTimeFormat );
	return m_pcPrefsLayout;
}

void LauncherClock::AttachedToWindow( void )
{
	m_pcMenu->SetCloseMsgTarget( GetWindow( ) ); 
    m_pcMenu->SetTargetForItems( GetWindow( ) );
	m_pcPlugin->AddTimer( 1000000, false );
    DisplayTime( );
}

void LauncherClock::MouseDown( const Point &cPosition, uint32 nButtons  )
{
	if( nButtons == 2 ) {
		GetWindow( )->PostMessage( LW_ZOOM_LOCK );
    	Point cPos = ConvertToScreen( cPosition );
	    Point cSize = m_pcMenu->GetPreferredSize( false );
    	Rect cFrame = frame_in_screen( Rect( cPos.x, cPos.y, cPos.x + cSize.x, cPos.y + cSize.y ) ); 
	    m_pcMenu->Open( Point( cFrame.left, cFrame.top) );
	} 
}

void LauncherClock::SetPrefs( LauncherMessage *pcPrefs )
{
    pcPrefs->FindString( "TimeFormat", &m_zTimeFormat );
    DisplayTime( );
}



LauncherMessage *LauncherClock::GetPrefs( void )
{
    LauncherMessage *pcPrefs = new LauncherMessage( 0, m_pcPlugin);
    pcPrefs->AddString( "TimeFormat", m_zTimeFormat );
    return pcPrefs;
}


void LauncherClock::RemoveView( void )
{
    m_pcPlugin->RemoveTimer( );
	RemoveThis( );
}


void LauncherClock::HandleMessage( LauncherMessage *pcMessage )
{

	int nId = pcMessage->GetCode( );

	switch( nId )
	{
		
      case LW_PLUGIN_PREFS_BTN_OK:
      	m_zTimeFormat = m_pcDDTimeFormat->GetCurrentString( );
        DisplayTime();
		m_pcPlugin->RequestWindowRefresh();
        break;

      case LW_PLUGIN_PREFS_BTN_CANCEL:
        DisplayTime();
		m_pcPlugin->RequestWindowRefresh();
        break;

      case LW_PLUGIN_TIMER:
      	DisplayTime( );
      	break;
      	
      case LW_PLUGIN_WINDOW_POS_CHANGED:
      	m_pcPlugin->AddTimer( 1000000, false );
      	break;
      	
      default:
//		pcParentInvoker = new Invoker( new LauncherMessage( nId, m_pcPlugin), GetWindow() );
//		pcParentInvoker->Invoke();      	
        break;
    }
}


void LauncherClock::DisplayTime( void )
{

    string zTimeStr = m_zTimeFormat;

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

    uint nLoc;
    char zValue[4];
    
    for( int n = 0; n < 7; n++ )
    {
    	nLoc = zTimeStr.find( azTimeToken[n] );
    	if( nLoc != string::npos ) {
    		sprintf( zValue, "%02ld", anTimes[n] ); 
    		zTimeStr.replace( nLoc, 2, zValue );
    	}
    	
    } 
 
    m_zString = zTimeStr;
	Invalidate( );
	Flush( );
}

void LauncherClock::Paint( const Rect &cUpdateRect )
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

Point LauncherClock::GetPreferredSize( bool bLargest ) const
{
	
	float fWidth = GetStringWidth( m_zString );
	font_height sHeight;
	GetFontHeight( &sHeight );
	
	return Point( fWidth + 16, sHeight.ascender + sHeight.descender + sHeight.line_gap + 8 );
}

//*************************************************************************************



