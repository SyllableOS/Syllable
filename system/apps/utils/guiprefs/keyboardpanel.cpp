#include <stdio.h>
#include <stdarg.h>
#include <dirent.h>

#include "keyboardpanel.h"
#include "coloredit.h"

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

#include <appserver/keymap.h>


using namespace os;

enum
{
  ID_CLOSE,
  ID_APPLY,
  ID_REFRESH,
  ID_TIMING_CHANGED,
};


static Point get_max_width( Point* pcFirst, ... )
{
  Point cSize = *pcFirst;
  va_list hList;

  va_start( hList, pcFirst );

  Point* pcTmp;
  while( (pcTmp = va_arg( hList, Point* )) != NULL ) {
    if ( pcTmp->x > cSize.x ) {
      cSize = *pcTmp;
    }
  }
  va_end( hList );
  return( cSize );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

KeyboardPanel::KeyboardPanel( const Rect& cFrame ) : LayoutView( cFrame, "", NULL, CF_FOLLOW_NONE )
{
    int nKeyDelay;
    int nKeyRepeat;

    HLayoutNode* pcRoot = new HLayoutNode( "root" );
    
    Application::GetInstance()->GetKeyboardConfig( NULL, &nKeyDelay, &nKeyRepeat );
    
    m_pcRefreshBut= new Button( Rect( 0, 0, 0, 0 ), "refresh_but", "Refresh", new Message( ID_REFRESH ), CF_FOLLOW_NONE );
    m_pcOkBut     = new Button( Rect( 0, 0, 0, 0 ), "apply_but", "Apply", new Message( ID_APPLY ), CF_FOLLOW_NONE );
    m_pcCancelBut = new Button( Rect( 0, 0, 0, 0 ), "close_but", "Close", new Message( M_QUIT ), CF_FOLLOW_NONE );

    m_pcKeymapList  = new ListView( Rect(0,0,0,0), "keymap_list", ListView::F_RENDER_BORDER );
    m_pcKeyDelay    = new Spinner( Rect(0,0,0,0), "key_delay", float(nKeyDelay), new Message( ID_TIMING_CHANGED ) );
    m_pcKeyRepeat   = new Spinner( Rect(0,0,0,0), "key_repeat", float(nKeyRepeat), new Message( ID_TIMING_CHANGED ) );


    m_pcKeyRepeat->SetMinPreferredSize( 4 );
    m_pcKeyRepeat->SetMaxPreferredSize( 4 );
    m_pcKeyDelay->SetMinPreferredSize( 4 );
    m_pcKeyDelay->SetMaxPreferredSize( 4 );

    pcRoot->AddChild( m_pcKeymapList, 1.0f );


    VLayoutNode* pcRightPanel       = new VLayoutNode( "right_panel", 0.0f, pcRoot );
    VLayoutNode* pcTopRightPanel    = new VLayoutNode( "top_left_panel", 1.0f, pcRightPanel );
    new LayoutSpacer( "spacer", 2.0f, pcRightPanel );
    VLayoutNode* pcBottomRightPanel = new VLayoutNode( "bottom_left_panel", 0.0f, pcRightPanel );

    pcBottomRightPanel->SetHAlignment( ALIGN_RIGHT );

    pcTopRightPanel->AddChild( new StringView( Rect(0,0,0,0), "", "Key delay", ALIGN_CENTER ) );
    pcTopRightPanel->AddChild( m_pcKeyDelay );
    pcTopRightPanel->AddChild( new LayoutSpacer( "spacer", 0.5f ) );
    pcTopRightPanel->AddChild( new StringView( Rect(0,0,0,0), "", "Key repeat", ALIGN_CENTER ) );
    pcTopRightPanel->AddChild( m_pcKeyRepeat );

    pcBottomRightPanel->AddChild( m_pcOkBut );
    pcBottomRightPanel->AddChild( m_pcRefreshBut );
    pcBottomRightPanel->AddChild( m_pcCancelBut );
    
    
    m_pcKeyDelay->SetFormat( "%.0f" );
    m_pcKeyDelay->SetMinMax( 10, 1000 );
    m_pcKeyDelay->SetStep( 1.0f );
    m_pcKeyDelay->SetScale( 1.0f );
    
    m_pcKeyRepeat->SetFormat( "%.0f" );
    m_pcKeyRepeat->SetMinMax( 10, 1000 );
    m_pcKeyRepeat->SetStep( 1.0f );
    m_pcKeyRepeat->SetScale( 1.0f );
  
    m_pcKeymapList->InsertColumn( "Key mapping", 10000 );

    pcRoot->SetBorders( Rect(10.0f,10.0f,10.0f,10.0f), "keymap_list", "right_panel", NULL );
    pcRoot->SetBorders( Rect(10.0f,5.0f,10.0f,5.0f), "refresh_but", "apply_but", "close_but", NULL );
    pcRoot->SameWidth( "refresh_but", "apply_but", "close_but", NULL );
    pcRoot->SameWidth( "key_delay", "key_repeat", NULL );
    
    SetRoot( pcRoot );
    m_pcKeymapList->SetInvokeMsg( new Message( ID_APPLY ) );

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
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void KeyboardPanel::AllAttached()
{
  m_pcKeymapList->SetTarget( this );
  m_pcOkBut->SetTarget( this );
  m_pcRefreshBut->SetTarget( this );
  m_pcKeyDelay->SetTarget( this );
  m_pcKeyRepeat->SetTarget( this );
  
  UpdateResList();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void KeyboardPanel::FrameSized( const Point& cDelta )
{
    LayoutView::FrameSized( cDelta );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void KeyboardPanel::KeyDown( const char* pzString, const char* pzRawString, uint32 nQualifiers )
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

void KeyboardPanel::Paint( const Rect& cUpdateRect )
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

void KeyboardPanel::UpdateResList()
{
    DIR* pDir = opendir( "/system/keymaps" );
    if ( pDir == NULL ) {
	return;
    }
    String cKeymap;
    Application::GetInstance()->GetKeyboardConfig( &cKeymap, NULL, NULL );

    m_pcKeymapList->Clear();
    dirent* psEntry;

    int i = 0;
    while( (psEntry = readdir( pDir )) != NULL ) {
	if ( strcmp( psEntry->d_name, "." ) == 0 || strcmp( psEntry->d_name, ".." ) == 0 ) {
	    continue;
	}

	FILE* hFile = fopen( (std::string( "/system/keymaps/" ) + psEntry->d_name).c_str(), "r" );
	if ( hFile == NULL ) {
	    continue;
	}
	uint32 nMagic = 0;
	fread( &nMagic, sizeof(nMagic), 1, hFile );
	fclose( hFile );
	if ( nMagic != KEYMAP_MAGIC ) {
	    continue;
	}
    
	ListViewStringRow* pcRow = new ListViewStringRow();

	pcRow->AppendString( psEntry->d_name );      
	m_pcKeymapList->InsertRow( pcRow );
	if ( cKeymap == psEntry->d_name ) {
	    m_pcKeymapList->Select( i );
	}
	++i;
    }
    closedir( pDir );
}

void KeyboardPanel::HandleMessage( Message* pcMessage )
{
    switch( pcMessage->GetCode() )
    {
	case ID_REFRESH:
	    UpdateResList();
	    break;
	case ID_TIMING_CHANGED:
	    Application::GetInstance()->SetKeyboardTimings( int(m_pcKeyDelay->GetValue()), int(m_pcKeyRepeat->GetValue()) );
	    break;
	case ID_APPLY:
	{
	    int nSelection = m_pcKeymapList->GetFirstSelected();
	    if ( nSelection < 0 ) {
		break;
	    }
	    ListViewStringRow* pcRow = static_cast<ListViewStringRow*>(m_pcKeymapList->GetRow( nSelection ));
	    Application::GetInstance()->SetKeymap( pcRow->GetString( 0 ).c_str() );
	    break;
	}
	default:
	    View::HandleMessage( pcMessage );
	    break;
    }
}

