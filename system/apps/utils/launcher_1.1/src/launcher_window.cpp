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

#include "launcher_window.h"
#include "icons.h"

LauncherWindow::LauncherWindow( Message *pcPrefs, bool bOpenPrefsWindow = false ) 
  : Window( Rect(0,0,10,10 ), "Launcher", "Launcher", WND_NOT_RESIZABLE | WND_NO_BORDER )
{

  m_nWindowId = pcPrefs->GetCode( );

  m_nVPos = LW_VPOS_TOP;
  m_nHPos = LW_HPOS_LEFT;
  m_nAlign = LW_ALIGN_VERTICAL;
  m_bAutoHide = false;
  m_bAutoShow = false;
  m_bVisible = true;

  m_bZoomLocked = false;

  m_pcArrow = new ArrowButton( AB_DIR_UP, "ArrowButton", new LauncherMessage( LW_EVENT_ZOOM ) );
  m_pcLayout = NULL;
  m_pcRoot = NULL;

  if( pcPrefs != NULL )
    SetPrefs( pcPrefs );

  PositionWindow( );
  
  AddTimer( this, LW_CHECK_MOUSE, 100000, false );
  
  if( bOpenPrefsWindow )
    m_pcPluginListWindow = new PrefWindow( this );

}


LauncherWindow::~LauncherWindow( )
{
}


bool LauncherWindow::OkToQuit( void )
{
  DeletePlugins( );
  return true;
}

void LauncherWindow::HandleMessage( Message *pcMessage )
{

  LauncherMessage *pcMessage2 = (LauncherMessage *)pcMessage;
  
  int nId = pcMessage2->GetCode( );

//  printf( "%d\n", nId );

  int32 nPos = 0;
  pcMessage->FindInt32( "POS", &nPos );
  
  switch( nId )
  {

    case LW_ZOOM_LOCK:
      m_bZoomLocked = true;
      break;

    case LW_ZOOM_UNLOCK:
      m_bZoomLocked = false;
      break;

    case LW_EVENT_PREFS:
      m_pcPluginListWindow = new PrefWindow( this );
      break;
      
    case LW_EVENT_ADD_WINDOW:
      Application::GetInstance( )->PostMessage( pcMessage );
      break;
      
    case LW_EVENT_CLOSE_WINDOW:
      pcMessage->AddInt32( "ID", m_nWindowId );
      Application::GetInstance( )->PostMessage( pcMessage );
      break;

    case LW_PREFS_WINDOW_CLOSED:
      m_pcPluginListWindow = NULL;
      break;

    case LW_CREATE_PLUGINS:
      InsertPlugins( (LauncherMessage *)pcMessage );
      PositionWindow( );
	  Application::GetInstance( )->PostMessage( new Message( LW_SAVE_PREFS ) );      
      break;

    case LW_EVENT_ZOOM:
	  PositionWindow( true );
      break;

    case LW_EVENT_QUIT:
      Application::GetInstance( )->PostMessage( new Message( M_QUIT ) );
      break;

    case LW_REQUEST_REFRESH:
      PositionWindow( );
      break;

    case LW_PLUGIN_MOVE_UP:
      MovePluginUp( pcMessage2 );
	  Application::GetInstance( )->PostMessage( new Message( LW_SAVE_PREFS ) );      
      break;

    case LW_PLUGIN_MOVE_DOWN:
      MovePluginDown( pcMessage2 );
	  Application::GetInstance( )->PostMessage( new Message( LW_SAVE_PREFS ) );     
      break;

    case LW_PLUGIN_DELETE:
      DeletePlugin( pcMessage2 );
	  Application::GetInstance( )->PostMessage( new Message( LW_SAVE_PREFS ) );      
      break;
      
    case LW_PLUGIN_OPEN_ABOUT:
      OpenPluginAbout( pcMessage2->GetPlugin( ) );
      break;
      
    case LW_PLUGIN_OPEN_PREFS:
      OpenPluginPrefs( pcMessage2->GetPlugin( ) );
      break;
      
    case LW_PREFS_UPDATE_POSITION:
      SetPrefs( pcMessage, false );
      PositionWindow( );
	  Application::GetInstance( )->PostMessage( new Message( LW_SAVE_PREFS ) );
      break;

	case LW_EVENT_ABOUT:
	  OpenPluginAbout();
	  break;

	case LW_PLUGIN_PREFS_BTN_OK:   // OK Clicked in PrefsWindow.
      if( pcMessage2->GetPlugin() != 0 ) {
      	pcMessage2->GetPlugin( )->HandleMessage( pcMessage2 );
      }
	  Application::GetInstance( )->PostMessage( new Message( LW_SAVE_PREFS ) );
	  break;	  

	case LW_EVENT_BROADCAST:
	  BroadcastMessage( pcMessage2 );
	  break;

    default:
      if( pcMessage2->GetPlugin() != 0 ) {
      	pcMessage2->GetPlugin( )->HandleMessage( pcMessage2 );
      }
      break;

  }

}



void LauncherWindow::InitPlugins( Message *pcMessage )
{

  LauncherMessage *pcPluginMessage = new LauncherMessage( );
  string zPluginName;

  int nPluginCount = 0;
  int nJunk = 0;
  pcMessage->GetNameInfo( "Plugin", &nJunk, &nPluginCount );

  for( int n = 0; n < nPluginCount; n++ )
  {
    pcMessage->FindMessage( "Plugin", pcPluginMessage, n );
	InsertPlugins( pcPluginMessage );
  }
}


void LauncherWindow::InsertPlugins( LauncherMessage *pcMessage )
{

  string zPluginName;
  LauncherPlugin *pcPlugin;
  int nPluginCount = 0;
  int nJunk = 0;
  pcMessage->GetNameInfo( "__Name", &nJunk, &nPluginCount );
  pcMessage->AddInt32( "VPos", m_nVPos );
  pcMessage->AddInt32( "HPos", m_nHPos );
  pcMessage->AddInt32( "Align", m_nAlign );
  pcMessage->AddBool( "AutoHide", m_bAutoHide );
  pcMessage->AddBool( "AutoShow", m_bAutoShow );
  pcMessage->AddPointer( "__WINDOW", this );

  for( int n = 0; n < nPluginCount; n++ )
  {    
    pcMessage->FindString( "__Name", &zPluginName, n );
    
    pcPlugin = new LauncherPlugin( zPluginName, pcMessage );
    if( pcPlugin->GetId( ) != -1 ) {
      pcPlugin->SetWindowPos( m_apcPlugin.size( ) );
      m_apcPlugin.push_back( pcPlugin );
    } else {
      delete pcPlugin;
    }
  }
  
}    

void LauncherWindow::SetPrefs( Message *pcPrefs, bool bDeletePlugins = true )
{

  pcPrefs->FindInt32( "VPos", &m_nVPos );
  pcPrefs->FindInt32( "HPos", &m_nHPos );
  pcPrefs->FindInt32( "Align", &m_nAlign );
  pcPrefs->FindBool( "AutoHide", &m_bAutoHide );
  pcPrefs->FindBool( "AutoShow", &m_bAutoShow );
  pcPrefs->FindBool( "Visible", &m_bVisible );

  if( bDeletePlugins == true ) {
    if( m_apcPlugin.size( ) > 0 )
      DeletePlugins( );
    InitPlugins( pcPrefs );
  } else {
  	if( m_apcPlugin.size( ) > 0 ) {
  		pcPrefs->SetCode( LW_WINDOW_PREFS_CHANGED );
  		for( uint n = 0; n < m_apcPlugin.size(); n++ )
  			m_apcPlugin[n]->HandleMessage( (LauncherMessage *)pcPrefs );
  	}
  }
}


Message *LauncherWindow::GetPrefs( void )
{
  Message *pcPrefs = new Message( );
  pcPrefs->AddInt32( "VPos", m_nVPos );
  pcPrefs->AddInt32( "HPos", m_nHPos );
  pcPrefs->AddInt32( "Align", m_nAlign );
  pcPrefs->AddBool( "AutoHide", m_bAutoHide );
  pcPrefs->AddBool( "AutoShow", m_bAutoShow );

  for( uint32 n = 0; n < m_apcPlugin.size( ); n++ )
    pcPrefs->AddMessage( "Plugin", m_apcPlugin[n]->GetPrefs( ) );

  return pcPrefs;
}


void LauncherWindow::MovePluginUp( LauncherMessage *pcMessage )
{

  int nPos = pcMessage->GetPlugin( )->GetWindowPos( );

  if( nPos > 0 ) {
    LauncherPlugin *pcPlugin = m_apcPlugin[nPos - 1];
    m_apcPlugin[nPos - 1] = m_apcPlugin[nPos];
    m_apcPlugin[nPos] = pcPlugin;
    m_apcPlugin[nPos - 1]->SetWindowPos( nPos - 1 );
    m_apcPlugin[nPos]->SetWindowPos( nPos );
    PositionWindow( );
  }

}


void LauncherWindow::MovePluginDown( LauncherMessage *pcMessage )
{
	
  int nPos = pcMessage->GetPlugin( )->GetWindowPos( );

  if( (uint32)nPos < m_apcPlugin.size( ) - 1 ) {
    LauncherPlugin *pcPlugin = m_apcPlugin[nPos + 1];
    m_apcPlugin[nPos + 1] = m_apcPlugin[nPos];
    m_apcPlugin[nPos] = pcPlugin;
    m_apcPlugin[nPos + 1]->SetWindowPos( nPos + 1 );
    m_apcPlugin[nPos]->SetWindowPos( nPos );
    PositionWindow( );
  }

}

void LauncherWindow::DeletePlugin( LauncherMessage *pcMessage )
{

  int nPos = pcMessage->GetPlugin( )->GetWindowPos( );

  delete m_apcPlugin[nPos];
  m_apcPlugin.erase( &m_apcPlugin[nPos] );
  for( uint n = 0; n < m_apcPlugin.size( ); n++ )
    m_apcPlugin[n]->SetWindowPos( n );
  PositionWindow( );

}

void LauncherWindow::DeletePlugins( void )
{
  for( uint32 n = 0; n < m_apcPlugin.size( ); n++ ) {
    delete m_apcPlugin[n];
  }
}



void LauncherWindow::PositionWindow( bool bToggleVisibility = false )
{

  if( bToggleVisibility )
	  m_bVisible = ( m_bVisible ) ? false : true;

  float fSize = 0;
  string zBiggest;
  View *pcView;

  for( uint32 n = 0; n < m_apcPlugin.size( ); n++ )
    if( m_apcPlugin[n]->GetView( )->GetWindow( ) )
      m_apcPlugin[n]->RemoveView( );

  if( m_pcArrow->GetWindow( ) )
    m_pcLayout->RemoveChild( m_pcArrow );

  if( m_pcLayout )
    delete m_pcLayout;

  m_pcRoot = (m_nAlign == LW_ALIGN_VERTICAL) ? new VLayoutNode( "Root" ) : new HLayoutNode( "Root" );     

  if( m_nAlign == LW_ALIGN_VERTICAL ) {
    if( m_nVPos == LW_VPOS_BOTTOM ) {
      m_pcArrow->SetArrow( (m_bVisible) ? AB_DIR_DOWN : AB_DIR_UP );
      m_pcRoot->AddChild( m_pcArrow );
    }
  } else {
    if( m_nHPos == LW_HPOS_RIGHT ) {
      m_pcArrow->SetArrow( (m_bVisible) ? AB_DIR_RIGHT : AB_DIR_LEFT );
      m_pcRoot->AddChild( m_pcArrow );
    }
  }

  if( m_apcPlugin.size( ) > 0 )
    for( uint32 n = 0; n < m_apcPlugin.size( ); n++ )
      if( m_apcPlugin[n] )
        if( ( m_apcPlugin[n]->GetHideStatus( ) == LW_VISIBLE_ALWAYS ) || ( m_bVisible ) )
          m_pcRoot->AddChild( m_apcPlugin[n]->GetView( ) );

  if( m_nAlign == LW_ALIGN_VERTICAL ) {
    if( m_nVPos == LW_VPOS_TOP ) {
      m_pcArrow->SetArrow( (m_bVisible) ? AB_DIR_UP : AB_DIR_DOWN );
      m_pcRoot->AddChild( m_pcArrow );
    }
  } else {
    if( m_nHPos == LW_HPOS_LEFT ) {
      m_pcArrow->SetArrow( (m_bVisible) ? AB_DIR_LEFT : AB_DIR_RIGHT );
      m_pcRoot->AddChild( m_pcArrow );
    }
  }

  if( m_apcPlugin.size( ) > 0 )
  {
  	
    for( uint32 n = 0; n < m_apcPlugin.size( ); n++ )
    {
      if( m_apcPlugin[n] )
        if( ( m_apcPlugin[n]->GetHideStatus( ) == LW_VISIBLE_ALWAYS ) || ( m_bVisible ) ) {
          pcView = m_apcPlugin[n]->GetView( );
          Point cSize = pcView->GetPreferredSize( false );
          if( m_nAlign == LW_ALIGN_VERTICAL ) {
            if( cSize.x > fSize ) {
              zBiggest = pcView->GetTitle( );
              fSize = cSize.x;
            }
          } else { 
            if( cSize.y > fSize ) {
              zBiggest = pcView->GetTitle( );
              fSize = cSize.y;
            }  
          }
        }
    }

    if( zBiggest != "" )
      if( m_nAlign == LW_ALIGN_VERTICAL )
        m_pcRoot->SameWidth( zBiggest.c_str( ), "ArrowButton" , NULL );
      else
        m_pcRoot->SameHeight( zBiggest.c_str( ), "ArrowButton" , NULL );

    for( uint32 n = 0; n < m_apcPlugin.size( ); n++ )
    {   
      if( m_apcPlugin[n] ) 
        if( ( m_apcPlugin[n]->GetHideStatus( ) == LW_VISIBLE_ALWAYS ) || ( m_bVisible ) ) {
          pcView = m_apcPlugin[n]->GetView( );
          if( zBiggest != pcView->GetTitle( ) )
            if( m_nAlign == LW_ALIGN_VERTICAL )
              m_pcRoot->SameWidth( zBiggest.c_str( ), pcView->GetTitle( ).c_str( ), NULL );
            else
              m_pcRoot->SameHeight( zBiggest.c_str( ), pcView->GetTitle( ).c_str( ), NULL );
        }
    }

  }
  
  Point cSize = m_pcRoot->GetPreferredSize( false );

  ResizeTo( Point(cSize.x,cSize.y - 1) );
  m_pcLayout = new LayoutView( Rect( 0,0,cSize.x,cSize.y), "Layout", m_pcRoot );
  AddChild( m_pcLayout );
  
  Application::GetInstance( )->PostMessage( new Message( LW_EVENT_POSITION_WINDOW ) );  
  
}


void LauncherWindow::TimerTick( int nId )
{
  if( nId == LW_CHECK_MOUSE ) {
    if( ! m_bZoomLocked && m_pcLayout ) {
      Point cPoint;
      uint32 nButtons;

      m_pcLayout->GetMouse( &cPoint, &nButtons );
      cPoint = m_pcLayout->ConvertToScreen( cPoint );
      Rect cFrame = GetFrame( );

      if( cFrame.DoIntersect( cPoint ) ) {
        if( m_bAutoShow && ! m_bVisible )
        	PositionWindow( true );
      } else if( m_bAutoHide && m_bVisible )
      	  PositionWindow( true );
    }
  } else if( (m_apcPlugin[nId]) && ((uint)nId < m_apcPlugin.size())) m_apcPlugin[nId]->TimerTick( ); 
}


void LauncherWindow::OpenPluginAbout( LauncherPlugin *pcPlugin = 0)
{
	string zName = APP_NAME;
	string zVersion = APP_VERSION;
	string zAuthor = APP_AUTHOR;
	string zDesc = APP_DESC;
	
	if( pcPlugin != 0 ) {
	  	LauncherMessage *pcInfo = pcPlugin->GetInfo( );
		pcInfo->FindString( "Name", &zName );
		pcInfo->FindString( "Author", &zAuthor );
		pcInfo->FindString( "Version",  &zVersion );
		pcInfo->FindString( "Description", &zDesc );
	}
	
	string zTitle = (string)"About " + zName;
	
	string zInfo = zName + (string)"\n\nVersion: " + zVersion + (string)"\n\nAuthor: " + zAuthor + (string)"\n\n" + zDesc;

	Alert *pcAboutAlert = new Alert( zTitle, zInfo, 0, "OK", NULL );
	pcAboutAlert->Go( new Invoker( new LauncherMessage( LW_PLUGIN_ABOUT_CLOSED, pcPlugin ), this ) );
}


void LauncherWindow::OpenPluginPrefs( LauncherPlugin *pcPlugin )
{
	PluginPrefWindow *pcPluginPrefWindow = new PluginPrefWindow( pcPlugin, this );
	pcPluginPrefWindow->Show( );
	pcPluginPrefWindow->MakeFocus( );
}


void LauncherWindow::BroadcastMessage( LauncherMessage *pcMessage )
{
	
  int32 nBroadcastCode = 0;
  pcMessage->FindInt32( "BroadcastCode", &nBroadcastCode );
  
  for( uint32 n = 0; n < m_apcPlugin.size( ); n++ )  
    if( (m_apcPlugin[n]) && (m_apcPlugin[n] != pcMessage->GetPlugin()) ) 
      m_apcPlugin[n]->HandleMessage( new LauncherMessage( nBroadcastCode ) );

}

//*******************************************************************************************


ArrowButton::ArrowButton( int nDirection, const char *pzName, Message *pcMessage ) : ImageButton( Rect(), pzName, "", pcMessage )
{

  m_pcMenu = 0;
  
  SetArrow( nDirection );
}

ArrowButton::~ArrowButton( )
{

}

void ArrowButton::SetArrow( int nDirection )
{

  switch( nDirection ) {
    case AB_DIR_UP:    SetImage( arrow_up.width, arrow_up.height, (uint8 *)arrow_up.pixel_data )   ; break;
    case AB_DIR_DOWN:  SetImage( arrow_down.width, arrow_down.height, (uint8 *)arrow_down.pixel_data ) ; break;
    case AB_DIR_LEFT:  SetImage( arrow_left.width, arrow_left.height, (uint8 *)arrow_left.pixel_data ) ; break; 
    case AB_DIR_RIGHT: SetImage( arrow_right.width, arrow_right.height, (uint8 *)arrow_right.pixel_data ); break; 
    default: break;
  }


}
  
void ArrowButton::AttachedToWindow( void )
{

  SetTarget( GetWindow( ) );

}

void ArrowButton::MouseDown( const Point &cPosition, uint32 nButtons )
{

  if( nButtons == 2 ) {
  	if( m_pcPluginMenu ) delete m_pcMenu;
    m_pcMenu = new Menu( Rect( ), "PopUp", ITEMS_IN_COLUMN );
    m_pcMenu->AddItem( "About Launcher...", new Message( LW_EVENT_ABOUT ) );
    m_pcMenu->AddItem( "Preferences...", new Message( LW_EVENT_PREFS ) );
    m_pcMenu->AddItem( new MenuSeparator( ) );
    m_pcMenu->AddItem( "New Window", new Message( LW_EVENT_ADD_WINDOW ) );
    m_pcMenu->AddItem( new MenuSeparator( ) );
    m_pcMenu->AddItem( "Delete Window", new Message( LW_EVENT_CLOSE_WINDOW ) );
    m_pcMenu->AddItem( "Quit Launcher", new Message( LW_EVENT_QUIT ) );
    m_pcMenu->SetTargetForItems( GetWindow( ) );
  
  	m_pcPluginMenu = new Menu( Rect( ), "Add Plugin", ITEMS_IN_COLUMN );
  	
    Directory *pcDir = new Directory( );
    string zName;
 
    if( pcDir->SetTo( get_launcher_path() + "plugins/" ) == 0 ) {
      while( pcDir->GetNextEntry( &zName ) )
      {
        if( zName.length() > 2 ) {
          if( zName.find( ".so", zName.length() - 3) != string::npos ) {
            zName.erase( zName.length( ) - 3, 3 );
            Message *pcMsg = new Message( LW_CREATE_PLUGINS );
            pcMsg->AddString( "__Name", zName );
            m_pcPluginMenu->AddItem(  zName.c_str( ), pcMsg );

          }
        }
      }

      delete pcDir;
    }
    m_pcPluginMenu->SetTargetForItems( GetWindow( ) );
   	Message cCloseMessage( LW_ZOOM_UNLOCK );
    m_pcMenu->SetCloseMessage( cCloseMessage );
	m_pcMenu->SetCloseMsgTarget( GetWindow( ) ); 
    m_pcMenu->AddItem( m_pcPluginMenu, 4 );
    GetWindow( )->PostMessage( new LauncherMessage( LW_ZOOM_LOCK ) );
    
    Point cPos = ConvertToScreen( cPosition );
    Point cSize = m_pcMenu->GetPreferredSize( false );
    Rect cFrame = frame_in_screen( Rect( cPos.x, cPos.y, cPos.x + cSize.x, cPos.y + cSize.y ) ); 
    m_pcMenu->Open( Point( cFrame.left, cFrame.top) );
    
  }  else 
    Button::MouseDown( cPosition, nButtons );
}


void ArrowButton::MouseUp( const Point &cPosition, uint32 nButtons, Message *pcData )
{
	if( pcData ) {
		string zDraggedFile;
		pcData->FindString( "file/path", &zDraggedFile );
		if( zDraggedFile != "" ) {
			pcData->SetCode( LW_CREATE_PLUGINS );
			pcData->AddString( "__Name", "LauncherIcon" );
			pcData->AddString( "Target", zDraggedFile );
			Invoker *pcInvoker = new Invoker( pcData, GetWindow() );
			pcInvoker->Invoke();
		}
	} else {
		Button::MouseUp( cPosition, nButtons, pcData );
	}
}

//*******************************************************************************************















