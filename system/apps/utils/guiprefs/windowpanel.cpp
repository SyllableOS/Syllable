#include <stdio.h>
#include <stdarg.h>
#include <dirent.h>

#include "windowpanel.h"
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
#include <util/appserverconfig.h>


using namespace os;

enum
{
  ID_CLOSE,
  ID_APPLY,
  ID_REFRESH,
  ID_PEN_CHANGED,
};

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

WindowPanel::WindowPanel( const Rect& cFrame ) : LayoutView( cFrame, "", NULL, CF_FOLLOW_NONE )
{
    HLayoutNode* pcRoot = new HLayoutNode( "root" );
    
    m_pcRefreshBut= new Button( Rect( 0, 0, 0, 0 ), "refresh_but", "Refresh", new Message( ID_REFRESH ), CF_FOLLOW_NONE );
    m_pcOkBut     = new Button( Rect( 0, 0, 0, 0 ), "apply_but", "Apply", new Message( ID_APPLY ), CF_FOLLOW_NONE );
    m_pcCancelBut = new Button( Rect( 0, 0, 0, 0 ), "close_but", "Close", new Message( M_QUIT ), CF_FOLLOW_NONE );

    m_pcDecoratorList    = new ListView( Rect(0,0,0,0), "decorator_list", ListView::F_RENDER_BORDER | ListView::F_NO_AUTO_SORT );
    m_pcPenMenu          = new DropdownMenu( Rect(0,0,0,0), "pen_list" );
    m_pcColorEdit        = new ColorEdit( Rect(0,0,0,0), "color_edit", get_default_color( COL_NORMAL ) );
    m_pcPopupWndCheckBox = new CheckBox( Rect(0,0,0,0), "all_popup_selected_windows", "Popup selected windows", NULL );
    m_pcDecoratorList->InsertColumn( "Window decorator", 10000 );

    pcRoot->AddChild( m_pcDecoratorList );
    
    VLayoutNode* pcRightPanel       = new VLayoutNode( "left_panel", 0.0f, pcRoot );
    VLayoutNode* pcTopRightPanel    = new VLayoutNode( "top_left_panel", 1.0f, pcRightPanel );
    VLayoutNode* pcBottomRightPanel = new VLayoutNode( "bottom_left_panel", 0.0f, pcRightPanel );

    pcBottomRightPanel->SetHAlignment( ALIGN_RIGHT );
    
    pcRoot->ExtendMaxSize( Point( 0.0f, FLT_MAX ) );
    pcRightPanel->ExtendMaxSize( Point( 0.0f, FLT_MAX ) );
    pcTopRightPanel->ExtendMaxSize( Point( FLT_MAX, FLT_MAX ) );
    pcBottomRightPanel->ExtendMaxSize( Point( FLT_MAX, FLT_MAX ) );
    
    pcTopRightPanel->AddChild( m_pcPenMenu );
    pcTopRightPanel->AddChild( m_pcColorEdit );
    pcTopRightPanel->AddChild( m_pcPopupWndCheckBox );

    pcBottomRightPanel->AddChild( m_pcRefreshBut );
    pcBottomRightPanel->AddChild( m_pcCancelBut );
    pcBottomRightPanel->AddChild( m_pcOkBut );

    pcRoot->SetBorders( Rect(5.0f,5.0f,5.0f,5.0f) );
    pcRoot->SetBorders( Rect(5.0f,15.0f,5.0f,10.0f), "decorator_list", "top_left_panel", NULL );
    pcRoot->SetBorders( Rect(5.0f,10.0f,5.0f,10.0f), "bottom_left_panel", NULL );
    pcRoot->SetBorders( Rect( 15.0f,10.0f,0.0f,0.0f), "refresh_but", "apply_but", "close_but", NULL );

    pcRoot->SameWidth( "refresh_but", "apply_but", "close_but", NULL );
    
    m_pcPenMenu->SetReadOnly();
    m_pcPenMenu->AppendItem( "Normal" );
    m_pcPenMenu->AppendItem( "Shine" );
    m_pcPenMenu->AppendItem( "Shadow" );
    m_pcPenMenu->AppendItem( "Selected window" );
    m_pcPenMenu->AppendItem( "Window border" );
    m_pcPenMenu->AppendItem( "Menu text" );
    m_pcPenMenu->AppendItem( "Menu highlight text" );
    m_pcPenMenu->AppendItem( "Menu background" );
    m_pcPenMenu->AppendItem( "Menu highlight" );
    m_pcPenMenu->AppendItem( "ScrollBar bg." );
    m_pcPenMenu->AppendItem( "ScrollBar knob" );
    m_pcPenMenu->AppendItem( "ListView tab" );
    m_pcPenMenu->AppendItem( "ListView tab text" );

    m_pcPenMenu->SetSelection(0);
    m_pcPenMenu->SetSelectionMessage( new Message( ID_PEN_CHANGED ) );
  
    m_pcDecoratorList->SetInvokeMsg( new Message( ID_APPLY ) );

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
    m_pcPopupWndCheckBox->SetValue( AppserverConfig().GetPopoupSelectedWindows() );
    SetRoot( pcRoot );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void WindowPanel::AllAttached()
{
    m_pcDecoratorList->SetTarget( this );
    m_pcOkBut->SetTarget( this );
    m_pcRefreshBut->SetTarget( this );
    m_pcPenMenu->SetTarget( this );

    UpdateDecoratorList();
  
    m_pcDecoratorList->Select( 0 );
    m_pcPenMenu->SetSelection( 0 );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void WindowPanel::FrameSized( const Point& cDelta )
{
    LayoutView::FrameSized( cDelta );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void WindowPanel::KeyDown( const char* pzString, const char* pzRawString, uint32 nQualifiers )
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

void WindowPanel::Paint( const Rect& cUpdateRect )
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

void WindowPanel::UpdateDecoratorList()
{
    DIR* pDir = opendir( "/system/drivers/appserver/decorators" );
    if ( pDir == NULL ) {
	return;
    }
    m_pcDecoratorList->Clear();
    dirent* psEntry;
    while( (psEntry = readdir( pDir )) != NULL ) {
	if ( strcmp( psEntry->d_name, "." ) == 0 || strcmp( psEntry->d_name, ".." ) == 0 ) {
	    continue;
	}
	ListViewStringRow* pcRow = new ListViewStringRow();

	pcRow->AppendString( psEntry->d_name );      
	  
	m_pcDecoratorList->InsertRow( pcRow );
    }
    closedir( pDir );
}

void WindowPanel::HandleMessage( Message* pcMessage )
{
    switch( pcMessage->GetCode() )
    {
	case ID_REFRESH:
	    UpdateDecoratorList();
	    m_pcPopupWndCheckBox->SetValue( AppserverConfig().GetPopoupSelectedWindows() );
	    break;
	case ID_PEN_CHANGED:
	{
	    default_color_t ePen = COL_NORMAL;
	    switch( m_pcPenMenu->GetSelection() )
	    {
		case 0: ePen = COL_NORMAL; break;
		case 1: ePen = COL_SHINE; break;
		case 2: ePen = COL_SHADOW; break;
		case 3: ePen = COL_SEL_WND_BORDER; break;
		case 4: ePen = COL_NORMAL_WND_BORDER; break;
		case 5: ePen = COL_MENU_TEXT; break;
		case 6: ePen = COL_SEL_MENU_TEXT; break;
		case 7: ePen = COL_MENU_BACKGROUND; break;
		case 8: ePen = COL_SEL_MENU_BACKGROUND; break;
		case 9: ePen = COL_SCROLLBAR_BG; break;
		case 10: ePen = COL_SCROLLBAR_KNOB; break;
		case 11: ePen = COL_LISTVIEW_TAB; break;
		case 12: ePen = COL_LISTVIEW_TAB_TEXT; break;
	    }
	    m_pcColorEdit->SetValue( get_default_color( ePen ) );
	    break;
	}
	case ID_APPLY:
	{
	    char zPath[1024];
      
	    AppserverConfig().SetPopoupSelectedWindows( m_pcPopupWndCheckBox->GetValue() );
	    int nSelection = m_pcDecoratorList->GetFirstSelected();
	    if ( nSelection < 0 ) {
		break;
	    }
	    default_color_t ePen = COL_NORMAL;
	    switch( m_pcPenMenu->GetSelection() )
	    {
		case 0: ePen = COL_NORMAL; break;
		case 1: ePen = COL_SHINE; break;
		case 2: ePen = COL_SHADOW; break;
		case 3: ePen = COL_SEL_WND_BORDER; break;
		case 4: ePen = COL_NORMAL_WND_BORDER; break;
		case 5: ePen = COL_MENU_TEXT; break;
		case 6: ePen = COL_SEL_MENU_TEXT; break;
		case 7: ePen = COL_MENU_BACKGROUND; break;
		case 8: ePen = COL_SEL_MENU_BACKGROUND; break;
		case 9: ePen = COL_SCROLLBAR_BG; break;
		case 10: ePen = COL_SCROLLBAR_KNOB; break;
		case 11: ePen = COL_LISTVIEW_TAB; break;
		case 12: ePen = COL_LISTVIEW_TAB_TEXT; break;
	  
	    }
	    set_default_color( ePen, m_pcColorEdit->GetValue() );
	    Application::GetInstance()->CommitColorConfig();
	    ListViewStringRow* pcRow = static_cast<ListViewStringRow*>(m_pcDecoratorList->GetRow( nSelection ));
	    strcpy( zPath, "/system/drivers/appserver/decorators/" );
	    strcat( zPath, pcRow->GetString( 0 ).c_str() );
	    Application::GetInstance()->SetWindowDecorator( zPath );
	    break;
	}
	default:
	    View::HandleMessage( pcMessage );
	    break;
    }
}
