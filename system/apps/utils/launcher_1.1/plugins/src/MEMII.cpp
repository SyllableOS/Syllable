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

#include "launcher_plugin.h"
                                                                                                                                                                                                        
#include <vector>
#include <gui/font.h>
#include <gui/layoutview.h>
#include <gui/menu.h>
#include <sys/types.h>
#include <unistd.h>
#include <atheos/time.h>
#include <atheos/kernel.h>

#include "gbar.h"
                                                                                                                                                                                                        
#define PLUGIN_NAME    "Launcher Mem II"
#define PLUGIN_VERSION "1.0"
#define PLUGIN_AUTHOR  "Andrew Kennan, Henrik Isaksson"
#define PLUGIN_DESC    "A RAM Meter"
                                                                                                                                                                                                        
#define TIMER 500000                                                                                                                                                                                                
                                                                                                                                                                                                        
using namespace os;
                                                                                                                                                                                     
                                                                                                                                                                                                        
class LauncherMem;
class MemMeter;

std::vector<LauncherMem *>g_apcLauncherMem;
uint32 g_nOldSize = 0;
LauncherMessage *g_pcInfoMessage;

class LauncherMem : public LayoutView
{
	public:
		LauncherMem( string zName, LauncherMessage *pcPrefs );
		~LauncherMem( );
		void RemoveView( void );
		void HandleMessage( LauncherMessage *pcMessage );
        void MouseDown( const Point &cPosition, uint32 nButtons  );
        void AttachedToWindow( void );
        
	private:
		LauncherPlugin *m_pcPlugin;
		string m_zName;
		bool m_bAboutAlertOpen;
		Menu *m_pcMenu;
		
        MemMeter *m_pcMeter;
};

class MemMeter : public GradientBar
{
	public:
		MemMeter( );
		~MemMeter( );
		void UpdateMeter( void );
		
	private:
		float m_vValue;
		float m_vOldValue;
		
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
    string zName = (string)"LauncherMemII_" + (string)zID;
    g_apcLauncherMem.push_back( new LauncherMem( zName, pcPrefs ) );
    
    if( g_apcLauncherMem.size( ) <= g_nOldSize )
      return INIT_FAIL;
      
    g_nOldSize = g_apcLauncherMem.size( );
    
    return g_nOldSize - 1;
}


void finish( int nId )
{
	
	if( g_apcLauncherMem[nId] )
    	delete g_apcLauncherMem[nId];
}

LauncherMessage *get_info( void )
{
	return g_pcInfoMessage;
}

View *get_view( int nId )
{
	return (View *)g_apcLauncherMem[nId];
}

void remove_view( int nId )
{
	g_apcLauncherMem[nId]->RemoveView( );
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
	g_apcLauncherMem[nId]->HandleMessage( pcMessage );
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

LauncherMem::LauncherMem( string zName, LauncherMessage *pcPrefs ) : LayoutView( Rect( ), zName.c_str() )
{
	
    m_pcPlugin = pcPrefs->GetPlugin( );
    
	m_zName = zName;
	
    m_pcMenu = new Menu( Rect(0,0,10,10), "PopUp", ITEMS_IN_COLUMN );
    m_pcMenu->AddItem( "About Launcher Mem", new LauncherMessage( LW_PLUGIN_OPEN_ABOUT, m_pcPlugin ) );
    m_pcMenu->AddItem( "Preferences...", new LauncherMessage( LW_PLUGIN_OPEN_PREFS, m_pcPlugin ) );
    m_pcMenu->AddItem( new MenuSeparator( ) );
   	m_pcMenu->AddItem( "Move Up", new LauncherMessage( LW_PLUGIN_MOVE_UP, m_pcPlugin ) );
    m_pcMenu->AddItem( "Move Down", new LauncherMessage( LW_PLUGIN_MOVE_DOWN, m_pcPlugin ) );
   	m_pcMenu->AddItem( new MenuSeparator( ) );
   	m_pcMenu->AddItem( "Delete", new LauncherMessage( LW_PLUGIN_DELETE, m_pcPlugin ) );
   	Message cCloseMessage( LW_ZOOM_UNLOCK );
    m_pcMenu->SetCloseMessage( cCloseMessage );

	VLayoutNode *pcRoot = new VLayoutNode( "Root" );
	
	m_pcMeter = new MemMeter;
	pcRoot->AddChild( m_pcMeter );
		
	SetRoot( pcRoot );
}

LauncherMem::~LauncherMem( )
{

}

void LauncherMem::AttachedToWindow( void )
{
	m_pcMenu->SetCloseMsgTarget( GetWindow( ) ); 
    m_pcMenu->SetTargetForItems( GetWindow( ) );
    m_pcPlugin->AddTimer( TIMER, false );
	m_pcMeter->UpdateMeter();
}

void LauncherMem::MouseDown( const Point &cPosition, uint32 nButtons  )
{
	if( nButtons == 2 ) {
		m_pcPlugin->LockWindow( );
    	Point cPos = ConvertToScreen( cPosition );
	    Point cSize = m_pcMenu->GetPreferredSize( false );
    	Rect cFrame = frame_in_screen( Rect( cPos.x, cPos.y, cPos.x + cSize.x, cPos.y + cSize.y ) ); 
	    m_pcMenu->Open( Point( cFrame.left, cFrame.top) );
	} 
}

void LauncherMem::RemoveView( void )
{
    m_pcPlugin->RemoveTimer( );
	RemoveThis( );
}


void LauncherMem::HandleMessage( LauncherMessage *pcMessage )
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
			m_pcMeter->UpdateMeter();
			break;
		
    	default:
//			pcParentInvoker = new Invoker( new LauncherMessage( pcMessage->GetCode( ), 0, m_pcPlugin), GetWindow() );
//			pcParentInvoker->Invoke();
        	break;
    } 
}


//*************************************************************************************



MemMeter::MemMeter( ) : GradientBar( Rect(), "MemMeter" )
{
	m_vValue = 0;
}

MemMeter::~MemMeter( )
{
	
}

void MemMeter::UpdateMeter( void )
{

	system_info sInfo;
	get_system_info( &sInfo );

	m_vOldValue = m_vValue;
	m_vValue = 1.0f - (float(sInfo.nFreePages) / float(sInfo.nMaxPages));

	SetValue(m_vValue);
}






