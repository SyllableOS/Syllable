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

PrefWindow::PrefWindow( LauncherWindow *pcParent ) : Window( Rect(), "plugin_list", "Launcher Preferences" )
{

  m_pcParent = pcParent;

  Message *pcPrefs = m_pcParent->GetPrefs( );
  pcPrefs->FindInt32( "VPos", &m_nVPos );
  pcPrefs->FindInt32( "HPos", &m_nHPos );
  pcPrefs->FindInt32( "Align", &m_nAlign );
  pcPrefs->FindBool( "AutoHide", &m_bAutoHide );
  pcPrefs->FindBool( "AutoShow", &m_bAutoShow );

  VLayoutNode *pcPosRoot = new VLayoutNode( "PosRoot" );
  pcPosRoot->AddChild( new HLayoutSpacer( "blah", 100 ) );
  m_pcPosition = new WindowPositioner( m_nVPos, m_nHPos, m_nAlign );
  pcPosRoot->AddChild( m_pcPosition );
  
  HLayoutNode *pcPosThree = new HLayoutNode( "PosThree", 1.0f, pcPosRoot );
  m_pcCBAutoHide = new CheckBox( Rect( ), "AutoHide", "Auto-Hide", new Message( LW_PREFS_UPDATE_POSITION ) );
  m_pcCBAutoHide->SetValue( m_bAutoHide );
  pcPosThree->AddChild( m_pcCBAutoHide );  
  m_pcCBAutoShow = new CheckBox( Rect( ), "AutoShow", "Auto-Show", new Message( LW_PREFS_UPDATE_POSITION ) );
  m_pcCBAutoShow->SetValue( m_bAutoShow );
  pcPosThree->AddChild( m_pcCBAutoShow );
    
  LayoutView *pcPosLayout = new LayoutView( Rect(), "PosLayout", pcPosRoot );

  VLayoutNode *pcPluginRoot = new VLayoutNode( "PluginRoot" );
  HLayoutNode *pcHLPluginList = new HLayoutNode( "PluginListLayout" );
  HLayoutNode *pcHLPluginInfo = new HLayoutNode( "PluginInfoLayout" );
  HLayoutNode *pcPluginButtons = new HLayoutNode( "PluginButtons" );
  FrameView *pcFVPluginInfo = new FrameView( Rect(), "PluginInfo", "" );
  VLayoutNode *pcInfoLayout = new VLayoutNode( "InfoLayout" );
  HLayoutNode *pcInfoRoot = new HLayoutNode( "InfoRoot" );
  m_pcSVPluginName = new StringView( Rect(), "PluginName", "Name:", ALIGN_LEFT );
  m_pcSVPluginName->SetMinPreferredSize( 20 );
  m_pcSVPluginVersion = new StringView( Rect(), "PluginVersion", "Version:", ALIGN_LEFT );
  m_pcSVPluginAuthor = new StringView( Rect(), "PluginAuthor", "Author:", ALIGN_LEFT );
  m_pcSVPluginInfo = new StringView( Rect(), "PluginInfo", "Info:", ALIGN_LEFT );
//  pcInfoLayout->AddChild( new VLayoutSpacer( "space", 1.0f ) );
  pcInfoLayout->AddChild( m_pcSVPluginName );  
  pcInfoLayout->AddChild( m_pcSVPluginVersion );
  pcInfoLayout->AddChild( m_pcSVPluginAuthor );
  pcInfoLayout->AddChild( m_pcSVPluginInfo );
  pcInfoLayout->AddChild( new HLayoutSpacer( "blah", 100 ) );  
  pcInfoLayout->SameWidth( "blah", "PluginName", "PluginVersion", "PluginAuthor", "PluginInfo", NULL );
  pcInfoRoot->AddChild( new VLayoutSpacer( "space", 0.5f ) );
  pcInfoRoot->AddChild( pcInfoLayout );
  pcFVPluginInfo->SetRoot( pcInfoRoot );
  
  
  m_pcPluginList = new ListView( Rect(), "PluginList" );
  m_pcPluginList->InsertColumn( "Name", 160 );
  UpdatePluginList( );
  m_pcPluginList->SetSelChangeMsg( new Message( LW_PREFS_PLUGIN_SELECTED ) );
  m_pcPluginList->SetInvokeMsg( new Message( LW_EVENT_ADD_PLUGIN ) );
  
  pcHLPluginList->AddChild( new VLayoutSpacer( "space", 100 ) );
  pcHLPluginList->AddChild( m_pcPluginList );
  
//  pcHLPluginInfo->AddChild( new VLayoutSpacer( "space", 100 ) );
  pcHLPluginInfo->AddChild( pcFVPluginInfo );
  
  pcPluginRoot->AddChild( new HLayoutSpacer( "blah", 100 ) );
  pcPluginRoot->AddChild( pcHLPluginList );
  pcPluginRoot->AddChild( pcHLPluginInfo );
  pcPluginButtons->AddChild( new VLayoutSpacer( "space", 1.0f ) );
  pcPluginButtons->AddChild( new Button( Rect( ), "btn_add", "Add", new Message( LW_EVENT_ADD_PLUGIN ) ) );
  pcPluginRoot->AddChild( pcPluginButtons );
 
  LayoutView *pcPluginLayout = new LayoutView( Rect(), "PluginLayout", pcPluginRoot );

  TabView *pcTabs = new TabView( Rect(), "Tabs" );
  pcTabs->AppendTab( "Position", pcPosLayout );
  pcTabs->AppendTab( "Plugins", pcPluginLayout );

  VLayoutNode *pcRoot = new VLayoutNode( "root" );
  pcRoot->AddChild( pcTabs );
  HLayoutNode *pcBottom = new HLayoutNode( "bottom" );
  Button *pcCloseBtn = new Button( Rect( ), "btn_close", "Close", new Message( LW_EVENT_CLOSE_WINDOW ) );
  pcBottom->AddChild( new HLayoutSpacer( "space", 100 ) );
  pcBottom->AddChild( new VLayoutSpacer( "space2", 1.0f ) );
  pcBottom->AddChild( pcCloseBtn );
  pcRoot->AddChild( pcBottom );

  Point cSize = pcRoot->GetPreferredSize( false );

  SetFrame( center_frame( Rect( 0,0,cSize.x + 32, cSize.y + 32 ) ) );
  
  pcPluginRoot->AddChild( new HLayoutSpacer( "space_1", cSize.x + 32 ) );
  pcPluginRoot->SameWidth( "PluginList", "PluginInfo", "PluginButtons", "space_1", NULL );
  
  m_pcLayout = new LayoutView( Rect(0,0,cSize.x + 32, cSize.y + 32), "Layout", pcRoot );
  AddChild( m_pcLayout );

  SetDefaultButton( pcCloseBtn );

  Show( );

  MakeFocus();
  SetFocusChild( pcCloseBtn );

}


PrefWindow::~PrefWindow( )
{

}


void PrefWindow::AdjustWindowSize( Point cSize, bool bCentre = true )
{
	
}

void PrefWindow::HandleMessage( Message *pcMessage )
{


  int nId = pcMessage->GetCode( );

  switch( nId )
  {

    case LW_PREFS_UPDATE_POSITION:
      UpdatePosition( );
      break;
      
    case LW_PREFS_PLUGIN_SELECTED:
      ShowPluginInfo( );
      break;
      
    case LW_EVENT_ADD_PLUGIN:
      AddPlugins( );
      break;

    case LW_EVENT_CLOSE_WINDOW:
      Close( );
      break;

    default: 
      break;

  }

}

void PrefWindow::UpdatePosition( void )
{
  m_nVPos = m_pcPosition->GetVPos(); 
  m_nHPos = m_pcPosition->GetHPos(); 
  m_nAlign = m_pcPosition->GetAlign();
  m_bAutoHide = m_pcCBAutoHide->GetValue( );
  m_bAutoShow = m_pcCBAutoShow->GetValue( );

  Message *pcPrefs = new Message( LW_PREFS_UPDATE_POSITION );
  pcPrefs->AddInt32( "VPos", m_nVPos );
  pcPrefs->AddInt32( "HPos", m_nHPos );
  pcPrefs->AddInt32( "Align", m_nAlign );
  pcPrefs->AddBool( "AutoHide", m_bAutoHide );
  pcPrefs->AddBool( "AutoShow", m_bAutoShow );
      
  Invoker *pcParentInvoker = new Invoker( pcPrefs, m_pcParent );
  pcParentInvoker->Invoke();
}

void PrefWindow::AddPlugins( void )
{

  Message *pcMsg = new Message( LW_CREATE_PLUGINS );

  for( uint32 n = 0; n < m_pcPluginList->GetRowCount(); n++ )
    if( m_pcPluginList->IsSelected( n ) )
    {

      ListViewStringRow *pcRow = (ListViewStringRow *)m_pcPluginList->GetRow( n );
      string zName = pcRow->GetString( 0 );
      pcMsg->AddString( "__Name", zName );

    }

  m_pcPluginList->ClearSelection( );

  Invoker *pcParentInvoker = new Invoker( pcMsg, m_pcParent );
  pcParentInvoker->Invoke();

}

void PrefWindow::Close( void )
{

  m_pcParent->PostMessage( new Message( LW_PREFS_WINDOW_CLOSED ) );
  Window::Close( );

}



void PrefWindow::UpdatePluginList( void )
{
  Directory *pcDir = new Directory( );
  string zName;
 
  m_pcPluginList->Clear( );
 
  if( pcDir->SetTo( get_launcher_path() + "plugins/" ) == 0 ) {
    while( pcDir->GetNextEntry( &zName ) )
    {
      if( zName.length() > 2 ) {
        if( zName.find( ".so", zName.length() - 3) != string::npos ) {
          zName.erase( zName.length( ) - 3, 3 );

          ListViewStringRow *pcRow = new ListViewStringRow( );    
          pcRow->AppendString( zName );
          m_pcPluginList->InsertRow( pcRow, true );
        }
      }
    }

    delete pcDir;
  }
}


void PrefWindow::ShowPluginInfo( void )
{

  ListViewStringRow *pcRow = (ListViewStringRow *)m_pcPluginList->GetRow( m_pcPluginList->GetLastSelected( ) );
  
  if( pcRow ) {
  	
  	LauncherMessage *pcMessage = new LauncherMessage( );
  	pcMessage->AddPointer( "__WINDOW", this );
  	LauncherPlugin *pcPlugin = new LauncherPlugin( pcRow->GetString( 0 ), pcMessage );
  	
  	if( pcPlugin->GetId( ) != -1 ) {
  		
  	  Message *pcInfo = pcPlugin->GetInfo( );

 	  string zName, zVersion, zAuthor, zInfo;
  	  pcInfo->FindString( "Name", &zName );
 	  pcInfo->FindString( "Version", &zVersion );
  	  pcInfo->FindString( "Author", &zAuthor );
  	  pcInfo->FindString( "Description", &zInfo ); 
  		
  	  m_pcSVPluginName->SetString( ((string)"Name: " + zName).c_str() );
  	  m_pcSVPluginVersion->SetString( ((string)"Version: " + zVersion).c_str() );
  	  m_pcSVPluginAuthor->SetString( ((string)"Author: " + zAuthor).c_str() );
  	  m_pcSVPluginInfo->SetString( ((string)"Info: " + zInfo).c_str() );
  		
  	  delete pcPlugin;
  	}
  	
  }
}
  	
  		
//**************************************************************************


PluginPrefWindow::PluginPrefWindow( LauncherPlugin *pcPlugin, Window *pcParent ) : Window( Rect(), "plugin_prefs", "Preferences" )
{
	
	m_pcPlugin = pcPlugin;
	m_pcParent = pcParent;
	
	string zTitle = m_pcPlugin->GetName( ) + (string)" Preferences";
	SetTitle( zTitle.c_str( ) );
	
	LayoutView *pcPrefsView;
	VLayoutNode  *pcRoot;
	FrameView      *pcFrame;
	VLayoutNode      *pcInner;
	//View             *m_pcPluginPrefs;
	HLayoutNode        *pcInnerBottom;
	//CheckBox           *m_pcCBVisible;
	HLayoutNode    *pcBottom;
	ImageButton           *pcBtnOk;
	ImageButton           *pcBtnCancel;
	
	pcRoot = new VLayoutNode( "root" );
	pcInner = new VLayoutNode( "inner" );
	pcInnerBottom = new HLayoutNode( "innerbottom" );
	pcBottom = new HLayoutNode( "bottom" );
	
    m_pcPluginPrefs = m_pcPlugin->GetPrefsView( );
    
    m_pcCBVisible = new CheckBox( Rect(), "cb_visible", "Always Visible", NULL );
    m_pcCBVisible->SetValue( ( m_pcPlugin->GetHideStatus( ) == LW_VISIBLE_ALWAYS ) ? 1 : 0 );
    
    if( m_pcPluginPrefs != NULL )
    	pcInner->AddChild( m_pcPluginPrefs );
   	pcInnerBottom->AddChild( new VLayoutSpacer( "spacer_0", 1.0f ) );
    pcInnerBottom->AddChild( m_pcCBVisible );
    pcInner->AddChild( pcInnerBottom );
	pcInner->AddChild( new HLayoutSpacer( "spacer_4", 100 ) );
	    
    pcBtnOk = new ImageButton( Rect(), "btn_ok", "OK", new LauncherMessage( LW_PLUGIN_PREFS_BTN_OK, m_pcPlugin ) );
    string zIconPath = get_launcher_path() + "icons/choice-yes.png";
    pcBtnOk->SetImageFromFile( zIconPath.c_str() );
    pcBtnCancel = new ImageButton( Rect(), "btn_cancel", "Cancel", new LauncherMessage( LW_PLUGIN_PREFS_BTN_CANCEL, m_pcPlugin ) );
    zIconPath = get_launcher_path() + "icons/choice-no.png";    
    pcBtnCancel->SetImageFromFile( zIconPath.c_str() );    
    
	pcBottom->AddChild( pcBtnCancel );
	pcBottom->AddChild( new HLayoutSpacer( "spacer_1", 100 ) );
	pcBottom->AddChild( new VLayoutSpacer( "spacer_2", 1.0f ) );
	pcBottom->AddChild( pcBtnOk );
	
	pcFrame = new FrameView( Rect(), "frame_1", "" );
	pcFrame->SetRoot( pcInner );
	
	pcRoot->AddChild( pcFrame );
	pcRoot->AddChild( pcBottom );
	
	Point cSize = pcRoot->GetPreferredSize( false );
	
	pcPrefsView = new LayoutView( Rect( 2, 2, cSize.x + 34, cSize.y + 34 ), "prefs_layout" );
	pcPrefsView->SetRoot( pcRoot );

	SetFrame( center_frame( Rect( 0, 0, cSize.x + 36, cSize.y + 36 ) ) );
	
	AddChild( pcPrefsView );
	
	SetDefaultButton( pcBtnOk );
	SetFocusChild( pcBtnOk );
	
}


PluginPrefWindow::~PluginPrefWindow( )
{
	if( m_pcPluginPrefs ) 
		m_pcPluginPrefs->RemoveThis( );

}


void PluginPrefWindow::HandleMessage( Message *pcMessage )
{
	uint32 nCode = pcMessage->GetCode();
	
	if( nCode == LW_PLUGIN_PREFS_BTN_OK )
		m_pcPlugin->SetHideStatus( m_pcCBVisible->GetValue() ? LW_VISIBLE_ALWAYS : LW_VISIBLE_WHEN_ZOOMED );
		
	Invoker *pcParentInvoker = new Invoker( pcMessage, m_pcParent );
	pcParentInvoker->Invoke();
	
	if( (nCode == LW_PLUGIN_PREFS_BTN_OK) || (nCode == LW_PLUGIN_PREFS_BTN_CANCEL) )
		Quit( );		
}
	
//**************************************************************************


WindowPositioner::WindowPositioner( int32 nVPos, int32 nHPos, int32 nAlign ) : Control( Rect(), "WindowPositioner", "", new Message( LW_PREFS_UPDATE_POSITION ), CF_FOLLOW_ALL )
{
	m_nVPos = nVPos;
	m_nHPos = nHPos;
	m_nAlign = nAlign;
}


WindowPositioner::~WindowPositioner()
{
	
}

Point WindowPositioner::GetPreferredSize( bool bLargest ) const
{
	
	font_height sHeight;
	GetFontHeight( &sHeight );
	float vCharHeight = sHeight.ascender + sHeight.descender;
	float vHeight = vCharHeight * 12;
	float vWidth = GetStringWidth("W") * 20;
		
	return Point( vWidth, vHeight );
}

void WindowPositioner::MouseDown( const Point &cPosition, uint32 nButtons )
{
	Rect cBounds = GetBounds();

	cBounds.top += 2;
	cBounds.left += 2;
	cBounds.right -= 2;
	cBounds.bottom -= 2;
	
	float vX = cBounds.Width() / 10.0;
	float vY = cBounds.Height() / 10.0;

	m_nVPos = LW_VPOS_BOTTOM;
	m_nAlign = LW_ALIGN_HORIZONTAL;
	
	if( cPosition.x < vX * 5 ) {
		m_nHPos = LW_HPOS_LEFT;
		if( cPosition.y < ( vY * 5 ) )
			m_nVPos = LW_VPOS_TOP;
		if( cPosition.x < vX )
			m_nAlign = LW_ALIGN_VERTICAL;
	} else {
		m_nHPos = LW_HPOS_RIGHT;
		if( cPosition.y < ( vY * 5 ) )
			m_nVPos = LW_VPOS_TOP;
		if( cPosition.x > ( vX * 9 ) )
			m_nAlign = LW_ALIGN_VERTICAL;
	}
	
	Invoke();
	Invalidate();
	Flush();
}


void WindowPositioner::Paint( const Rect &cUpdateRect )
{
	Rect cBounds = GetBounds();
	
	SetEraseColor( get_default_color( COL_NORMAL ) );
	SetBgColor( get_default_color( COL_NORMAL ) );
	SetFgColor( get_default_color( COL_NORMAL ) );
	FillRect( cBounds );	
	
	DrawFrame( cBounds, FRAME_RECESSED );
	
	cBounds.top += 2;
	cBounds.left += 2;
	cBounds.right -= 2;
	cBounds.bottom -= 2;
	
	SetFgColor( 255,255,255 );
	FillRect( cBounds );

	float vT = cBounds.top;
	float vL = cBounds.left;	
	float vX = cBounds.Width() / 10.0;
	float vY = cBounds.Height() / 10.0;

	DrawFrame( Rect( vL, vT + vY , vL + vX, vT + vY * 3 ), FRAME_RAISED );
	DrawFrame( Rect( vL + vX, vT, vL + vX * 3, vT + vY ), FRAME_RAISED );
	DrawFrame( Rect( vL + vX * 7, vT, vL + vX * 9, vT + vY ), FRAME_RAISED );
	DrawFrame( Rect( vL + vX * 9, vT + vY, vL + vX * 10, vT + vY * 3 ), FRAME_RAISED ); 
	DrawFrame( Rect( vL + vX * 9, vT + vY * 7 , vL + vX * 10, vT + vY * 9 ), FRAME_RAISED );
	DrawFrame( Rect( vL + vX * 7, vT + vY * 9, vL + vX * 9, vT + vY * 10 ), FRAME_RAISED );
	DrawFrame( Rect( vL + vX, vT + vY * 9, vL + vX * 3, vT + vY * 10 ), FRAME_RAISED );
	DrawFrame( Rect( vL, vT + vY * 7, vL + vX, vT + vY * 9 ), FRAME_RAISED );
	
	if( (m_nAlign == LW_ALIGN_VERTICAL) && (m_nHPos == LW_HPOS_LEFT) && (m_nVPos == LW_VPOS_TOP) )
		DrawFrame( Rect( vL, vT + vY , vL + vX, vT + vY * 3 ), FRAME_RECESSED );
	else if( (m_nAlign == LW_ALIGN_HORIZONTAL) && (m_nHPos == LW_HPOS_LEFT) && (m_nVPos == LW_VPOS_TOP) )
		DrawFrame( Rect( vL + vX, vT, vL + vX * 3, vT + vY ), FRAME_RECESSED );
	else if( (m_nAlign == LW_ALIGN_HORIZONTAL) && (m_nHPos == LW_HPOS_RIGHT) && (m_nVPos == LW_VPOS_TOP) )		
		DrawFrame( Rect( vL + vX * 7, vT, vL + vX * 9, vT + vY ), FRAME_RECESSED );
	else if( (m_nAlign == LW_ALIGN_VERTICAL) && (m_nHPos == LW_HPOS_RIGHT) && (m_nVPos == LW_VPOS_TOP) )				
		DrawFrame( Rect( vL + vX * 9, vT + vY, vL + vX * 10, vT + vY * 3 ), FRAME_RECESSED ); 
	else if( (m_nAlign == LW_ALIGN_VERTICAL) && (m_nHPos == LW_HPOS_RIGHT) && (m_nVPos == LW_VPOS_BOTTOM) )				
		DrawFrame( Rect( vL + vX * 9, vT + vY * 7 , vL + vX * 10, vT + vY * 9 ), FRAME_RECESSED );
	else if( (m_nAlign == LW_ALIGN_HORIZONTAL) && (m_nHPos == LW_HPOS_RIGHT) && (m_nVPos == LW_VPOS_BOTTOM) )				
		DrawFrame( Rect( vL + vX * 7, vT + vY * 9, vL + vX * 9, vT + vY * 10 ), FRAME_RECESSED );
	else if( (m_nAlign == LW_ALIGN_HORIZONTAL) && (m_nHPos == LW_HPOS_LEFT) && (m_nVPos == LW_VPOS_BOTTOM) )				
		DrawFrame( Rect( vL + vX, vT + vY * 9, vL + vX * 3, vT + vY * 10 ), FRAME_RECESSED );
	else if( (m_nAlign == LW_ALIGN_VERTICAL) && (m_nHPos == LW_HPOS_LEFT) && (m_nVPos == LW_VPOS_BOTTOM) )				
		DrawFrame( Rect( vL, vT + vY * 7, vL + vX, vT + vY * 9 ), FRAME_RECESSED );

	string zLabel = (m_nVPos == LW_VPOS_TOP)?(string)"Top - ":(string)"Bottom - ";
	zLabel += (m_nHPos == LW_HPOS_LEFT)?(string)"Left - ":(string)"Right - ";
	zLabel += (m_nAlign == LW_ALIGN_VERTICAL)?(string)"Vertical":(string)"Horizontal";
	float vStrWidth = GetStringWidth( zLabel.c_str() );
	SetFgColor( 0,0,0 );
	DrawString( zLabel.c_str(), Point( (cBounds.Width() - vStrWidth)/2, cBounds.Height()/2 ) );
}

