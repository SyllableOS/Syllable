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

	Name : LauncherIcon
	Version : 1.1
	Author	: Andrew Kennan
	
	A Launcher plugin which displays an icon which, when clicked,
	will either launcher a program or open a LauncherSubWindow.
	
************************************************************/

#include "../../include/launcher_plugin.h"
#include "../../include/launcher_bar.h"
#include "../../include/launcher_mime.h"

#include <gui/stringview.h>
#include <gui/textview.h>
#include <gui/menu.h>
#include <gui/filerequester.h>
#include <gui/checkbox.h>
#include <storage/directory.h>

// For the About message.                                                                                                                                            
#define PLUGIN_NAME    "Launcher Icon"
#define PLUGIN_VERSION "1.1"
#define PLUGIN_AUTHOR  "Andrew Kennan"
#define PLUGIN_DESC    "An Icon Launcher"

#define LI_OPEN_TARGET_REQ 20001  
#define LI_OPEN_ICON_REQ   20002
#define LI_TARGET_CHANGED  20003
#define LI_ICON_CHANGED    20004
#define LI_OPEN_ICONED     20005
#define LI_LAUNCHBROWSER_CHANGED 20006
                                                                                                                                                                                                                                      
using namespace os;

class LauncherIcon;         // The main plugin class


std::vector<LauncherIcon *>g_apcLauncherIcon; // A list of plugins.
uint32 g_nOldSize = 0;
LauncherMessage *g_pcInfoMessage;


class LauncherIcon : public ImageButton
{
	public:
		LauncherIcon( string zName, LauncherMessage *pcPrefs );
		~LauncherIcon( );
		View *GetPrefsView( void );
		void RemoveView( void );
		LauncherMessage *GetPrefs( void );
		void SetPrefs( LauncherMessage *pcPrefs, bool bCreating = true );
		void HandleMessage( LauncherMessage *pcMessage );
		void MouseDown( const Point &cPosition, uint32 nButtons );
		virtual bool Invoked( Message *pcMessage );
		
	private:
		int32 m_nAlign;
		int32 m_nVPos;
		int32 m_nHPos;
		string m_zLabel;
		string m_zIcon;
		string m_zTarget;
		string m_zOriginalTarget;
		string m_zName;
		bool m_bLaunchBrowser;
		bool m_bDrawBorder;
						
		LauncherPlugin *m_pcPlugin;
		Menu *m_pcMenu;
		LauncherSubWindow *m_pcSubWindow;
			
		LayoutView *m_pcPrefsLayout;
		TextView *m_pcTVTarget;
		TextView *m_pcTVLabel;
		TextView *m_pcTVIcon;
		ImageButton *m_pcBrowseIcon;
		FileRequester *m_pcFRTarget;
		FileRequester *m_pcFRIcon;
		CheckBox *m_pcCBLaunchBrowser;
		CheckBox *m_pcCBDrawBorder;		

		void OpenSubWindow( void );
		void CloseSubWindow( void );
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
    string zName = (string)"LauncherIcon_" + (string)zID;
    g_apcLauncherIcon.push_back( new LauncherIcon( zName, pcPrefs ) );
    
    if( g_apcLauncherIcon.size( ) <= g_nOldSize )
      return INIT_FAIL;
      
    g_nOldSize = g_apcLauncherIcon.size( );

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
	if( g_apcLauncherIcon[nId] )
    	delete g_apcLauncherIcon[nId];
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
	return g_apcLauncherIcon[nId];
}


/*
** name:       remove_view
** purpose:    Instructs a plugin to remove it's View from the window.
** parameters: The ID number of the plugin.
** returns:    nothing.
*/
void remove_view( int nId )
{
	g_apcLauncherIcon[nId]->RemoveView( );
}


/*
** name:       get_prefs
** purpose:    For getting the Preferences of the plugin.
** parameters: The ID number of the plugin.
** returns:    A pointer to a LauncherMessage.
*/
LauncherMessage *get_prefs( int nId )
{
	return g_apcLauncherIcon[nId]->GetPrefs( );
}


/*
** name:       set_prefs
** purpose:    Sets the preferences of the plugin.
** parameters: The plugin's ID and a LauncherMessage containing prefs info.
** returns:    nothing.
*/
void set_prefs( int nId, LauncherMessage *pcPrefs )
{
	g_apcLauncherIcon[nId]->SetPrefs( pcPrefs );
}



/*
** name:       handle_message
** purpose:    Passes messages to the plugin.
** parameters: The plugin's ID and a LauncherMessage.
** returns:    nothing.
*/
void handle_message( int nId, LauncherMessage *pcMessage )
{
	g_apcLauncherIcon[nId]->HandleMessage( pcMessage );
}


/*
** name:       get_prefs_view
** purpose:    Returns a View* to be displayed in the PrefsWindow
** parameters: The ID number of the plugin.
** returns:    A pointer to the plugin's Prefs View.
*/
View *get_prefs_view( int nId )
{
	return g_apcLauncherIcon[nId]->GetPrefsView( );
}

//*************************************************************************************
/*
** name:       LauncherIcon
** purpose:    The core class of the LauncherIcon plugin.
** parameters: The name of the plugin and a LauncherMessage containing prefs info.
**
**************************************************************************************/
LauncherIcon::LauncherIcon( string zName, LauncherMessage *pcPrefs ) : ImageButton( Rect( ), zName.c_str(), "", new Message( LW_PLUGIN_INVOKED ) )
{
	m_zName       = zName;  // Get the name of the plugin.
	m_zOriginalTarget     = "~/config/Launcher"; // Default directory to display.
	m_zLabel      = ""; // Default label;
	m_zIcon       = "";
	m_bLaunchBrowser = false;
	m_nAlign      = LW_ALIGN_VERTICAL;   // Default values.
	m_nHPos       = LW_HPOS_LEFT;
	m_nVPos       = LW_VPOS_TOP;
	
	m_pcSubWindow   = 0;  // SubWindow closed.
	
	SetPrefs( pcPrefs );  // Retrieve prefs info.

	SetImageFromFile( m_zIcon.c_str() );  // Set the icon if possible.
		
	SetLabel( m_zLabel.c_str() );

	VLayoutNode *pcPrefsRoot = new VLayoutNode( "li_prefs_root" );
	HLayoutNode *pcPrefsLine1 = new HLayoutNode( "li_prefs_line1" );
	HLayoutNode *pcPrefsLine2 = new HLayoutNode( "li_prefs_line2" );
	HLayoutNode *pcPrefsLine3 = new HLayoutNode( "li_prefs_line3" );
	HLayoutNode *pcPrefsLine4 = new HLayoutNode( "li_prefs_line4" );
	HLayoutNode *pcPrefsLine5 = new HLayoutNode( "li_prefs_line5" );
	
	m_pcTVTarget = new TextView( Rect(), "tv_target", m_zTarget.c_str() );
	m_pcTVLabel = new TextView( Rect(), "tv_label", m_zLabel.c_str() );
	m_pcTVIcon = new TextView( Rect(), "tv_icon", m_zIcon.c_str() );
	m_pcCBLaunchBrowser = new CheckBox( Rect(), "cb_lbrowse", "Launch LBrowse On Directories", new LauncherMessage( LI_LAUNCHBROWSER_CHANGED, m_pcPlugin ) );
	m_pcCBDrawBorder = new CheckBox( Rect(), "cb_border", "Draw Borders Around Buttons", new LauncherMessage( LI_LAUNCHBROWSER_CHANGED, m_pcPlugin ) );
	ImageButton *pcBrowseTarget = new ImageButton( Rect(), "btn_fr_0", "", new LauncherMessage( LI_OPEN_TARGET_REQ, m_pcPlugin )); 
	
	m_pcBrowseIcon = new ImageButton( Rect(), "btn_fr_1", "Icon", new LauncherMessage( LI_OPEN_ICON_REQ, m_pcPlugin ) );
	string zIconPath = get_launcher_path() + (string)"icons/folder.png";
	m_pcBrowseIcon->SetImageFromFile( zIconPath.c_str() );
	pcBrowseTarget->SetImageFromFile( zIconPath.c_str() );
	
	pcPrefsLine1->AddChild( new VLayoutSpacer( "spacer_0", 1.0f ) );
	pcPrefsLine1->AddChild( new StringView( Rect(), "sv_label_0", "Target" ) );
	pcPrefsLine1->AddChild( m_pcTVTarget );
	pcPrefsLine1->AddChild( pcBrowseTarget );
		
	pcPrefsLine2->AddChild( new VLayoutSpacer( "spacer_1", 1.0f ) );
	pcPrefsLine2->AddChild( new StringView( Rect(), "sv_label_1", "Label" ) );
	pcPrefsLine2->AddChild( m_pcTVLabel );
	
	pcPrefsLine3->AddChild( new VLayoutSpacer( "spacer_0", 1.0f ) );
	pcPrefsLine3->AddChild( m_pcBrowseIcon );
	
	pcPrefsLine4->AddChild( new VLayoutSpacer( "spacer_2", 1.0f ) );
	pcPrefsLine4->AddChild( m_pcCBLaunchBrowser );

	pcPrefsLine5->AddChild( new VLayoutSpacer( "spacer_5", 1.0f ) );
	pcPrefsLine5->AddChild( m_pcCBDrawBorder );
		
	pcPrefsRoot->AddChild( pcPrefsLine1 );
	pcPrefsRoot->AddChild( pcPrefsLine2 );
	pcPrefsRoot->AddChild( pcPrefsLine3 );
	pcPrefsRoot->AddChild( pcPrefsLine4 );
	pcPrefsRoot->AddChild( pcPrefsLine5 );
	
	m_pcPrefsLayout = new LayoutView( Rect(), "lb_prefs_layout" );
	m_pcPrefsLayout->SetRoot( pcPrefsRoot );
	
    m_pcMenu = new Menu( Rect(), "PopUp", ITEMS_IN_COLUMN );
	m_pcMenu->AddItem( "About LauncherIcon", new LauncherMessage( LW_PLUGIN_OPEN_ABOUT, m_pcPlugin ) );
   	m_pcMenu->AddItem( "Preferences", new LauncherMessage( LW_PLUGIN_OPEN_PREFS, m_pcPlugin ) );
	m_pcMenu->AddItem( new MenuSeparator( ) );
   	m_pcMenu->AddItem( "Move Up", new LauncherMessage( LW_PLUGIN_MOVE_UP, m_pcPlugin ) );
	m_pcMenu->AddItem( "Move Down", new LauncherMessage( LW_PLUGIN_MOVE_DOWN, m_pcPlugin ) );
   	m_pcMenu->AddItem( new MenuSeparator( ) );
	m_pcMenu->AddItem( "Delete", new LauncherMessage( LW_PLUGIN_DELETE, m_pcPlugin ) );
	Message cCloseMessage( LW_ZOOM_UNLOCK );
    m_pcMenu->SetCloseMessage( cCloseMessage );	
	
	m_pcFRTarget = NULL;
	m_pcFRIcon = NULL;
	
	if( m_pcPlugin->GetWindow() ) {
//		m_pcFRTarget = new FileRequester( FileRequester::LOAD_REQ, new Messenger(m_pcPlugin->GetWindow()), getenv("$HOME"),FileRequester::NODE_FILE,false,new LauncherMessage( LI_TARGET_CHANGED, m_pcPlugin ) );
//		m_pcFRIcon = new FileRequester( FileRequester::LOAD_REQ, new Messenger(m_pcPlugin->GetWindow()), getenv("$HOME"),FileRequester::NODE_FILE,false,new LauncherMessage( LI_ICON_CHANGED, m_pcPlugin ) );
		pcBrowseTarget->SetTarget( m_pcPlugin->GetWindow( ) );
		m_pcBrowseIcon->SetTarget( m_pcPlugin->GetWindow( ) );
	}
}


LauncherIcon::~LauncherIcon( )
{
	CloseSubWindow( );  // Close the SubWindow
	if( m_pcFRTarget != NULL ) m_pcFRTarget->Quit();
	if( m_pcFRIcon != NULL ) m_pcFRIcon->Quit();
}


/*
** name:       LauncherBar::SetPrefs
** purpose:    Set the preferences info for the plugin.
** parameters: A LauncherMessage containing prefs info.
** returns:    nothing.
*/
void LauncherIcon::SetPrefs( LauncherMessage *pcPrefs, bool bCreating = true )
{
	pcPrefs->FindInt32( "Align",      &m_nAlign );
	pcPrefs->FindInt32( "HPos",       &m_nHPos );
	pcPrefs->FindInt32( "VPos",       &m_nVPos );
	pcPrefs->FindBool( "LaunchBrowser", &m_bLaunchBrowser );
	bool bDrawBorder = GetDrawBorder();
	pcPrefs->FindBool( "DrawBorder", &bDrawBorder );
	pcPrefs->FindString( "Target", &m_zOriginalTarget );
	m_zTarget = convert_path( m_zOriginalTarget );
	int nFoundLabel = pcPrefs->FindString( "Label", &m_zLabel );
	int nFoundIcon = pcPrefs->FindString( "Icon", &m_zIcon );		
	
	if( pcPrefs->GetPlugin() )
		m_pcPlugin = pcPrefs->GetPlugin( );

	if( m_zIcon.length() == 0 ) {
		MimeInfo *pcMimeInfo = new MimeInfo();
		if( pcMimeInfo->FindMimeType( m_zTarget ) == 0 )
			m_zIcon = pcMimeInfo->GetDefaultIcon( );
	}
		

	if( bCreating )
		if( ( nFoundIcon != 0 ) && ( nFoundLabel != 0 ) && (m_pcPlugin->GetWindow() )) {
			Invoker *pcInvoker = new Invoker( new LauncherMessage( LW_PLUGIN_OPEN_PREFS, m_pcPlugin ), m_pcPlugin->GetWindow() );
			pcInvoker->Invoke();
		}

	SetTextPosition((m_nHPos == LW_HPOS_LEFT)? IB_TEXT_LEFT : IB_TEXT_RIGHT);
	SetDrawBorder( bDrawBorder );
}


/*
** name:       LauncherBar::GetPrefs
** purpose:    For retrieving the Prefs info of the plugin.
** parameters: none.
** returns:    A LauncherMessage containing prefs info.
*/
LauncherMessage *LauncherIcon::GetPrefs( void )
{
	LauncherMessage *pcPrefs = new LauncherMessage( );

	pcPrefs->AddInt32( "Align",      m_nAlign );
	pcPrefs->AddInt32( "HPos",       m_nHPos );
	pcPrefs->AddInt32( "VPos",       m_nVPos );
	
	pcPrefs->AddString( "Target", m_zOriginalTarget);
	pcPrefs->AddString( "Label", m_zLabel);
	pcPrefs->AddString( "Icon", m_zIcon);		
	pcPrefs->AddBool(   "LaunchBrowser", m_bLaunchBrowser );
	pcPrefs->AddBool( "DrawBorder", GetDrawBorder() );
	
    pcPrefs->SetPlugin( m_pcPlugin );

	return pcPrefs;
}




/*
** name:       LauncherBar::GetPrefsView
** purpose:    Gets the plugin's prefs View for display in the Prefs window
** parameters: none
** returns:    A pointer to the Prefs view
*/
View *LauncherIcon::GetPrefsView( void )
{
	m_pcTVTarget->Set( m_zOriginalTarget.c_str( ) );
	m_pcTVLabel->Set( m_zLabel.c_str( ) );
	m_pcTVIcon->Set( m_zIcon.c_str( ) );
	m_pcCBLaunchBrowser->SetValue( m_bLaunchBrowser );
	m_pcCBDrawBorder->SetValue( GetDrawBorder() );
	
	if( m_zIcon.length() > 0 )
		m_pcBrowseIcon->SetImageFromFile( m_zIcon.c_str() );
	return m_pcPrefsLayout;
}


/*
** name:       LauncherBar::RemoveView
** purpose:    Removes the LauncherView from it's window
** parameters: none
** returns:    nothing
*/
void LauncherIcon::RemoveView( void )
{
	  
	CloseSubWindow();  // Close the SubWindow
	RemoveThis( );  // Remove the View
	
}

	
/*
** name:       LauncherBar::OpenSubWindow
** purpose:    Opens a LauncherSubWindow
** parameters: A LauncherMessage containing the directory to list and the position of the parent button.
** returns:    nothing.
*/
void LauncherIcon::OpenSubWindow( void )
{
	if( m_pcSubWindow ) {  // Close the SubWindow if it is already open
		m_pcSubWindow->Quit( );
		m_pcSubWindow = 0;
	}
	
	  // Instruct other LauncherBar type plugins to close their sub-windows
	m_pcPlugin->BroadcastMessage( LB_CLOSE_SUB_WINDOW );	
	
	  // Lock the LauncherWindow
	m_pcPlugin->LockWindow( );
	
	LauncherMessage *pcMessage = new LauncherMessage( 0,m_pcPlugin );
	
	  // Add extra positioning info to the Message
	pcMessage->AddInt32( "Align",      m_nAlign );
	pcMessage->AddInt32( "HPos",       m_nHPos );
	pcMessage->AddInt32( "VPos",       m_nVPos );	
	pcMessage->AddPointer( "ParentWindow", (void **)GetWindow( ) );  // Used to send messages back to the parent.
	pcMessage->AddString( "Directory", m_zTarget );
	pcMessage->AddBool(   "LaunchBrowser", m_bLaunchBrowser );
		
	Rect cWinFrame = GetWindow()->GetFrame();
	Rect cParentFrame = GetParent()->GetFrame();
	Rect cFrame = GetFrame();
	cFrame.left += cParentFrame.left + cWinFrame.left;  // Why isn't ConvertToScreen working here? grrr.
	cFrame.right += cParentFrame.left + cWinFrame.left;
	cFrame.top += cParentFrame.top + cWinFrame.top;
	cFrame.bottom += cParentFrame.top + cWinFrame.top;

	pcMessage->AddPoint( "Position", Point( cFrame.left + (cFrame.Width() / 2), cFrame.top + (cFrame.Height() / 2) ) );
	
	
	  // Create the SubWindow
	m_pcSubWindow = new LauncherSubWindow( "LauncherSubWindow", pcMessage );
	m_pcSubWindow->Show( );
	
	  // Timeout if there is no action taken within 5 seconds.
	m_pcPlugin->AddTimer( 5000000, true );
}


/*
** name:       LauncherBar::CloseSubWindow
** purpose:    Closes the SubWindow
** parameters: none
** returns:    nothing.
*/
void LauncherIcon::CloseSubWindow( void )
{
	m_pcPlugin->RemoveTimer( );  // Remove the timeout

	if( m_pcSubWindow ) {  // If the SubWindow is open
		m_pcSubWindow->Quit( );  // Close the Window
		m_pcSubWindow = 0;
		m_pcPlugin->UnlockWindow( );
	}
}




/*
** name:       LauncherBar::HandleMessage
** purpose:    Process a message sent to the plugin.
** parameters: A LauncherMessage
** returns:    nothing.
*/
void LauncherIcon::HandleMessage( LauncherMessage *pcMessage )
{
//	Invoker *pcParentInvoker;  // An invoker for sending messages to the parent window.
	
	int32 nCode = pcMessage->GetCode( );  // Get the message's code.
	string zNewPath;
	
	switch( nCode )
	{
				
		case LW_WINDOW_PREFS_CHANGED:  // Parent window has changed it's settings.
			CloseSubWindow();
			SetPrefs( pcMessage, false );
			break;			
					
		case LW_PLUGIN_PREFS_BTN_OK:   // OK Clicked in PrefsWindow.
			m_zOriginalTarget = (string)m_pcTVTarget->GetBuffer()[0];
			m_zTarget = convert_path( m_zOriginalTarget );
			m_zLabel = (string)m_pcTVLabel->GetBuffer()[0];
			m_zIcon = convert_path((string)m_pcTVIcon->GetBuffer()[0]);
			m_bLaunchBrowser = m_pcCBLaunchBrowser->GetValue( );
			SetDrawBorder( m_pcCBDrawBorder->GetValue() );
			SetLabel( m_zLabel.c_str() );
			SetImageFromFile( m_zIcon.c_str() );
			break;
		
		case LW_PLUGIN_PREFS_BTN_CANCEL:// Cancel clicked in PrefsWindow.
			break;
			
		case LB_REMOVE_TIMER:          // Message to remove a SubWindow timeout.
			m_pcPlugin->RemoveTimer( );
			break;
					
		case LW_PLUGIN_INVOKED:        // LauncherButton clicked.
			CloseSubWindow( );
			break;
			
		case LW_PLUGIN_TIMER:          // SubWindow timeout occurred.
			CloseSubWindow( );
			break;
			
		case LI_OPEN_TARGET_REQ:
			if( m_pcFRTarget == NULL )
				m_pcFRTarget = new FileRequester( FileRequester::LOAD_REQ, new Messenger(m_pcPlugin->GetWindow()), getenv("$HOME"),FileRequester::NODE_FILE,false,new LauncherMessage( LI_TARGET_CHANGED, m_pcPlugin ) );
			m_pcFRTarget->Show( true );
			break;

		case LI_OPEN_ICON_REQ:
			if( m_pcFRIcon == NULL )
				m_pcFRIcon = new FileRequester( FileRequester::LOAD_REQ, new Messenger(m_pcPlugin->GetWindow()), getenv("$HOME"),FileRequester::NODE_FILE,false,new LauncherMessage( LI_ICON_CHANGED, m_pcPlugin ) );			
			m_pcFRIcon->Show( true );
			break;
			
		case LI_ICON_CHANGED:
			pcMessage->FindString( "file/path", &zNewPath );
			m_pcTVIcon->Set( zNewPath.c_str() );
			m_pcBrowseIcon->SetImageFromFile( convert_path(zNewPath).c_str() );			
			break;

		case LI_TARGET_CHANGED:
			pcMessage->FindString( "file/path", &zNewPath );
			m_pcTVTarget->Set( convert_path(zNewPath).c_str() );
			break;

		case LB_CLOSE_SUB_WINDOW:
			CloseSubWindow();
			break;

		default:
			break;			
			
	}
}


/*
** name:       LauncherIcon::MouseDown
** purpose:    Called when the mouse is clicked on the Button
** parameters: The position of the mouse and which mouse buttons are being held down.
** returns:    nothing.
*/
void LauncherIcon::MouseDown( const Point &cPosition, uint32 nButtons  )
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
    } else {
    	ImageButton::MouseDown( cPosition, nButtons );
    }
}



/*
** name:       LauncherButton::Invoked
** purpose:    Called when the button is clicked.
** parameters: The message being returned to the Window
** returns:    true if the message is to be sent.
*/
bool LauncherIcon::Invoked( Message *pcMessage )
{

	struct stat psStat;
	MimeNode *pcNode = new MimeNode( );
	
	if( pcNode->SetTo( convert_path( m_zTarget ) ) == 0 ) {

		pcNode->GetStat( &psStat );
		// Is it a directory?
		if( S_ISDIR( psStat.st_mode ) ) {
			OpenSubWindow( );
			return false;
		} else {
			pcNode->Launch();
			m_pcPlugin->BroadcastMessage( LB_CLOSE_SUB_WINDOW );
		}
	}
	
	return true;
}


