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

#include "main.h"

Launcher::Launcher( ) : Application( "application/x-vnd.ADK-Launcher" )
{
 
  FSNode *pcConfigDir = new FSNode();
  if( pcConfigDir->SetTo( PREFS_DIR ) != 0 )
  {
  	dbprintf( "Config Directory Not Found. Creating Default Config Directory\n" );
	system( (get_launcher_path() + (string)"/bin/create_default_config").c_str() );
  }

  FSNode *pcNode = new FSNode();
  
  if( pcNode->SetTo( PREFS_FILE ) != 0 )
  {
  	dbprintf( "Config File Not Found. Using Default Config File\n" );
  	string zPath = get_launcher_path() + "default.cfg";  	
	pcNode->SetTo( zPath );
  }
  
  File *pcConfig = new File( *pcNode );
  uint32 nSize = pcConfig->GetSize( );
  void *pBuffer = malloc( nSize );
  pcConfig->Read( pBuffer, nSize );
  Message *pcPrefs = new Message( );
  pcPrefs->Unflatten( (uint8 *)pBuffer );

  int nWindowCount = 0;
  int nJunk = 0;
  Message *pcWindowPrefs = new Message( );
  LauncherWindow *pcWindow;

  pcPrefs->GetNameInfo( "Window", &nJunk, &nWindowCount );
  if( nWindowCount > 0 ) {
    for( int n=0; n < nWindowCount; n++ ) {
      pcPrefs->FindMessage( "Window", pcWindowPrefs, n );
      pcWindowPrefs->AddInt32( "ID", n );
      pcWindowPrefs->SetCode( m_apcWindow.size() );

      pcWindow = new LauncherWindow( pcWindowPrefs );
      pcWindow->Show( );
      if( pcWindow )
        m_apcWindow.push_back( pcWindow );
    }
  } else {
    LauncherWindow *pcWindow = new LauncherWindow( new Message( m_apcWindow.size() ) );
    m_apcWindow.push_back( pcWindow );
    pcWindow->Show( );
  }
  
  delete pcConfig;
  free( pBuffer );

  delete pcConfigDir;
  delete pcNode;

}

Launcher::~Launcher( )
{

}


bool Launcher::OkToQuit( void )
{
  SavePrefs();

  for( uint32 n = 0; n < m_apcWindow.size( ); n++ )
    if( m_apcWindow[n] != NULL ) {
  	  m_apcWindow[n]->PostMessage( new Message( os::M_QUIT ) );
    }

  return true;
}    


void Launcher::SavePrefs( void )
{
  Message *pcPrefs = new Message( );
  for( uint32 n = 0; n < m_apcWindow.size( ); n++ )
    if( m_apcWindow[n] != NULL )
      pcPrefs->AddMessage( "Window", m_apcWindow[n]->GetPrefs( ) );

  File *pcConfig = new File( (string)PREFS_FILE, O_WRONLY | O_CREAT );  
  if( pcConfig ) 
  {
    uint32 nSize = pcPrefs->GetFlattenedSize( );
    void *pBuffer = malloc( nSize );
    pcPrefs->Flatten( (uint8 *)pBuffer, nSize );

    pcConfig->Write( pBuffer, nSize );
      
    delete pcConfig;
    free( pBuffer );
  }
}	


int main( int argc, char** argv )
{

  Launcher* pcMyLauncher = new Launcher( );

  pcMyLauncher->Run( );
  return( 0 );

}


void Launcher::HandleMessage( Message *pcMessage )
{
	int nCode = pcMessage->GetCode( );
	LauncherWindow *pcWindow;
	int32 nId = 0;
	pcMessage->FindInt32( "ID", &nId );
	
	switch( nCode )
	{
		case LW_EVENT_ADD_WINDOW:
			pcWindow = new LauncherWindow( new Message( m_apcWindow.size() ), true );
		    m_apcWindow.push_back( pcWindow );
		    pcWindow->Show( );
		    break;
		    
		case LW_EVENT_CLOSE_WINDOW:
        	m_apcWindow[nId]->PostMessage( new Message( os::M_QUIT ) );
        	m_apcWindow[nId] = NULL;
        	PositionWindow( );
			break;
			
		case LW_EVENT_POSITION_WINDOW:
			PositionWindow( );
			break;
			
		case LW_SAVE_PREFS:
			SavePrefs();
			break;
			
		default:
			Application::HandleMessage( pcMessage );
			break;
	}
}



void Launcher::PositionWindow( void )
{
	Desktop *pcDesktop = new Desktop( );
	IPoint  *pcRes = new IPoint( pcDesktop->GetResolution( ) );  
	Point cTopLeft = Point( 0,0 );
	Point cTopRight = Point( pcRes->x, 0 );
	Point cBotLeft = Point( 0, pcRes->y );
	Point cBotRight = Point( pcRes->x, pcRes->y );

	for( uint32 n = 0; n < m_apcWindow.size( ); n++ )
		if( m_apcWindow[n] != NULL ) {
				
			int32 nHPos = m_apcWindow[n]->GetHPos( );
			int32 nVPos = m_apcWindow[n]->GetVPos( );
			int32 nAlign = m_apcWindow[n]->GetAlign( );			
			Rect cFrame = m_apcWindow[n]->GetFrame();
		
			if( nHPos == LW_HPOS_LEFT ) {
				if( nVPos == LW_VPOS_TOP ) {
					m_apcWindow[n]->MoveTo( cTopLeft.x, cTopLeft.y );
					cFrame = m_apcWindow[n]->GetFrame();
					cTopLeft = (nAlign == LW_ALIGN_VERTICAL) ? Point( cFrame.right, cTopLeft.y )
															 : Point( cTopLeft.x, cFrame.bottom );
				} else {
					m_apcWindow[n]->MoveTo( Point( cBotLeft.x, cBotLeft.y - cFrame.Height() ) );
					cFrame = m_apcWindow[n]->GetFrame();
					cBotLeft = (nAlign == LW_ALIGN_VERTICAL) ? Point( cFrame.right, cBotLeft.y )
															 : Point( cBotLeft.x, cFrame.top );
				}
			} else {
				if( nVPos == LW_VPOS_TOP ) {
					m_apcWindow[n]->MoveTo( Point( cTopRight.x - cFrame.Width(), cTopRight.y ) );
					cFrame = m_apcWindow[n]->GetFrame();
					cTopRight = (nAlign == LW_ALIGN_VERTICAL) ? Point( cFrame.left, cTopRight.y )
															  : Point( cTopRight.x, cFrame.bottom );
				} else {
					m_apcWindow[n]->MoveTo( Point( cBotRight.x - cFrame.Width(), cBotRight.y - cFrame.Height() ) );
					cFrame = m_apcWindow[n]->GetFrame();
					cBotRight = (nAlign == LW_ALIGN_VERTICAL) ? Point( cFrame.left, cBotRight.y )
															  : Point( cBotRight.x, cFrame.top );
				}
			}
			m_apcWindow[n]->Flush();
		}
}
	
