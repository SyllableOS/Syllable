/*
    launcher_plugin - A plugin interface for the AtheOS Launcher
    Copyright (C) 2002 Andrew Kennan (bewilder@stresscow.org)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
    
    See ../../include/COPYING
*/


#include "../../include/launcher_bar.h"
#include "../../include/launcher_mime.h"

//*************************************************************************************
/*
** name:       LauncherView
** purpose:    A subclass of LayoutView. It displays the buttons.
** parameters: The plugin name, a LauncherMessage containing prefs. If bNoMenu is true no pop-up menu is created.
**
**************************************************************************************/
LauncherView::LauncherView( string zName, LauncherMessage *pcPrefs ) : LayoutView( Rect(), zName )
{
	m_bLaunchBrowser = false;
	
	pcPrefs->FindInt32( "Align",      &m_nAlign );	    // Get some prefs info
	pcPrefs->FindInt32( "HPos",	      &m_nHPos );
	pcPrefs->FindString( "Directory", &m_zDirectory );
	pcPrefs->FindBool( "LaunchBrowser", &m_bLaunchBrowser );
	pcPrefs->FindBool( "DrawBorder", &m_bDrawBorder );
	
	m_pcPlugin = pcPrefs->GetPlugin( );                 // Get the pointer to the plugin.
 
	CreateButtons( );  // Create the Buttons.
}

LauncherView::~LauncherView( )
{
}


/*
** name:       LauncherView::CreateButtons
** purpose:    Reads a directory and creates a button corresponding to each entry.
** parameters: none
** returns:    nothing.
*/
void LauncherView::CreateButtons( void )
{

	  // Create the LayoutNode
	m_pcRoot = (m_nAlign == LW_ALIGN_VERTICAL) ? new VLayoutNode( "Root" ) : new HLayoutNode( "Root" );

      // Create a spacer for sizing the buttons
    m_pcSpacer = new LayoutSpacer( "__SPACER", 100 );
    m_pcRoot->AddChild( m_pcSpacer );
	
	string zName;
	string zPath;
 
	Directory *pcDir = new Directory( );
    if( pcDir->SetTo( m_zDirectory ) == 0 ) {  // Read the Directory.
   		pcDir->GetPath( &zPath );
    	while( pcDir->GetNextEntry( &zName ) )
    	      // Do not include files starting with a "." or containing the word Launcher
        	if( ( zName.substr( 0, 1 ) != "." ) && ( zName.find( "Launcher",0 ) == string::npos  ) ) {   	
        	      // Create a button.
       			LauncherButton *pcButton = new LauncherButton( zName, zPath, new LauncherMessage( LB_BUTTON_CLICKED, m_pcPlugin ), (m_nHPos == LW_HPOS_LEFT)? IB_TEXT_LEFT : IB_TEXT_RIGHT, m_bDrawBorder, m_bLaunchBrowser );
       			m_zName.push_back( zName );  // Add it to the array of buttons as well as the LayoutNode.
       			m_pcButton.push_back( pcButton );
       			m_pcRoot->AddChild( pcButton );
       		}
    } else { // Couldn't read the directory so display the word ERROR.
    	dbprintf( "LauncherBar::CreateButtons - Failed to read directory %s\n", m_zDirectory.c_str() ); 
    	m_pcRoot->AddChild( new StringView( Rect(), "ERROR", "ERROR!", ALIGN_CENTER ) );	
    }

	delete pcDir;
	
	SetRoot( m_pcRoot );
}


/*
** name:       LauncherView::AttachedToWindow
** purpose:    Resize the buttons so the match the width or height of the LauncherWindow.
** parameters: none.
** returns:    nothing.
*/
void LauncherView::AttachedToWindow( void )
{
	Window *pcWindow = GetWindow();
	    
      // Get the size of the Window
	Rect cWinSize = pcWindow->GetFrame( );
	
	  // vBiggest is the size all the buttons will be set to.
	float vBiggest = (m_nAlign == LW_ALIGN_VERTICAL) ? cWinSize.Width() : cWinSize.Height( );

	if( m_pcButton.size( ) > 0 )  // If there are some buttons.
		for( uint32 n = 0; n < m_pcButton.size( ); n++ ) {
			m_pcButton[n]->SetTarget( pcWindow ); // Set their targets to the LauncherWindow.
			
			  // Find the widest/tallest.
			float vSize = (m_nAlign == LW_ALIGN_VERTICAL) ? m_pcButton[n]->GetPreferredSize( false ).x : m_pcButton[n]->GetPreferredSize( false ).y;
			if(  vSize > vBiggest )
				vBiggest = vSize;
		}

      // Set the size of the spacer
	m_pcSpacer->SetMinSize( (m_nAlign == LW_ALIGN_VERTICAL) ? Point( vBiggest, 0) : Point( 0, vBiggest ) );

      // Resize each button.
	if( m_pcButton.size( ) > 0 )
		for( uint32 n = 0; n < m_pcButton.size( ); n++ )
			(m_nAlign == LW_ALIGN_VERTICAL) ? m_pcRoot->SameWidth( "__SPACER", m_zName[n].c_str(), NULL ) : m_pcRoot->SameHeight( "__SPACER", m_zName[n].c_str(), NULL );

      // Resize the View.
	ResizeTo( m_pcRoot->GetPreferredSize( false ) );
}




//*************************************************************************************
/*
** name:       LauncherButton
** purpose:    A Button that launches other programs.
** parameters: The Label of the button, the directory the program lives in, a message to return to the Window.
*/
//*************************************************************************************
LauncherButton::LauncherButton( string zLabel, string zDirectory, LauncherMessage *pcMessage, uint32 nTextPos, bool bDrawBorder, bool bLaunchBrowser ) : 
		ImageButton( Rect(), zLabel.c_str(), zLabel.c_str(), pcMessage, nTextPos, bDrawBorder )
{
	m_zLabel = zLabel;
	m_zDirectory = zDirectory;
	string zCommand = m_zDirectory + (string)"/" + m_zLabel;
	m_bLaunchBrowser = bLaunchBrowser;
 	m_zDraggedFile = "";
	m_bDropped = false;
	
	SetImageFromFile( get_icon( zCommand ).c_str() );
}

LauncherButton::~LauncherButton( )
{
	
}


/*
** name:       LauncherButton::Invoked
** purpose:    Called when the button is clicked.
** parameters: The message being returned to the Window
** returns:    true if the message is to be sent.
*/
bool LauncherButton::Invoked( Message *pcMessage )
{

	LauncherMessage *pcMessage2 = (LauncherMessage *)pcMessage;
	
	struct stat psStat;

	string zCommand = m_zDirectory + (string)"/" + m_zLabel;

    // Stat the file

	stat( zCommand.c_str(), &psStat );

	// Is it a directory?
	if( S_ISDIR( psStat.st_mode ) )
	{  

		if( m_bLaunchBrowser ) {
			pid_t nPid = fork();
			
			if( nPid == 0 )
			{	    
				set_thread_priority( -1, 0 );
				execlp("/Applications/Launcher/bin/LBrowser", "LBrowser", zCommand.c_str(), NULL );
				exit( 1 );
			}
		} else {
				
	  		// Add some extra info to the message.
			pcMessage2->AddString( "Directory", zCommand );
	
			Rect cWinFrame = GetWindow()->GetFrame();
			Rect cParentFrame = GetParent()->GetFrame();
			Rect cFrame = GetFrame();
			cFrame.left += cParentFrame.left + cWinFrame.left;  // Why isn't ConvertToScreen working here? grrr.
			cFrame.right += cParentFrame.left + cWinFrame.left;
			cFrame.top += cParentFrame.top + cWinFrame.top;
			cFrame.bottom += cParentFrame.top + cWinFrame.top;
	
			pcMessage2->AddPoint( "Position", Point( cFrame.left + (cFrame.Width() / 2), cFrame.top + (cFrame.Height() / 2) ) );
	
			pcMessage2->SetCode( LB_OPEN_SUB_WINDOW );  // Send a message to open the SubWindow
		}

	} else {

		MimeNode *pcNode = new MimeNode( zCommand );
		
		if( pcNode->GetMimeType() != "application/octet-stream" ) {
			pcNode->Launch();
		} else {	
			  // Fork and execute the file.
			
			pid_t nPid = fork();
			
			if( nPid == 0 )
			{	    
				set_thread_priority( -1, 0 );
				if( m_bDropped == true) {
					execlp(zCommand.c_str(), m_zLabel.c_str(), m_zDraggedFile.c_str(), NULL );
					m_bDropped = false;
				 	m_zDraggedFile = "";
				} else {
					execlp(zCommand.c_str(), m_zLabel.c_str(), NULL );
				}
				exit( 1 );
			}
		}
		
	}
 
	return true;

}


void LauncherButton::MouseUp( const Point &cPosition, uint32 nButtons, Message *pcData )
{
	if( pcData ) {
		pcData->FindString( "file/path", &m_zDraggedFile );
		if( m_zDraggedFile != "" ) {
			m_bDropped = true;
			Invoke();
		}
	} else {	
		Button::MouseUp( cPosition, nButtons, pcData );
	}
}

//*************************************************************************************
/*
** name:       LauncherSubWindow
** purpose:    Displays a subdirectory containing a LauncherView
** parameters: The window name and a prefs message.
*/
//*************************************************************************************
LauncherSubWindow::LauncherSubWindow( string zName, LauncherMessage *pcPrefs ) : Window( Rect(0,0,0,0), zName.c_str(), zName.c_str(), WND_NOT_RESIZABLE | WND_NO_BORDER )
{

	pcPrefs->FindInt32( "Align",      &m_nAlign );  // Get the prefs.
	pcPrefs->FindInt32( "HPos",       &m_nHPos );
	pcPrefs->FindInt32( "VPos",       &m_nVPos );
	pcPrefs->FindInt32( "WindowPos",  &m_nWindowPos );
	pcPrefs->FindBool( "DrawBorder", &m_bDrawBorder );
	m_pcSubWindow = 0;
	
	pcPrefs->FindPointer( "ParentWindow", (void **)&m_pcParentWindow );
	Rect cParentFrame = m_pcParentWindow->GetFrame( );
	
	Point cButtonPos;
	pcPrefs->FindPoint( "Position", &cButtonPos );  // The location of the button that was clicked to open this window.
	
	m_pcPlugin = pcPrefs->GetPlugin( );
	
	pcPrefs->RemoveName( "Align" );  // All the SubWindow's Views are aligned vertically
	pcPrefs->AddInt32( "Align", LW_ALIGN_VERTICAL );
	m_pcView = new LauncherView( "LauncherSubWindowView", pcPrefs );  // Create the View
	
	Point cViewSize = m_pcView->GetPreferredSize( false );  // How big does the View want to be?
	
	float vX, vY;
	
	if( m_nAlign == LW_ALIGN_VERTICAL ) {  // Position the window relative to the button that opened it.
		vY = cButtonPos.y - ( cViewSize.y / 2 );
		vX = (m_nHPos == LW_HPOS_LEFT) ? cParentFrame.right : cParentFrame.left - cViewSize.x;
	} else {
		vX = cButtonPos.x - ( cViewSize.x / 2 );
		vY = (m_nVPos == LW_VPOS_TOP) ? cParentFrame.bottom : cParentFrame.top - cViewSize.y;
	}
	
	SetFrame( frame_in_screen( Rect( vX, vY, vX + cViewSize.x, vY + cViewSize.y ) ) );  // Resize the Window and View.
	m_pcView->SetFrame( Rect( 0,0, cViewSize.x, cViewSize.y ) );
	AddChild( m_pcView );
}

LauncherSubWindow::~LauncherSubWindow( )
{
	CloseSubWindow( );
}



/*
** name:       LauncherSubWindow::HandleMessage
** purpose:    Process messages
** parameters: A Message
** returns:    nothing.
*/
void LauncherSubWindow::HandleMessage( Message *pcMessage )
{
	
	Invoker *pcParentInvoker;
	int32 nCode = pcMessage->GetCode( );
	
	switch( nCode )
	{
		
		case LB_OPEN_SUB_WINDOW:                 // Request to open the SubWindow.
			OpenSubWindow( (LauncherMessage *)pcMessage );
			break;
			
		case LB_REMOVE_TIMER:                    // Remvoe the SubWindow inactivity timeout.
			RemoveTimer( this, LW_PLUGIN_TIMER );
			break;
			
		default:								 // All other messages sent back to the parent.
			pcParentInvoker = new Invoker( new LauncherMessage( nCode, m_pcPlugin), m_pcParentWindow );
			pcParentInvoker->Invoke();
			break;
	}
	
}



/*
** name:       LauncherSubWindow::OpenSubWindow
** purpose:    Opens the subwindow
** parameters: Message containing prefs
** returns:    nothing.
*/
void LauncherSubWindow::OpenSubWindow( LauncherMessage *pcMessage )
{
	
	CloseSubWindow( );  // Close an existing SubWindow.
	
	pcMessage->AddInt32( "Align",      LW_ALIGN_VERTICAL );  // Add prefs info to the message
	pcMessage->AddInt32( "HPos",       m_nHPos );
	pcMessage->AddInt32( "VPos",       m_nVPos );
	pcMessage->AddInt32( "WindowPos",  m_nWindowPos );
	pcMessage->AddPointer( "ParentWindow", (void **)this );
	pcMessage->AddBool( "DrawBorder", m_bDrawBorder );
	
	m_pcSubWindow = new LauncherSubWindow( "LauncherSubWindow", pcMessage );  // Create the window
	m_pcSubWindow->Show( );
	
	  // Remove the inactivity timeout from the parent window
	Invoker *pcParentInvoker = new Invoker( new LauncherMessage( LB_REMOVE_TIMER, m_pcPlugin), m_pcParentWindow );
	pcParentInvoker->Invoke();
	
	  // Add a new inactivity timeout.
	AddTimer( this, LW_PLUGIN_TIMER, 5000000, false );
}



/*
** name:       LauncherSubWindow::CloseSubWindow
** purpose:    Closes the SubWindow
** parameters: none
** returns:    nothing
*/
void LauncherSubWindow::CloseSubWindow( void )
{
	
	if( m_pcSubWindow ) { // Close the Window.
		m_pcSubWindow->Quit( );
		m_pcSubWindow = 0;
	}
	
}



/*
** name:       LauncherSubWindow::TimerTick
** purpose:    Called when the inactivity timeout occurs.
** parameters: the ID of the timer.
** returns:    nothing.
*/
void LauncherSubWindow::TimerTick( int nId )
{
	  // Send a LW_PLUGIN_INVOKED message to the parent. It will cause any SubWindows to be closed.
	Invoker *pcParentInvoker = new Invoker( new LauncherMessage( LW_PLUGIN_INVOKED, m_pcPlugin), m_pcParentWindow );
	pcParentInvoker->Invoke();
}


//*************************************************************************************

