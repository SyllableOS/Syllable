#include "searchdialog.h"

#include <gui/button.h>
#include <gui/textview.h>
#include <gui/checkbox.h>

enum { ID_SEARCH, ID_CONTINUE, ID_SEARCH_STR };

using namespace os;

static bool g_bCaseSensitive = false;
static std::string g_cSearchStr;

FindDialogView::FindDialogView( const os::Rect& cFrame, const os::Messenger& cMsgTarget, const Message& cSearchMsg, const Message& cCloseMsg ) :
	LayoutView( cFrame, "search_dialog", NULL, CF_FOLLOW_ALL ), m_cMsgTarget(cMsgTarget), m_cSearchMsg(cSearchMsg), m_cCloseMsg(cCloseMsg)
{
    VLayoutNode* pcRoot = new VLayoutNode( "root" );

    m_pcSearchStr   = new TextView( Rect(), "src_str", g_cSearchStr.c_str() );
    m_pcSearchBut   = new Button( Rect(), "src_but", "Search", new Message( ID_SEARCH ) );
    m_pcContinueBut = new Button( Rect(), "cont_but", "Repeat", new Message( ID_CONTINUE ) );
    m_pcCloseBut    = new Button( Rect(), "close_but", "Close", new Message( M_QUIT ) );
    m_pcCaseSensCB  = new CheckBox( Rect(), "case_sens_cb", "Case sensitive", NULL );


    m_pcSearchStr->SetEventMask( TextView::EI_ESC_PRESSED );
    m_pcSearchStr->SetMessage( new Message( ID_SEARCH_STR ) );
    if ( g_cSearchStr.empty() == false ) {
	m_pcSearchStr->SetCursor( -1, 0 );
	m_pcSearchStr->SelectAll();
    }

    m_pcCaseSensCB->SetValue( g_bCaseSensitive );
    
    HLayoutNode* pcButtonPanel = new HLayoutNode( "but_panel" );
    HLayoutNode* pcCBPanel = new HLayoutNode( "cb_panel" );

    pcCBPanel->AddChild( m_pcCaseSensCB );
    pcCBPanel->AddChild( new HLayoutSpacer( "space" ) );
    pcCBPanel->SetBorders( Rect( 10.0f, 0.0f, 10.0f, 10.0f ) );
    
    pcButtonPanel->AddChild( new HLayoutSpacer( "space" ) );
    pcButtonPanel->AddChild( m_pcSearchBut )->SetBorders( Rect( 20.0f, 0.0f, 0.0f, 10.0f ) );
    pcButtonPanel->AddChild( m_pcContinueBut )->SetBorders( Rect( 10.0f, 0.0f, 0.0f, 10.0f ) );
    pcButtonPanel->AddChild( m_pcCloseBut )->SetBorders( Rect( 10.0f, 0.0f, 10.0f, 10.0f ) );

    pcRoot->AddChild( m_pcSearchStr )->SetBorders( Rect( 10.0f, 10.0f, 10.0f, 10.0f ) );
    pcRoot->AddChild( pcCBPanel );
    pcRoot->AddChild( pcButtonPanel );

    SetRoot( pcRoot );
}

FindDialogView::~FindDialogView()
{
    g_bCaseSensitive = m_pcCaseSensCB->GetValue();
    g_cSearchStr     = m_pcSearchStr->GetBuffer()[0];
    m_cMsgTarget.SendMessage( &m_cCloseMsg );
}

void FindDialogView::AllAttached()
{
    m_pcSearchStr->SetTarget( this );
    m_pcSearchBut->SetTarget( this );
    m_pcContinueBut->SetTarget( this );
    m_pcCloseBut->SetTarget( this );

    m_pcSearchStr->MakeFocus();
    GetWindow()->SetDefaultButton( m_pcContinueBut );
}

void FindDialogView::KeyDown( const char* pzString, const char* pzRawString, uint32 nQualifiers )
{
    if ( pzString[0] == VK_ESCAPE ) {
	GetWindow()->PostMessage( M_QUIT );
    } else {
	LayoutView::KeyDown( pzString, pzRawString, nQualifiers );
    }
}

void FindDialogView::HandleMessage( Message* pcMessage )
{
    switch( pcMessage->GetCode() )
    {
	case ID_SEARCH:
	{
	    Message cMsg( m_cSearchMsg );
	    cMsg.AddBool( "from_top", true );
	    cMsg.AddBool( "case_sensitive", m_pcCaseSensCB->GetValue() );
	    cMsg.AddString( "string", m_pcSearchStr->GetBuffer()[0] );
	    m_cMsgTarget.SendMessage( &cMsg );
	    break;
	}
	case ID_CONTINUE:
	{
	    Message cMsg( m_cSearchMsg );
	    cMsg.AddBool( "from_top", false );
	    cMsg.AddBool( "case_sensitive", m_pcCaseSensCB->GetValue() );
	    cMsg.AddString( "string", m_pcSearchStr->GetBuffer()[0] );
	    m_cMsgTarget.SendMessage( &cMsg );
	    break;
	}
	case ID_SEARCH_STR:
	{
	    int32 nEvents = 0;
	    pcMessage->FindInt( "events", &nEvents );
	    if ( nEvents & TextView::EI_ESC_PRESSED ) {
		GetWindow()->PostMessage( M_QUIT );
	    }
	}
	default:
	    LayoutView::HandleMessage( pcMessage );
	    break;
    }
}

FindDialog::FindDialog( const os::Messenger& cMsgTarget, const Message& cSearchMsg, const Message& cCloseMsg ) :
	Window( Rect(0,0,0,0), "search_dlg", "ABrowse Search:" )
{
    m_pcTopView = new FindDialogView( GetBounds(), cMsgTarget, cSearchMsg, cCloseMsg );
    AddChild( m_pcTopView );
    
    Point cMinSize = m_pcTopView->GetPreferredSize( false );
    Point cMaxSize = m_pcTopView->GetPreferredSize( true );

    Point cMousePos;
    m_pcTopView->GetMouse( &cMousePos, NULL );
    m_pcTopView->ConvertToScreen( &cMousePos );

    SetSizeLimits( cMinSize, cMaxSize );
    
    
//    cMinSize.x = cMinSize.y * 3.0f;
    ResizeTo( cMinSize );
    MoveTo( floor( cMousePos.x - cMinSize.x * 0.5f ), floor( cMousePos.y - cMinSize.y * 0.5f ) );
}

FindDialog::~FindDialog()
{
}
