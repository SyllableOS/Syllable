#include <stdio.h>
#include <stdarg.h>

#include "screenpanel.h"

#include <gui/button.h>
#include <gui/listview.h>
#include <gui/dropdownmenu.h>
#include <gui/checkbox.h>
#include <gui/spinner.h>
#include <gui/stringview.h>
#include <gui/desktop.h>
#include <gui/spinner.h>

#include <util/application.h>
#include <util/message.h>


using namespace os;

enum
{
  ID_CLOSE,
  ID_APPLY,
  ID_COLOR_SPACE_CHANGED,
  ID_RESOLUTION_CHANGED,
  ID_REFRESH_CHANGED
};

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

ScreenPanel::ScreenPanel( const Rect& cFrame ) : LayoutView( cFrame, "", NULL, CF_FOLLOW_NONE )
{
    HLayoutNode* pcRoot = new HLayoutNode( "root" );
    
    m_sOriginalMode = Desktop().GetScreenMode();
    m_sCurrentMode  = m_sOriginalMode;
    
    m_pcOkBut     = new Button( Rect( 0, 0, 0, 0 ), "apply_but", "Apply", new Message( ID_APPLY ), CF_FOLLOW_NONE );
    m_pcCancelBut = new Button( Rect( 0, 0, 0, 0 ), "close_but", "Close", new Message( M_QUIT ), CF_FOLLOW_NONE );

//    m_pcAllCheckBox    = new CheckBox( Rect(0,0,0,0), "all_cb", "All desktops", NULL );
    m_pcColorSpaceStr  = new StringView( Rect(0,0,0,0), "", "Color space", ALIGN_CENTER );
    m_pcColorSpaceList = new DropdownMenu( Rect(0,0,0,0), "color_space_list", true, CF_FOLLOW_NONE );
    m_pcRefreshRateStr = new StringView( Rect(0,0,0,0), "", "Refresh rate", ALIGN_CENTER );
    m_pcRefreshRate    = new Spinner( Rect(0,0,0,0), "refresh_spinner", 60.0f, new Message( ID_REFRESH_CHANGED ) );
    m_pcModeList       = new ListView( Rect(0,0,0,0), "mode_list", ListView::F_RENDER_BORDER | ListView::F_NO_AUTO_SORT );

    pcRoot->AddChild( m_pcModeList, 1.0f );


    VLayoutNode* pcRightPanel       = new VLayoutNode( "right_panel", 0.0f, pcRoot );
    VLayoutNode* pcTopRightPanel    = new VLayoutNode( "top_left_panel", 1.0f, pcRightPanel );
    new LayoutSpacer( "spacer", 1.0f, pcRightPanel );
    VLayoutNode* pcBottomRightPanel = new VLayoutNode( "bottom_left_panel", 0.0f, pcRightPanel );

    pcBottomRightPanel->SetHAlignment( ALIGN_RIGHT );
    
    pcRoot->ExtendMaxSize( Point( 0.0f, MAX_SIZE ) );
    pcRightPanel->ExtendMaxSize( Point( 0.0f, MAX_SIZE ) );
    pcTopRightPanel->ExtendMaxSize( Point( MAX_SIZE, MAX_SIZE ) );
    pcBottomRightPanel->ExtendMaxSize( Point( MAX_SIZE, MAX_SIZE ) );

    
//    pcTopRightPanel->AddChild( m_pcAllCheckBox );
    pcTopRightPanel->AddChild( new LayoutSpacer( "spacer" ) );
    pcTopRightPanel->AddChild( m_pcColorSpaceStr );
    pcTopRightPanel->AddChild( m_pcColorSpaceList );
    pcTopRightPanel->AddChild( new LayoutSpacer( "spacer" ) );
    pcTopRightPanel->AddChild( m_pcRefreshRateStr );
    pcTopRightPanel->AddChild( m_pcRefreshRate );
    pcTopRightPanel->AddChild( new LayoutSpacer( "spacer" ) );

    pcBottomRightPanel->AddChild( m_pcOkBut );
    pcBottomRightPanel->AddChild( m_pcCancelBut );

    m_pcRefreshRate->SetValue( m_sOriginalMode.m_vRefreshRate );
    m_pcColorSpaceList->SetSelectionMessage( new Message( ID_COLOR_SPACE_CHANGED ) );
    m_pcColorSpaceList->SetReadOnly();
    m_pcModeList->InsertColumn( "Resolution", 10000 );

    pcRoot->SetBorders( Rect(10.0f,10.0f,10.0f,10.0f), "mode_list", "right_panel", NULL );
    pcRoot->SetBorders( Rect(10.0f,5.0f,10.0f,5.0f), "apply_but", "close_but", NULL );
    pcRoot->SameWidth( "apply_but", "close_but", NULL );
    SetRoot( pcRoot );

    m_pcModeList->SetInvokeMsg( new Message( ID_APPLY ) );
    m_pcModeList->SetSelChangeMsg( new Message( ID_RESOLUTION_CHANGED ) );
    
    Application* pcApp = Application::GetInstance();
  
    int nModeCount = pcApp->GetScreenModeCount();

    for ( int i = 0 ; i < nModeCount ; ++i ) {
	screen_mode sMode;
  
	if ( pcApp->GetScreenModeInfo( i, &sMode ) == 0 )
	{
	    std::map<color_space,ColorSpace>::iterator i = m_cColorSpaces.find( sMode.m_eColorSpace );
	    if ( i == m_cColorSpaces.end() ) {
		m_cColorSpaces.insert( std::map<color_space,ColorSpace>::value_type( sMode.m_eColorSpace, ColorSpace( sMode.m_eColorSpace ) ) );
		i = m_cColorSpaces.find( sMode.m_eColorSpace );
	    }
	    (*i).second.m_cResolutions.push_back( sMode );
	}
    }
    int nSelCS = -1;
    std::map<color_space,ColorSpace>::iterator i;
    int j = 0;
    for ( i = m_cColorSpaces.begin() ; i != m_cColorSpaces.end() ; ++i, ++j ) {
	if ( (*i).first == m_sOriginalMode.m_eColorSpace ) {
	    nSelCS = j;
	}
	switch( (*i).first )
	{
	    case CS_RGB32:	m_pcColorSpaceList->AppendItem( "RGB32" ); break;
	    case CS_RGB24:	m_pcColorSpaceList->AppendItem( "RGB24" ); break;
	    case CS_RGB16:	m_pcColorSpaceList->AppendItem( "RGB16" ); break;
	    case CS_RGB15:	m_pcColorSpaceList->AppendItem( "RGB15" ); break;
	    case CS_CMAP8:	m_pcColorSpaceList->AppendItem( "CMAP8" ); break;
	    case CS_GRAY8:	m_pcColorSpaceList->AppendItem( "GRAY8" ); break;
	    case CS_GRAY1:	m_pcColorSpaceList->AppendItem( "GRAY1" ); break;
	    case CS_YUV422:	m_pcColorSpaceList->AppendItem( "YUV422" ); break;
	    case CS_YUV411:	m_pcColorSpaceList->AppendItem( "YUV411" ); break;
	    case CS_YUV420:	m_pcColorSpaceList->AppendItem( "YUV420" ); break;
	    case CS_YUV444:	m_pcColorSpaceList->AppendItem( "YUV444" ); break;
	    case CS_YUV9:	m_pcColorSpaceList->AppendItem( "YUV9" ); break;
	    case CS_YUV12:	m_pcColorSpaceList->AppendItem( "YUV12" ); break;
	    default:	m_pcColorSpaceList->AppendItem( "INV" ); break;
	}
    }
    if ( nSelCS != -1 ) {
	m_pcColorSpaceList->SetSelection( nSelCS );
    }
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ScreenPanel::AllAttached()
{
  m_pcModeList->SetTarget( this );
  m_pcColorSpaceList->SetTarget( this );
  m_pcOkBut->SetTarget( this );
  m_pcRefreshRate->SetTarget( this );
//  m_pcColorSpaceList->SetSelection( 0, false );
  UpdateResList();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ScreenPanel::FrameSized( const Point& cDelta )
{
    LayoutView::FrameSized( cDelta );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ScreenPanel::KeyDown( const char* pzString, const char* pzRawString, uint32 nQualifiers )
{
  switch( pzString[0] )
  {
    case VK_UP_ARROW:
      break;
    case VK_DOWN_ARROW:
      break;
    case VK_SPACE:
      break;
    default:
      View::KeyDown( pzString, pzRawString, nQualifiers );
      break;
  }
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ScreenPanel::Paint( const Rect& cUpdateRect )
{
  Rect cBounds = GetBounds();

  SetFgColor( get_default_color( COL_NORMAL ) );
  FillRect( cBounds );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ScreenPanel::UpdateResList()
{
    int nCSSelection = m_pcColorSpaceList->GetSelection();
    int nResNum = -1;
  
    if ( nCSSelection < 0  ) {
	return;
    }
    std::map<color_space,ColorSpace>::iterator i = m_cColorSpaces.begin();
    while( nCSSelection-- > 0 ) ++i;

    m_sCurrentMode.m_eColorSpace = (*i).first;
    m_pcModeList->Clear();

    for ( uint j = 0 ; j < (*i).second.m_cResolutions.size() ; ++j ) {
	ListViewStringRow* pcRow = new ListViewStringRow();
	char zBuf[128];
	sprintf( zBuf, "%dx%d", (*i).second.m_cResolutions[j].m_nWidth, (*i).second.m_cResolutions[j].m_nHeight );

	if ( m_sCurrentMode.m_nWidth == (*i).second.m_cResolutions[j].m_nWidth &&
	     m_sCurrentMode.m_nHeight == (*i).second.m_cResolutions[j].m_nHeight ) {
	    nResNum = j;
	}
	pcRow->AppendString( zBuf );
	  
	m_pcModeList->InsertRow( pcRow );
    }
    if ( nResNum > 0 ) {
	m_pcModeList->Select( nResNum );
	Flush();
    }
}

void ScreenPanel::HandleMessage( Message* pcMessage )
{
    switch( pcMessage->GetCode() )
    {
	case ID_COLOR_SPACE_CHANGED:
	{
	    bool bFinal = false;
	    pcMessage->FindBool( "final", &bFinal );
	    if ( bFinal ) {
		UpdateResList();
	    }
	    break;
	}
	case ID_RESOLUTION_CHANGED:
	{
	    int nCSSelection = m_pcColorSpaceList->GetSelection();
	    int nResSelection = m_pcModeList->GetFirstSelected();
	    std::map<color_space,ColorSpace>::iterator i = m_cColorSpaces.begin();
	    
	    while( nCSSelection-- > 0 ) ++i;
	    
	    m_sCurrentMode.m_nWidth  = (*i).second.m_cResolutions[nResSelection].m_nWidth;
	    m_sCurrentMode.m_nHeight = (*i).second.m_cResolutions[nResSelection].m_nHeight;
	    break;
	}
	case ID_REFRESH_CHANGED:
	    m_sCurrentMode.m_vRefreshRate = m_pcRefreshRate->GetValue();
	    break;

	case ID_APPLY:
	{
	    Desktop().SetScreenMode( &m_sCurrentMode );
	    break;
	}
	default:
	    View::HandleMessage( pcMessage );
	    break;
    }
}
