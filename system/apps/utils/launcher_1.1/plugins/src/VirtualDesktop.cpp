/*  Virtual desktop is a virtual desktop switch for Syllable///

	(C)opyright  -2003-   Rick Caudill   

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

#define PLUGIN_NAME    "Virtual Desktop"
#define PLUGIN_VERSION "1.0"
#define PLUGIN_AUTHOR  "Rick Caudill"
#define PLUGIN_DESC    "A Virtual Desktop Manager for Syllable"
#define VD_SWITCH_DESKTOP  15111                                                                                                                                                                                                        
                                                                                                                                                                                                        
                                                                                                                                                                                                        
using namespace os;

class VirtualDesktop;


std::vector<VirtualDesktop*> g_apcVirtualDesktop;
uint32 g_nOldSize = 0;
LauncherMessage* g_pcInfoMessage;
int g_nCurrentDesktop = CURRENT_DESKTOP ;

class VirtualDesktop : View
{
public:
	VirtualDesktop(std::string cName, LauncherMessage* pcPrefs);
	~VirtualDesktop();

	void SetPrefs(LauncherMessage*);
	LauncherMessage* GetPrefs();
	void RemoveView(void);
	void HandleMessage(LauncherMessage*);
	void MouseDown(const Point&, uint32 nButtons);
	void AttachedToWindow(void);
	virtual void Paint(const Rect&);
	virtual Point GetPreferredSize(bool) const;
	View* GetPrefsView(void);
	void DisplayDesktop();

private:
	char m_zString[1024];
	LauncherPlugin *m_pcPlugin;
	std::string m_zName;
	Menu *m_pcMenu;
    Menu* GetVirtualDesktopMenu();
    void SetVirtualDesktopMenuItems();
	LayoutView *m_pcPrefsLayout;
	std::vector <MenuItem*> m_pcVirtualDesktopMenuItems;
	DropdownMenu* m_pcDDMTarget;
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
    string zName = (string)"VirtualDesktop_" + (string)zID;
    g_apcVirtualDesktop.push_back( new VirtualDesktop( zName, pcPrefs ) );
    
    if( g_apcVirtualDesktop.size( ) <= g_nOldSize )
      return INIT_FAIL;
      
    g_nOldSize = g_apcVirtualDesktop.size( );
    
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
	
	if( g_apcVirtualDesktop[nId] )
    	delete g_apcVirtualDesktop[nId];
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
	return (View *)g_apcVirtualDesktop[nId];
}


/*
** name:       remove_view
** purpose:    Instructs a plugin to remove it's View from the window.
** parameters: The ID number of the plugin.
** returns:    nothing.
*/
void remove_view( int nId )
{
	g_apcVirtualDesktop[nId]->RemoveView( );
}


/*
** name:       get_prefs
** purpose:    For getting the Preferences of the plugin.
** parameters: The ID number of the plugin.
** returns:    A pointer to a LauncherMessage.
*/
LauncherMessage *get_prefs( int nId )
{
	return g_apcVirtualDesktop[nId]->GetPrefs( );
}



/*
** name:       set_prefs
** purpose:    Sets the preferences of the plugin.
** parameters: The plugin's ID and a LauncherMessage containing prefs info.
** returns:    nothing.
*/
void set_prefs( int nId, LauncherMessage *pcPrefs )
{
	g_apcVirtualDesktop[nId]->SetPrefs( pcPrefs );
}



/*
** name:       handle_message
** purpose:    Passes messages to the plugin.
** parameters: The plugin's ID and a LauncherMessage.
** returns:    nothing.
*/
void handle_message( int nId, LauncherMessage *pcMessage )
{
	g_apcVirtualDesktop[nId]->HandleMessage( pcMessage );
}

/*
** name:       get_prefs_view
** purpose:    Returns a View* to be displayed in the PrefsWindow
** parameters: The ID number of the plugin.
** returns:    A pointer to the plugin's Prefs View.
*/
View *get_prefs_view( int nId )
{
	return g_apcVirtualDesktop[nId]->GetPrefsView( );
}


VirtualDesktop::VirtualDesktop(std::string zName, LauncherMessage* pcPrefs) : View(Rect(),zName.c_str())
{
	m_pcPlugin = pcPrefs->GetPlugin();
	
	m_zName = zName;
	
	SetPrefs(pcPrefs);
	
	SetVirtualDesktopMenuItems();
	
	m_pcMenu = new Menu( Rect(0,0,10,10), "PopUp", ITEMS_IN_COLUMN );
    m_pcMenu->AddItem( "About Virtual Desktop...", new LauncherMessage( LW_PLUGIN_OPEN_ABOUT, m_pcPlugin ) );
   	m_pcMenu->AddItem( "Preferences...", new LauncherMessage( LW_PLUGIN_OPEN_PREFS, m_pcPlugin ) );
    m_pcMenu->AddItem( new MenuSeparator( ) );
    m_pcMenu->AddItem( GetVirtualDesktopMenu());
	m_pcMenu->AddItem( new MenuSeparator( ) );
   	m_pcMenu->AddItem( "Move Up", new LauncherMessage( LW_PLUGIN_MOVE_UP, m_pcPlugin ) );
    m_pcMenu->AddItem( "Move Down", new LauncherMessage( LW_PLUGIN_MOVE_DOWN, m_pcPlugin ) );
   	m_pcMenu->AddItem( new MenuSeparator( ) );
   	m_pcMenu->AddItem( "Delete", new LauncherMessage( LW_PLUGIN_DELETE, m_pcPlugin ) );
   	Message cCloseMessage( LW_ZOOM_UNLOCK );
    m_pcMenu->SetCloseMessage( cCloseMessage );

    sprintf(m_zString, "Desktop #%d", g_nCurrentDesktop+1);

}

Menu* VirtualDesktop::GetVirtualDesktopMenu()
{
	Menu* m_pcVirtualDesktopMenu = new Menu( Rect(0,0,10,10), "Switch to", ITEMS_IN_COLUMN );
	
	for (uint i =0;i<m_pcVirtualDesktopMenuItems.size(); i++)
		m_pcVirtualDesktopMenu->AddItem(m_pcVirtualDesktopMenuItems[i]);
	
	return m_pcVirtualDesktopMenu;
}

void VirtualDesktop::SetVirtualDesktopMenuItems()
{
		for (uint i = 0; i<= 14; i++){
		std::string zMenuItemName;
		sprintf((char*)zMenuItemName.c_str(),"%s %d","Desktop ", i+1);
		LauncherMessage* pcMsg = new LauncherMessage(VD_SWITCH_DESKTOP, m_pcPlugin);
		pcMsg->AddInt32("number", i);
		m_pcVirtualDesktopMenuItems.push_back(new MenuItem(zMenuItemName.c_str(), pcMsg));
	}
}

VirtualDesktop::~VirtualDesktop( )
{
}


View *VirtualDesktop::GetPrefsView( void )
{
	return m_pcPrefsLayout;
}

void VirtualDesktop::AttachedToWindow( void )
{
	m_pcMenu->SetCloseMsgTarget( GetWindow( ) ); 
    m_pcMenu->SetTargetForItems( GetWindow( ) );
	m_pcPlugin->AddTimer( 1000000, false );
	DisplayDesktop( );
}

void VirtualDesktop::MouseDown( const Point &cPosition, uint32 nButtons  )
{
	if( nButtons == 2 ) {
		GetWindow( )->PostMessage( LW_ZOOM_LOCK );
    	Point cPos = ConvertToScreen( cPosition );
	    Point cSize = m_pcMenu->GetPreferredSize( false );
    	Rect cFrame = frame_in_screen( Rect( cPos.x, cPos.y, cPos.x + cSize.x, cPos.y + cSize.y ) ); 
	    m_pcMenu->Open( Point( cFrame.left, cFrame.top) );
	} 
}

void VirtualDesktop::SetPrefs( LauncherMessage *pcPrefs )
{
}



LauncherMessage *VirtualDesktop::GetPrefs( void )
{
    LauncherMessage *pcPrefs = new LauncherMessage( 0, m_pcPlugin);
    return pcPrefs;
}


void VirtualDesktop::RemoveView( void )
{
    m_pcPlugin->RemoveTimer( );
	RemoveThis( );
}

void VirtualDesktop::DisplayDesktop()
{
	sprintf(m_zString, "Desktop %d", g_nCurrentDesktop+1);

	Invalidate();
	Flush();
}


void VirtualDesktop::HandleMessage( LauncherMessage *pcMessage )
{

	int nId = pcMessage->GetCode( );

	switch( nId )
	{
		
      case LW_PLUGIN_PREFS_BTN_OK:
		m_pcPlugin->RequestWindowRefresh();
        break;

      case LW_PLUGIN_PREFS_BTN_CANCEL:
		m_pcPlugin->RequestWindowRefresh();
        break;

		case LW_PLUGIN_WINDOW_POS_CHANGED:
      	m_pcPlugin->AddTimer( 1000000, false );
      	break;
      	
      	case VD_SWITCH_DESKTOP:
      	{
      		int nDesktop;
      		pcMessage->FindInt("number", &nDesktop);
			Desktop cDesktop(nDesktop);
			cDesktop.Activate();
			g_nCurrentDesktop = nDesktop;
			DisplayDesktop();
      	}
      	break;
      	
      	
      default:   	
        break;
    }
}



void VirtualDesktop::Paint( const Rect &cUpdateRect )
{
	Rect cBounds = GetBounds( );
	
    FillRect( cBounds, get_default_color( COL_NORMAL ) );
	
	float fWidth = GetStringWidth( m_zString );
	font_height sHeight;
	GetFontHeight( &sHeight );
	float fHeight = /*sHeight.ascender +*/ sHeight.descender;
	
	float x = ( cBounds.Width() / 2.0f) - (fWidth / 2.0f);
	float y = 2.0f + (cBounds.Height()+1.0f)*0.5f - fHeight*0.5f + sHeight.descender;

	DrawFrame( cBounds, FRAME_RECESSED);

	SetFgColor( 0,0,0 );
	SetBgColor( get_default_color( COL_NORMAL ) );
	MovePenTo( x,y );
  	DrawString( m_zString );
}

Point VirtualDesktop::GetPreferredSize( bool bLargest ) const
{
	
	float fWidth = GetStringWidth( m_zString );
	font_height sHeight;
	GetFontHeight( &sHeight );
	
	return Point( fWidth + 16, sHeight.ascender + sHeight.descender + sHeight.line_gap + 8 );
}










