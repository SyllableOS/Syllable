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
#include <gui/font.h>
#include <gui/layoutview.h>
#include <gui/menu.h>
#include <sys/types.h>
#include <unistd.h>
#include <atheos/time.h>
#include <atheos/kernel.h>
                                                                                                                                                                                                        
#define PLUGIN_NAME    "Launcher CPU"
#define PLUGIN_VERSION "1.0"
#define PLUGIN_AUTHOR  "Andrew Kennan"
#define PLUGIN_DESC    "A CPU Meter"
                                                                                                                                                                                                        
#define TIMER 50000                                                                                                                                                                                                
                                                                                                                                                                                                        
using namespace os;
                                                                                                                                                                                     
                                                                                                                                                                                                        
class LauncherCPU;
class CPUMeter;

std::vector<LauncherCPU *>g_apcLauncherCPU;
uint32 g_nOldSize = 0;
LauncherMessage *g_pcInfoMessage;

class LauncherCPU : public LayoutView
{
	public:
		LauncherCPU( string zName, LauncherMessage *pcPrefs );
		~LauncherCPU( );
		void RemoveView( void );
		void HandleMessage( LauncherMessage *pcMessage );
        void MouseDown( const Point &cPosition, uint32 nButtons  );
        void AttachedToWindow( void );
        
	private:
		LauncherPlugin *m_pcPlugin;
		string m_zName;
		bool m_bAboutAlertOpen;
		Menu *m_pcMenu;
		int m_nCpuCount;
		
        void UpdateMeters( void );		
        std::vector<CPUMeter *>m_apcMeter;
};

class CPUMeter : public View
{
	public:
		CPUMeter( int nCpuId );
		~CPUMeter();
		virtual void Paint( const Rect &cUpdateRect );
		void UpdateMeter( void );
		Point GetPreferredSize( bool bLargest ) const;
		
	private:
		int m_nCpuId;
		float m_vValue;
		float m_vOldValue;
	    bigtime_t m_nLastIdle;
    	bigtime_t m_nLastSys;
		
};

//*************************************************************************************


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
    string zName = (string)"LauncherCPU_" + (string)zID;
    g_apcLauncherCPU.push_back( new LauncherCPU( zName, pcPrefs ) );
    
    if( g_apcLauncherCPU.size( ) <= g_nOldSize )
      return INIT_FAIL;
      
    g_nOldSize = g_apcLauncherCPU.size( );
    
    return g_nOldSize - 1;
}


void finish( int nId )
{
	
	if( g_apcLauncherCPU[nId] )
    	delete g_apcLauncherCPU[nId];
}

LauncherMessage *get_info( void )
{
	return g_pcInfoMessage;
}

View *get_view( int nId )
{
	return (View *)g_apcLauncherCPU[nId];
}

void remove_view( int nId )
{
	g_apcLauncherCPU[nId]->RemoveView( );
}

LauncherMessage *get_prefs( int nId )
{
	return new LauncherMessage( );
}

void set_prefs( int nId, LauncherMessage *pcPrefs )
{
	
}

void handle_message( int nId, LauncherMessage *pcMessage )
{
	g_apcLauncherCPU[nId]->HandleMessage( pcMessage );
}

/*
** name:       get_prefs_view
** purpose:    Returns a View* to be displayed in the PrefsWindow
** parameters: The ID number of the plugin.
** returns:    A pointer to the plugin's Prefs View.
*/
View *get_prefs_view( int nId )
{
	return NULL;
}

//*************************************************************************************

LauncherCPU::LauncherCPU( string zName, LauncherMessage *pcPrefs ) : LayoutView( Rect( ), zName.c_str() )
{
	
    m_pcPlugin = pcPrefs->GetPlugin( );
    
	m_zName = zName;
	
	m_bAboutAlertOpen = false;
	
	system_info sInfo;
	get_system_info( &sInfo );
	m_nCpuCount = sInfo.nCPUCount;
	
	m_pcMenu = new Menu( Rect(0,0,10,10), "PopUp", ITEMS_IN_COLUMN );
    m_pcMenu->AddItem( "About Launcher CPU", new LauncherMessage( LW_PLUGIN_OPEN_ABOUT, m_pcPlugin ) );
    m_pcMenu->AddItem( "Preferences...", new LauncherMessage( LW_PLUGIN_OPEN_PREFS, m_pcPlugin ) );
    m_pcMenu->AddItem( new MenuSeparator( ) );
   	m_pcMenu->AddItem( "Move Up", new LauncherMessage( LW_PLUGIN_MOVE_UP, m_pcPlugin ) );
    m_pcMenu->AddItem( "Move Down", new LauncherMessage( LW_PLUGIN_MOVE_DOWN, m_pcPlugin ) );
   	m_pcMenu->AddItem( new MenuSeparator( ) );
   	m_pcMenu->AddItem( "Delete", new LauncherMessage( LW_PLUGIN_DELETE, m_pcPlugin ) );
   	Message cCloseMessage( LW_ZOOM_UNLOCK );
    m_pcMenu->SetCloseMessage( cCloseMessage );

	VLayoutNode *pcRoot = new VLayoutNode( "Root" );
	
	for( int n = 0; n < m_nCpuCount; n++ ) {
		m_apcMeter.push_back( new CPUMeter( n ) );
		pcRoot->AddChild( m_apcMeter[m_apcMeter.size() - 1] );
	}
	
	SetRoot( pcRoot );
}

LauncherCPU::~LauncherCPU( )
{

}

void LauncherCPU::AttachedToWindow( void )
{
	m_pcMenu->SetCloseMsgTarget( GetWindow( ) ); 
    m_pcMenu->SetTargetForItems( GetWindow( ) );
    m_pcPlugin->AddTimer( TIMER, false );
    UpdateMeters( );
}

void LauncherCPU::MouseDown( const Point &cPosition, uint32 nButtons  )
{
	if( nButtons == 2 ) {
		m_pcPlugin->LockWindow( );
    	Point cPos = ConvertToScreen( cPosition );
	    Point cSize = m_pcMenu->GetPreferredSize( false );
    	Rect cFrame = frame_in_screen( Rect( cPos.x, cPos.y, cPos.x + cSize.x, cPos.y + cSize.y ) ); 
	    m_pcMenu->Open( Point( cFrame.left, cFrame.top) );
	} 
}

void LauncherCPU::RemoveView( void )
{
    m_pcPlugin->RemoveTimer( );
	RemoveThis( );
}


void LauncherCPU::HandleMessage( LauncherMessage *pcMessage )
{
	int nId = pcMessage->GetCode( );

	switch( nId )
	{

		case LW_PLUGIN_PREFS_BTN_OK:
			m_pcPlugin->RequestWindowRefresh( );
			break;
			
		case LW_PLUGIN_PREFS_BTN_CANCEL:
			m_pcPlugin->RequestWindowRefresh( );
			break;	

    	case LW_PLUGIN_TIMER:
    	  	UpdateMeters( );
			break;
		
    	default:
        	break;
    } 
}


void LauncherCPU::UpdateMeters( void )
{
	for( uint n = 0; n < m_apcMeter.size(); n++ )
		m_apcMeter[n]->UpdateMeter();
}

//*************************************************************************************



CPUMeter::CPUMeter( int nCpuId ) : View( Rect(), "CPUMeter" )
{
	m_nCpuId = nCpuId;
	m_vValue = 0;
}

CPUMeter::~CPUMeter( )
{
	
}

void CPUMeter::Paint( const Rect &cUpdateRect )
{
	
	Rect cBounds = GetBounds( );
	
	DrawFrame( cBounds, FRAME_RECESSED );
	
	cBounds.left += 2;
	cBounds.top += 2;	
	cBounds.right -= 2;
	cBounds.bottom -= 2;

	if( m_vValue > 0.0f ) {
		SetFgColor( 0,0,0 );
		FillRect( Rect( cBounds.left, cBounds.top, cBounds.left + (m_vValue * cBounds.Width()), cBounds.bottom ) );
	}

	if( m_vValue < 1.0f ) {
		SetFgColor( 255,255,255 );
		FillRect( Rect( cBounds.left + (m_vValue * cBounds.Width()), cBounds.top, cBounds.right, cBounds.bottom ) );	
	}
		
}

void CPUMeter::UpdateMeter( void )
{

	bigtime_t	nCurIdleTime;
	bigtime_t	nCurSysTime;

	bigtime_t	nIdleTime;
	bigtime_t	nSysTime;
	
	nCurIdleTime	= get_idle_time( m_nCpuId % 2);
	nCurSysTime 	= get_real_time();
	
	nIdleTime	= nCurIdleTime - m_nLastIdle;
	nSysTime	= nCurSysTime  - m_nLastSys;

	m_nLastIdle = nCurIdleTime;
	m_nLastSys  = nCurSysTime;

	if ( nIdleTime > nSysTime ) {
	    nIdleTime = nSysTime;
	}
  
	if ( nSysTime == 0 ) {
	    nSysTime = 1;
	}

	m_vOldValue = m_vValue;
	m_vValue += ( (1.0f - double(nIdleTime) / double(nSysTime)) - m_vValue) * 0.5f;

	Invalidate( );
	Flush( );
}



Point CPUMeter::GetPreferredSize( bool bLargest ) const
{
	return Point( 94, 18 );
}

