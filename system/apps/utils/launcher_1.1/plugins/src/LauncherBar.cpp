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


/************************************************************

	Name : LauncherBar
	Version : 1.0.2
	Author	: Andrew Kennan
	
	A Launcher plugin which displays a list of buttons
	corresponding to files and directories within a specified
	directory.
	
************************************************************/

#include "../../include/launcher_plugin.h"
#include "../../include/launcher_bar.h"


#include <gui/stringview.h>
#include <gui/textview.h>
#include <gui/menu.h>
#include <gui/checkbox.h>
#include <storage/nodemonitor.h>

// For the About message.                                                                                                                                            
#define PLUGIN_NAME    "Launcher Bar"
#define PLUGIN_VERSION "1.0.2"
#define PLUGIN_AUTHOR  "Andrew Kennan"
#define PLUGIN_DESC    "A Box Full Of Buttons"
  
#define LI_OPEN_TARGET_REQ 25001  
#define LI_TARGET_CHANGED  25002
#define LB_LAUNCHBROWSER_CHANGED 25003
#define LB_DIR_CHANGED 25004
                                                                                                                                                                                                                       
using namespace os;

class LauncherBar;         // The main plugin class
class LauncherBarView;
class LauncherBarNodeMon;


std::vector<LauncherBar *>g_apcLauncherBar; // A list of plugins.
uint32 g_nOldSize = 0;
LauncherMessage *g_pcInfoMessage;


class LauncherBar
{
	public:
		LauncherBar( string zName, LauncherMessage *pcPrefs );
		~LauncherBar( );
		View *GetView( void );
		View *GetPrefsView( void );
		void RemoveView( void );
		LauncherMessage *GetPrefs( void );
		void SetPrefs( LauncherMessage *pcPrefs );
		void HandleMessage( LauncherMessage *pcMessage );
		
	private:
		int32 m_nAlign;
		int32 m_nVPos;
		int32 m_nHPos;
		int32 m_nHideStatus;
		int32 m_nWindowPos;
		string m_zDirectory;
		string m_zName;
		bool m_bLaunchBrowser;
		bool m_bDrawBorder;
		
		LauncherPlugin *m_pcPlugin;
		LauncherBarView *m_pcView;
		
		LayoutView *m_pcPrefsLayout;
		TextView *m_pcTVDirectory;
		FileRequester *m_pcFRTarget;		
		CheckBox *m_pcCBLaunchBrowser;
		CheckBox *m_pcCBDrawBorder;
		LauncherSubWindow *m_pcSubWindow;

		void OpenSubWindow( LauncherMessage *pcMessage );
		void CloseSubWindow( void );
		void RecreateView( void );
};


class LauncherBarView : public LauncherView
{
	public:
		LauncherBarView( string zName, LauncherMessage *pcPrefs, bool bNoMenu = false );
		~LauncherBarView( );
		void MouseDown( const os::Point &, long unsigned int );
		void AttachedToWindow( void );
				
	private:
		NodeMonitor *m_pcNodeMonitor;
		LauncherPlugin *m_pcPlugin;
		Menu *m_pcMenu;
};
	

class LauncherBarNodeMon : public Handler
{
	public:
		LauncherBarNodeMon( Window *pcTarget, Message *pcMessage );
		~LauncherBarNodeMon( );
		void HandleMessage( Message *pcMessage );
	
	private:
		Window *m_pcTarget;
		Message *m_pcMessage;
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
	
	if( ! g_pcInfoMessage ) {  // Create a message containing info about the plugin.
		g_pcInfoMessage = new LauncherMessage( );
	    g_pcInfoMessage->AddString( "Name",        PLUGIN_NAME    );
    	g_pcInfoMessage->AddString( "Version",     PLUGIN_VERSION );
	    g_pcInfoMessage->AddString( "Author",      PLUGIN_AUTHOR  );
    	g_pcInfoMessage->AddString( "Description", PLUGIN_DESC    );
    }
    char zID[8];
    sprintf( zID, "%ld", g_nOldSize );
    string zName = (string)"LauncherBar_" + (string)zID;
    g_apcLauncherBar.push_back( new LauncherBar( zName, pcPrefs ) );
    
    if( g_apcLauncherBar.size( ) <= g_nOldSize )
      return INIT_FAIL;
      
    g_nOldSize = g_apcLauncherBar.size( );

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
	if( g_apcLauncherBar[nId] )
    	delete g_apcLauncherBar[nId];
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
	return g_apcLauncherBar[nId]->GetView( );
}


/*
** name:       remove_view
** purpose:    Instructs a plugin to remove it's View from the window.
** parameters: The ID number of the plugin.
** returns:    nothing.
*/
void remove_view( int nId )
{
	g_apcLauncherBar[nId]->RemoveView( );
}


/*
** name:       get_prefs
** purpose:    For getting the Preferences of the plugin.
** parameters: The ID number of the plugin.
** returns:    A pointer to a LauncherMessage.
*/
LauncherMessage *get_prefs( int nId )
{
	return g_apcLauncherBar[nId]->GetPrefs( );
}


/*
** name:       set_prefs
** purpose:    Sets the preferences of the plugin.
** parameters: The plugin's ID and a LauncherMessage containing prefs info.
** returns:    nothing.
*/
void set_prefs( int nId, LauncherMessage *pcPrefs )
{
	g_apcLauncherBar[nId]->SetPrefs( pcPrefs );
}



/*
** name:       handle_message
** purpose:    Passes messages to the plugin.
** parameters: The plugin's ID and a LauncherMessage.
** returns:    nothing.
*/
void handle_message( int nId, LauncherMessage *pcMessage )
{
	g_apcLauncherBar[nId]->HandleMessage( pcMessage );
}


/*
** name:       get_prefs_view
** purpose:    Returns a View* to be displayed in the PrefsWindow
** parameters: The ID number of the plugin.
** returns:    A pointer to the plugin's Prefs View.
*/
View *get_prefs_view( int nId )
{
	return g_apcLauncherBar[nId]->GetPrefsView( );
}

//*************************************************************************************
/*
** name:       LauncherBar
** purpose:    The core class of the LauncherBar plugin.
** parameters: The name of the plugin and a LauncherMessage containing prefs info.
**
**************************************************************************************/
LauncherBar::LauncherBar( string zName, LauncherMessage *pcPrefs )
{
	m_zName       = zName;  // Get the name of the plugin.
	m_zDirectory  = "~/config/Launcher"; // Default directory to display.
	
	m_nAlign      = LW_ALIGN_VERTICAL;   // Default values.
	m_nHPos       = LW_HPOS_LEFT;
	m_nVPos       = LW_VPOS_TOP;
	m_bLaunchBrowser = false;
	m_bDrawBorder = true;
	
	m_pcSubWindow   = 0;  // SubWindow closed.
	
	SetPrefs( pcPrefs );  // Retrieve prefs info.
	
	VLayoutNode *pcPrefsVRoot = new VLayoutNode( "lb_prefs_vroot" );
	HLayoutNode *pcPrefsRoot = new HLayoutNode( "lb_prefs_root" );
	m_pcTVDirectory = new TextView( Rect(), "tv_dir", m_zDirectory.c_str() );
	m_pcCBLaunchBrowser = new CheckBox( Rect(), "cb_lbrowse", "Launch LBrowse On Directories", new LauncherMessage( LB_LAUNCHBROWSER_CHANGED, m_pcPlugin ) );
	m_pcCBDrawBorder = new CheckBox( Rect(), "cb_border", "Draw Borders Around Buttons", new LauncherMessage( LB_LAUNCHBROWSER_CHANGED, m_pcPlugin ) );
	pcPrefsRoot->AddChild( new VLayoutSpacer( "spacer_0", 1.0f ) );
	pcPrefsRoot->AddChild( new StringView( Rect(), "sv_label", "Directory" ) );
	pcPrefsRoot->AddChild( m_pcTVDirectory );
	ImageButton *pcDirBtn = new ImageButton( Rect(), "btn_fr_0", "", new LauncherMessage( LI_OPEN_TARGET_REQ, m_pcPlugin ) );
	string zIcon = get_launcher_path() + (string)"icons/folder.png";
	pcDirBtn->SetImageFromFile( zIcon.c_str() );
	pcPrefsRoot->AddChild( pcDirBtn ); 
	pcPrefsVRoot->AddChild( pcPrefsRoot );
	pcPrefsVRoot->AddChild( m_pcCBLaunchBrowser );
	pcPrefsVRoot->AddChild( m_pcCBDrawBorder );
	m_pcPrefsLayout = new LayoutView( Rect(), "lb_prefs_layout" );
	m_pcPrefsLayout->SetRoot( pcPrefsVRoot );
	
	m_pcView = new LauncherBarView( m_zName, GetPrefs( ) );  // Create the LauncherView
	
	if( ! m_pcView )  // Die if creating the View failed.
		delete this;

	m_pcFRTarget = NULL;		
	if( m_pcPlugin->GetWindow() )
		m_pcFRTarget = new FileRequester( FileRequester::LOAD_REQ, new Messenger(m_pcPlugin->GetWindow()), convert_path(m_pcTVDirectory->GetBuffer()[0]).c_str(), FileRequester::NODE_DIR,false,new LauncherMessage( LI_TARGET_CHANGED, m_pcPlugin ) );
}


LauncherBar::~LauncherBar( )
{
	if( m_pcView->GetWindow() )
		m_pcView->RemoveThis( );  // Detach the View from it's window if necessary.
	delete m_pcView;
	
	if( m_pcFRTarget != NULL ) m_pcFRTarget->Quit();
	
	CloseSubWindow( );  // Close the SubWindow
}


/*
** name:       LauncherBar::SetPrefs
** purpose:    Set the preferences info for the plugin.
** parameters: A LauncherMessage containing prefs info.
** returns:    nothing.
*/
void LauncherBar::SetPrefs( LauncherMessage *pcPrefs )
{
	pcPrefs->FindInt32( "Align",      &m_nAlign );
	pcPrefs->FindInt32( "HPos",       &m_nHPos );
	pcPrefs->FindInt32( "VPos",       &m_nVPos );
	pcPrefs->FindBool( "LaunchBrowser", &m_bLaunchBrowser );
	pcPrefs->FindString( "Directory", &m_zDirectory );
	pcPrefs->FindBool( "DrawBorder", &m_bDrawBorder );
	
	if( pcPrefs->GetPlugin() )
		m_pcPlugin = pcPrefs->GetPlugin( );

}


/*
** name:       LauncherBar::GetPrefs
** purpose:    For retrieving the Prefs info of the plugin.
** parameters: none.
** returns:    A LauncherMessage containing prefs info.
*/
LauncherMessage *LauncherBar::GetPrefs( void )
{
	LauncherMessage *pcPrefs = new LauncherMessage( );

	pcPrefs->AddInt32( "Align",      m_nAlign );
	pcPrefs->AddInt32( "HPos",       m_nHPos );
	pcPrefs->AddInt32( "VPos",       m_nVPos );
	pcPrefs->AddBool( "LaunchBrowser", m_bLaunchBrowser );
	pcPrefs->AddString( "Directory", m_zDirectory );
	pcPrefs->AddBool( "DrawBorder", m_bDrawBorder );
    pcPrefs->SetPlugin( m_pcPlugin );

	return pcPrefs;
}



/*
** name:       LauncherBar::GetView
** purpose:    Gets the plugin's View for display in the LauncherWindow
** parameters: none
** returns:    A pointer to the LauncherView
*/
View *LauncherBar::GetView( void )
{
	return m_pcView;
}



/*
** name:       LauncherBar::GetPrefsView
** purpose:    Gets the plugin's prefs View for display in the Prefs window
** parameters: none
** returns:    A pointer to the Prefs view
*/
View *LauncherBar::GetPrefsView( void )
{
	m_pcTVDirectory->Set( m_zDirectory.c_str( ) );
	m_pcCBLaunchBrowser->SetValue( m_bLaunchBrowser );
	m_pcCBDrawBorder->SetValue( m_bDrawBorder );
	return m_pcPrefsLayout;
}


/*
** name:       LauncherBar::RemoveView
** purpose:    Removes the LauncherView from it's window
** parameters: none
** returns:    nothing
*/
void LauncherBar::RemoveView( void )
{
	  
	CloseSubWindow();  // Close the SubWindow
	m_pcView->RemoveThis( );  // Remove the View
	
}

	
/*
** name:       LauncherBar::OpenSubWindow
** purpose:    Opens a LauncherSubWindow
** parameters: A LauncherMessage containing the directory to list and the position of the parent button.
** returns:    nothing.
*/
void LauncherBar::OpenSubWindow( LauncherMessage *pcMessage )
{
	if( m_pcSubWindow ) {  // Close the SubWindow if it is already open
		m_pcSubWindow->Quit( );
		m_pcSubWindow = 0;
	}
	
	
	  // Instruct other LauncherBar type plugins to close their sub-windows
	m_pcPlugin->BroadcastMessage( LB_CLOSE_SUB_WINDOW );
	
	  // Lock the LauncherWindow
	m_pcPlugin->LockWindow( );
	
	  // Add extra positioning info to the Message
	pcMessage->AddInt32( "Align",      m_nAlign );
	pcMessage->AddInt32( "HPos",       m_nHPos );
	pcMessage->AddInt32( "VPos",       m_nVPos );	
	pcMessage->AddPointer( "ParentWindow", (void **)m_pcView->GetWindow( ) );  // Used to send messages back to the parent.
	pcMessage->AddBool( "DrawBorder", m_bDrawBorder );
	
	  // Create the SubWindow
	m_pcSubWindow = new LauncherSubWindow( "LauncherSubWindow", pcMessage );
	m_pcSubWindow->Show( );
	
	  // Timeout if there is no action taken within 5 seconds.
	m_pcPlugin->AddTimer( 5000000, false );
}


/*
** name:       LauncherBar::CloseSubWindow
** purpose:    Closes the SubWindow
** parameters: none
** returns:    nothing.
*/
void LauncherBar::CloseSubWindow( void )
{

	if( m_pcSubWindow ) {  // If the SubWindow is open
		m_pcPlugin->RemoveTimer( );  // Remove the timeout
		m_pcSubWindow->Quit( );  // Close the Window
		m_pcSubWindow = 0;
		m_pcPlugin->UnlockWindow( );
	}
}


/*
** name:       LauncherBar::RecreateView
** purpose:    Deletes and then recreates the LauncherView if the preferences of the plugin or window are changed.
** parameters: none
** returns:    nothing
*/
void LauncherBar::RecreateView( void )
{
	Window *pcWindow = 0;
	if( m_pcView->GetWindow() ) {  // Remove the View from it's Window if necessary
		pcWindow = m_pcView->GetWindow( );
		m_pcView->RemoveThis( );
	}
	delete m_pcView;  // Delete the View
	
	m_pcView = new LauncherBarView( m_zName, GetPrefs( ) );  // Recreate it.
	if( pcWindow != 0 )  // If the old View was attached to a Window send a Message asking the Window to refresh the display.
		m_pcPlugin->RequestWindowRefresh( );
}


/*
** name:       LauncherBar::HandleMessage
** purpose:    Process a message sent to the plugin.
** parameters: A LauncherMessage
** returns:    nothing.
*/
void LauncherBar::HandleMessage( LauncherMessage *pcMessage )
{
	string zNewPath;	
	int32 nCode = pcMessage->GetCode( );  // Get the message's code.

	switch( nCode )
	{
				
		case LW_WINDOW_PREFS_CHANGED:  // Parent window has changed it's settings.
			CloseSubWindow();
			SetPrefs( pcMessage );
			RecreateView( );
			break;			
			
		case LB_OPEN_SUB_WINDOW:       // A request to open the SubWindow.
			OpenSubWindow( pcMessage );
			break;
			
		case LW_PLUGIN_PREFS_BTN_OK:   // OK Clicked in PrefsWindow.
			m_zDirectory = convert_path(m_pcTVDirectory->GetBuffer()[0]);
			m_bLaunchBrowser = m_pcCBLaunchBrowser->GetValue( );
			m_bDrawBorder = m_pcCBDrawBorder->GetValue();
			RecreateView( );
			break;
		
		case LW_PLUGIN_PREFS_BTN_CANCEL:// Cancel clicked in PrefsWindow.
			RecreateView( );
			break;
			
		case LB_REMOVE_TIMER:          // Message to remove a SubWindow timeout.
			m_pcPlugin->RemoveTimer( );
			break;
					
		case LB_BUTTON_CLICKED:        // LauncherButton clicked.
			CloseSubWindow( );
			break;
			
		case LW_PLUGIN_TIMER:          // SubWindow timeout occurred.
			CloseSubWindow( );
			break;

		case LI_OPEN_TARGET_REQ:       // Open the target selection filerequestor
			m_pcFRTarget->Show( true );
			break;

		case LI_TARGET_CHANGED:        // New target selected in filerequestor
			pcMessage->FindString( "file/path", &zNewPath );
			m_pcTVDirectory->Set( zNewPath.c_str() );
			break;

		case LB_DIR_CHANGED:
			RecreateView();
			break;

		case LB_CLOSE_SUB_WINDOW:
			CloseSubWindow();
			break;
			
		default:                       
			break;			
			
	}
}



//**************************************************************************************/
LauncherBarView::LauncherBarView( string zName, LauncherMessage *pcPrefs, bool bNoMenu = false ) : LauncherView( zName, pcPrefs )
{

	m_pcPlugin = pcPrefs->GetPlugin( );                 // Get the pointer to the plugin.
 	m_pcNodeMonitor = NULL;
 
	if( bNoMenu == false ) {  // Create the Right-click popup menu.
	    m_pcMenu = new Menu( Rect(0,0,1,1), "PopUp", ITEMS_IN_COLUMN );
    	m_pcMenu->AddItem( "About LauncherBar", new LauncherMessage( LW_PLUGIN_OPEN_ABOUT, m_pcPlugin ) );
	   	m_pcMenu->AddItem( "Preferences", new LauncherMessage( LW_PLUGIN_OPEN_PREFS, m_pcPlugin ) );
    	m_pcMenu->AddItem( new MenuSeparator( ) );
	   	m_pcMenu->AddItem( "Move Up", new LauncherMessage( LW_PLUGIN_MOVE_UP, m_pcPlugin ) );
    	m_pcMenu->AddItem( "Move Down", new LauncherMessage( LW_PLUGIN_MOVE_DOWN, m_pcPlugin ) );
	   	m_pcMenu->AddItem( new MenuSeparator( ) );
   		m_pcMenu->AddItem( "Delete", new LauncherMessage( LW_PLUGIN_DELETE, m_pcPlugin ) );
   		Message cCloseMessage( LW_ZOOM_UNLOCK );
	    m_pcMenu->SetCloseMessage( cCloseMessage );
    } else m_pcMenu = 0;

}

LauncherBarView::~LauncherBarView( )
{
}



void LauncherBarView::AttachedToWindow( void )
{
	if( m_pcNodeMonitor == NULL ) {
		LauncherBarNodeMon *pcHandler = new LauncherBarNodeMon( GetWindow(), new LauncherMessage( LB_DIR_CHANGED, m_pcPlugin ) );
		GetWindow()->AddHandler( pcHandler );
		m_pcNodeMonitor = new NodeMonitor( convert_path( "~/config/Launcher" ), NWATCH_DIR, pcHandler );
	}
	LauncherView::AttachedToWindow();
}

/*
** name:       LauncherView::MouseDown
** purpose:    Called when the mouse is clicked on the View
** parameters: The position of the mouse and which mouse buttons are being held down.
** returns:    nothing.
*/
void LauncherBarView::MouseDown( const Point &cPosition, uint32 nButtons  )
{
	if( (nButtons == 2) && ( m_pcMenu != 0 ) ) {  // If it's a Right-click and we have a menu to display.

		m_pcMenu->SetCloseMsgTarget( GetWindow() ); 
    	m_pcMenu->SetTargetForItems( GetWindow() );
    	
	      // Lock the parent window.
		m_pcPlugin->LockWindow( );
		
		  // Open the Menu.
    	Point cPos = ConvertToScreen( cPosition );
	    Point cSize = m_pcMenu->GetPreferredSize( false );
    	Rect cFrame = frame_in_screen( Rect( cPos.x, cPos.y, cPos.x + cSize.x, cPos.y + cSize.y ) ); 
	    m_pcMenu->Open( Point( cFrame.left, cFrame.top) );
    } 
}


//**************************************************************************************/

LauncherBarNodeMon::LauncherBarNodeMon( Window *pcTarget, Message *pcMessage ) : Handler( "LauncherBarNodeMonitor" )
{
	m_pcTarget = pcTarget;
	m_pcMessage = pcMessage;
}


LauncherBarNodeMon::~LauncherBarNodeMon( )
{
}


void LauncherBarNodeMon::HandleMessage( Message *pcMessage )
{
	Invoker *pcInvoker = new Invoker( m_pcMessage, m_pcTarget );
	pcInvoker->Invoke();
}

//**************************************************************************************/
