#include <stdio.h>

#include "fontpanel.h"

#include <gui/textview.h>
#include <gui/button.h>
#include <gui/dropdownmenu.h>
#include <gui/listview.h>
#include <gui/spinner.h>

#include <util/application.h>
#include <util/message.h>


using namespace os;

enum
{
    ID_APPLY,
    ID_RESTORE,
    ID_RESCAN,
    ID_SEL_CHANGED,
    ID_SELECTED,
    ID_SIZE_CHANGED,
    ID_BM_SIZE_CHANGED,
    ID_CONFIG_NAME_CHANGED
};


class TestView : public View
{
public:
  TestView( const Rect& cFrame, const char* pzName, uint32 nFollowFlags );
  virtual void Paint( const os::Rect& cUpdateRect );
};

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

TestView::TestView( const Rect& cFrame, const char* pzName, uint32 nFollowFlags ) : View( cFrame, pzName, nFollowFlags )
{
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void TestView::Paint( const os::Rect& cUpdateRect )
{
  SetFgColor( 255, 255 ,255 );
  FillRect( cUpdateRect );

  SetFgColor( 0, 0, 0 );
  
  MovePenTo( 30, 40 );
  DrawString( "Test string" );
}


//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

FontPanel::FontPanel( const Rect& cFrame ) : LayoutView( cFrame, "", NULL, CF_FOLLOW_NONE )
{
    VLayoutNode* pcRoot     = new VLayoutNode( "root" );
    HLayoutNode* pcTopPane  = new HLayoutNode( "top_pane", 1.0f, pcRoot );
    
    m_pcRescanBut = new Button( Rect( 0, 0, 0, 0 ), "rescan_but", "Rescan", new Message(ID_RESCAN), CF_FOLLOW_NONE );
    m_pcOkBut     = new Button( Rect( 0, 0, 0, 0 ), "ok_but", "Apply", new Message(ID_APPLY), CF_FOLLOW_NONE );
    m_pcCancelBut = new Button( Rect( 0, 0, 0, 0 ), "cancel_but", "Restore", new Message(ID_RESTORE), CF_FOLLOW_NONE );

    m_pcFontList    = new ListView( Rect( 0, 0, 0, 0 ), "font_list", ListView::F_RENDER_BORDER, CF_FOLLOW_NONE );
    m_pcSizeSpinner = new Spinner( Rect( 0, 0, 0, 0 ), "size_spinner", 12.0f, new Message( ID_SIZE_CHANGED ), CF_FOLLOW_NONE );

    m_pcFontConfigList = new DropdownMenu( Rect(0,0,0,0), "font_cfg_list", true, CF_FOLLOW_NONE );
    m_pcBitmapSizeList = new DropdownMenu( Rect(0,0,0,0), "bitmap_size_list", true, CF_FOLLOW_NONE );
    
    m_pcTestView    = new TextView( Rect( 0, 0, 0, 0 ), "test_view", "Test string", CF_FOLLOW_NONE );

    pcRoot->AddChild( m_pcTestView );
    
    pcTopPane->AddChild( m_pcFontList );
    VLayoutNode* pcRightPane = new VLayoutNode( "right_pane", 0.0f, pcTopPane );
    
    m_pcTestView->SetMultiLine( true );
    m_pcFontConfigList->SetReadOnly( true );
    m_pcBitmapSizeList->SetReadOnly( true );
    
    m_pcSizeSpinner->SetMinValue( 1.0 );
    m_pcSizeSpinner->SetMaxValue( 100.0 );
    m_pcSizeSpinner->SetMinPreferredSize( 5 );
    m_pcSizeSpinner->SetMaxPreferredSize( 5 );
    
    m_pcFontList->InsertColumn( "Name", 130 );
    m_pcFontList->InsertColumn( "Style", 100 );
    m_pcFontList->InsertColumn( "Spacing", 80 );

    pcRightPane->AddChild( m_pcFontConfigList );
    pcRightPane->AddChild( m_pcSizeSpinner );
    pcRightPane->AddChild( m_pcBitmapSizeList );
    pcRightPane->AddChild( new LayoutSpacer( "spacer", 5.0f ) );
    pcRightPane->AddChild( m_pcRescanBut );
    pcRightPane->AddChild( m_pcOkBut );
    pcRightPane->AddChild( m_pcCancelBut );

    pcRoot->SetBorders( Rect( 10,10,10,10 ), "top_pane", "test_view", NULL );
    pcRoot->SetBorders( Rect( 10,5,10,5 ), "font_cfg_list", "bitmap_size_list", "size_spinner", "rescan_but", "ok_but", "cancel_but", NULL );
    pcRoot->SameWidth( "rescan_but", "ok_but", "cancel_but", NULL );
    
    SetRoot( pcRoot );
    
    m_pcFontList->SetSelChangeMsg( new Message( ID_SEL_CHANGED ) );
    m_pcFontList->SetInvokeMsg( new Message( ID_SELECTED ) );
    
    std::vector<std::String> cConfigList;

    Font::GetConfigNames( &cConfigList );

    for ( uint i = 0 ; i < cConfigList.size() ; ++i ) {
	ConfigNode sNode;
	sNode.m_cName = cConfigList[i];
	sNode.m_bModified = false;
	if ( Font::GetDefaultFont( cConfigList[i], &sNode.m_sConfig ) != 0 ) {
	    printf( "Error: Failed to get default font %s\n", cConfigList[i].c_str() );
	    continue;
	}
	sNode.m_sOrigConfig = sNode.m_sConfig;
	m_cConfigList.push_back( sNode );
	m_pcFontConfigList->AppendItem( cConfigList[i].c_str() );
    }
    m_pcBitmapSizeList->SetSelectionMessage( new Message( ID_BM_SIZE_CHANGED ) );
    m_pcFontConfigList->SetSelectionMessage( new Message( ID_CONFIG_NAME_CHANGED ) );
    m_pcFontConfigList->SetSelection( 0 );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void FontPanel::AllAttached()
{
    m_pcRescanBut->SetTarget( this );
    m_pcOkBut->SetTarget( this );
    m_pcCancelBut->SetTarget( this );
    m_pcSizeSpinner->SetTarget( this );
    m_pcFontList->SetTarget( this );
    m_pcBitmapSizeList->SetTarget( this );
    m_pcFontConfigList->SetTarget( this );
    
    UpdateSelectedConfig();
  
    m_pcTestView->SetBgColor( 255, 255, 255 );
    m_pcTestView->SetEraseColor( 255, 255, 255 );
    m_pcTestView->SetFgColor( 0, 0, 0 );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void FontPanel::FrameSized( const Point& cDelta )
{
    LayoutView::FrameSized( cDelta );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void FontPanel::KeyDown( const char* pzString, const char* pzRawString, uint32 nQualifiers )
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

void FontPanel::Paint( const Rect& cUpdateRect )
{
  Rect cBounds = GetBounds();

  SetFgColor( get_default_color( COL_NORMAL ) );
  FillRect( cBounds );
}

void FontPanel::UpdateSelectedConfig()
{
    int nSelection = m_pcFontConfigList->GetSelection();
    if ( nSelection < 0 ) {
	return;
    }
    m_nCurSelConfig = nSelection;
    m_pcSizeSpinner->SetValue( m_cConfigList[m_nCurSelConfig].m_sConfig.m_vSize );

    m_pcFontList->Clear();
    int nFamCount = Font::GetFamilyCount();

    for ( int i = 0 ; i < nFamCount ; ++i ) {
	char zFamily[FONT_FAMILY_LENGTH+1];
	Font::GetFamilyInfo( i, zFamily );

	int nStyleCount = Font::GetStyleCount( zFamily );
    
	for ( int j = 0 ; j < nStyleCount ; ++j ) {
	    char zStyle[FONT_STYLE_LENGTH+1];
	    uint32 nFlags;
      
	    Font::GetStyleInfo( zFamily, j, zStyle, &nFlags );

	    if ( m_cConfigList[m_nCurSelConfig].m_cName == DEFAULT_FONT_FIXED ) {
		if ( (nFlags & os::FONT_IS_FIXED) == 0 ) {
		    continue;
		}
	    }
	    ListViewStringRow* pcRow = new ListViewStringRow();
	    pcRow->AppendString( zFamily );
	    pcRow->AppendString( zStyle );
	    pcRow->AppendString( (nFlags & os::FONT_IS_FIXED) ? "mono" : "dynamic" );
	    m_pcFontList->InsertRow( pcRow );
	}
    }

    
    uint nCount = m_pcFontList->GetRowCount();
    for ( uint j = 0 ; j < nCount ; ++j ) {
	ListViewStringRow* pcRow = dynamic_cast<ListViewStringRow*>( m_pcFontList->GetRow( j ) );

	if ( pcRow->GetString(0) == m_cConfigList[m_nCurSelConfig].m_sConfig.m_cFamily &&
	     pcRow->GetString(1) == m_cConfigList[m_nCurSelConfig].m_sConfig.m_cStyle ) {
	    m_pcFontList->Select( j );
	    m_pcFontList->MakeVisible( j );
	    break;
	}
    }
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void FontPanel::HandleMessage( Message* pcMessage )
{
    switch( pcMessage->GetCode() )
    {
	case ID_RESCAN:
	{
	    if ( Font::Rescan() ) {
		UpdateSelectedConfig();
	    }
	    break;
	}
	case ID_APPLY:
	{
	    for ( uint i = 0 ; i < m_cConfigList.size() ; ++i ) {
		if ( m_cConfigList[i].m_bModified ) {
		    Font::SetDefaultFont( m_cConfigList[i].m_cName, m_cConfigList[i].m_sConfig );
		    m_cConfigList[i].m_bModified = false;
		}
	    }
	    break;
	}
	case ID_RESTORE:
	{
	    for ( uint i = 0 ; i < m_cConfigList.size() ; ++i ) {
		m_cConfigList[i].m_sConfig = m_cConfigList[i].m_sOrigConfig;
		Font::SetDefaultFont( m_cConfigList[i].m_cName, m_cConfigList[i].m_sConfig );
	    }
	    UpdateSelectedConfig();
	    break;
	}
	case ID_CONFIG_NAME_CHANGED:
	{
	    UpdateSelectedConfig();
	    break;
	}
	case ID_SEL_CHANGED:
	{
	    int nSel = m_pcFontList->GetFirstSelected();
	    if ( nSel >= 0 ) {
		ListViewStringRow* pcRow = dynamic_cast<ListViewStringRow*>( m_pcFontList->GetRow( nSel ) );
		if ( pcRow != NULL ) {
		    m_cConfigList[m_nCurSelConfig].m_sConfig.m_cFamily = pcRow->GetString(0);
		    m_cConfigList[m_nCurSelConfig].m_sConfig.m_cStyle  = pcRow->GetString(1);
		    m_cConfigList[m_nCurSelConfig].m_bModified = true;



		    Font::GetBitmapSizes( pcRow->GetString(0), pcRow->GetString(1), &m_cBitmapSizes );
		    m_pcBitmapSizeList->Clear();

		    float vMinOffset = FLT_MAX;
		    int   nSelection = -1;
		    for ( uint i = 0 ; i < m_cBitmapSizes.size() ; ++i ) {
			char zName[64];
			float vOffset = fabs( m_cConfigList[m_nCurSelConfig].m_sConfig.m_vSize - m_cBitmapSizes[i] );
			if ( vOffset < vMinOffset ) {
			    vMinOffset = vOffset;
			    nSelection = i;
			}
			sprintf( zName, "%.2f", m_cBitmapSizes[i] );
			m_pcBitmapSizeList->AppendItem( zName );
		    }
		    if ( m_cBitmapSizes.size() > 0 ) {
			m_pcBitmapSizeList->SetSelection( nSelection, false );
		    } else {
			m_pcBitmapSizeList->SetCurrentString( "" );
		    }
//		    Font* pcFont = new Font( *m_pcTestView->GetEditor()->GetFont() ); // new Font();
		    Font* pcFont = m_pcTestView->GetEditor()->GetFont();
		    if ( pcFont->SetFamilyAndStyle( pcRow->GetString(0).c_str(), pcRow->GetString(1).c_str() ) != 0 ) {
			printf( "Failed to set family and style\n" );
			break;
		    }
		    Sync();
//		    m_pcTestView->SetFont( pcFont );
//		    m_pcTestView->GetEditor()->SetFont( pcFont );
		}
	    }
	    break;
	}
	case ID_BM_SIZE_CHANGED:
	{
	    int nSelection = m_pcBitmapSizeList->GetSelection();
	    if ( nSelection < 0 ) {
		break;
	    }
	    m_pcSizeSpinner->SetValue( m_cBitmapSizes[nSelection] );
	    break;
	}
	case ID_SIZE_CHANGED:
	{
	    Font* pcFont = m_pcTestView->GetEditor()->GetFont();

	    m_cConfigList[m_nCurSelConfig].m_sConfig.m_vSize = m_pcSizeSpinner->GetValue();
	    m_cConfigList[m_nCurSelConfig].m_bModified = true;


	    if ( m_cBitmapSizes.size() > 0 ) {
		float vMinOffset = FLT_MAX;
		int   nSelection = -1;
		for ( uint i = 0 ; i < m_cBitmapSizes.size() ; ++i ) {
		    float vOffset = fabs( m_cConfigList[m_nCurSelConfig].m_sConfig.m_vSize - m_cBitmapSizes[i] );
		    if ( vOffset < vMinOffset ) {
			vMinOffset = vOffset;
			nSelection = i;
		    }
		}
		m_pcBitmapSizeList->SetSelection( nSelection, false );
	    }
	    
	    if ( pcFont != NULL ) {
		Sync();
		pcFont->SetSize( m_pcSizeSpinner->GetValue() );
//		m_pcTestView->GetEditor()->SetFont( pcFont );
//		m_pcTestView->GetEditor()->SetFont( pcFont );
//		m_pcTestView->GetEditor()->FontChanged( pcFont );
	    }
	    break;
	}
	default:
	    View::HandleMessage( pcMessage );
	    break;
    }
}

