#include "downloaddialog.h"

#include <atheos/time.h>

#include <util/message.h>

#include <gui/button.h>
#include <gui/stringview.h>
#include <gui/progressbar.h>
#include <gui/filerequester.h>
#include <gui/font.h>

#include <vector>

using namespace os;

enum { ID_OK = 1, ID_CANCEL };

std::string DownloadRequesterView::s_cDownloadPath;

static String get_size_str( off_t nSize )
{
    if ( nSize < 1024 ) {
	return( String().Format( "%Ld", nSize ) );
    } else if ( nSize < 1024LL * 1024LL ) {
	return( String().Format( "%.2fKB", double(nSize) / 1024.0 ) );
    } else {
	return( String().Format( "%.2fMB", double(nSize) / (1024.0*1024.0) ) );
    }
}


class StatusView : public View
{
public:
    StatusView( const Rect& cFrame, const std::string& cTitle );

    int  AddLabel( const std::string& cLabel, const std::string& cText );
    void SetText( int nIndex, const std::string& cText );

    virtual void  Paint( const Rect& cUpdateRect );
    virtual Point GetPreferredSize( bool bLargest ) const;
    
private:
    float 	m_vLabelWidth;
    font_height m_sFontHeight;
    float	m_vGlyphHeight;
    std::vector<std::pair<std::string,std::string> > m_cStrList;
};

StatusView::StatusView( const Rect& cFrame, const std::string& cTitle ) : View( cFrame, cTitle )
{
    GetFontHeight( &m_sFontHeight );
    m_vGlyphHeight = ceil( (m_sFontHeight.ascender + m_sFontHeight.descender + m_sFontHeight.line_gap ) * 1.2f );
    m_vLabelWidth = 0.0f;
}

int StatusView::AddLabel( const std::string& cLabel, const std::string& cText )
{
    m_cStrList.push_back( std::make_pair( cLabel, cText ) );
    float vLabelWidth = GetStringWidth( cLabel );
    if ( vLabelWidth > m_vLabelWidth ) {
	m_vLabelWidth = vLabelWidth;
    }
    return( m_cStrList.size() - 1 );
}

void StatusView::SetText( int nIndex, const std::string& cText )
{
    m_cStrList[nIndex].second = cText;
    Invalidate( Rect( m_vLabelWidth, float(nIndex) * m_vGlyphHeight, COORD_MAX, float(nIndex+1) * m_vGlyphHeight ) );
    Flush();
}

Point StatusView::GetPreferredSize( bool bLargest ) const
{
    if ( bLargest ) {
	return( Point( COORD_MAX, m_vGlyphHeight * float(m_cStrList.size()) ) );
    } else {
	return( Point( m_vLabelWidth, m_vGlyphHeight * float(m_cStrList.size()) ) );
    }
}

void StatusView::Paint( const Rect& /*cUpdateRect*/ )
{
    float y = ceil( m_sFontHeight.ascender );

    FillRect( GetBounds(), get_default_color( COL_NORMAL ) );
    SetFgColor( 0, 0, 0 );
    for ( uint i = 0 ; i < m_cStrList.size() ; ++i ) {
	DrawString( m_cStrList[i].first, Point( 0.0f, y ) );
	DrawString( m_cStrList[i].second, Point( m_vLabelWidth + m_vGlyphHeight, y ) );
	y += m_vGlyphHeight;
    }
}


DownloadRequesterView::DownloadRequesterView( const Rect&          cFrame,
					      const std::string&   cURL,
					      const std::string&   cPreferredName,
					      const std::string&   cType,
					      off_t		   nContentSize,
					      const os::Messenger& cMsgTarget,
					      const os::Message&   cOkMsg,
					      const os::Message&   cCancelMessage ) :
	LayoutView( cFrame, "download_diag", NULL, CF_FOLLOW_ALL ),
	m_cMsgTarget( cMsgTarget ), m_cOkMsg( cOkMsg ), m_cCancelMsg( cCancelMessage ), m_cPreferredName(cPreferredName)
{
    VLayoutNode* pcRoot = new VLayoutNode( "root" );

    m_pcTermMsg = &m_cCancelMsg;
    
    m_pcOkBut = new Button( Rect(0,0,0,0), "ok", "Ok", new Message( ID_OK ) );
    m_pcCancelBut = new Button( Rect(0,0,0,0), "cancel", "Cancel", new Message( ID_CANCEL ) );
    
    HLayoutNode* pcButtonBar = new HLayoutNode( "button_bar" );

    StatusView* m_pcStatusView = new StatusView( Rect(), "status_view" );

    m_pcStatusView->AddLabel( "Location:", cURL );
    m_pcStatusView->AddLabel( "File type:", cType );
    if ( nContentSize != -1 ) {
	m_pcStatusView->AddLabel( "File size:", get_size_str( nContentSize ) );
    } else {
	m_pcStatusView->AddLabel( "File size:", "UNKNOWN" );
    }
    
    
    pcButtonBar->AddChild( new HLayoutSpacer( "space" ) );
    pcButtonBar->AddChild( m_pcOkBut )->SetBorders( Rect( 0, 0, 10, 10 ) );
    pcButtonBar->AddChild( m_pcCancelBut )->SetBorders( Rect( 0, 0, 10, 10 ) );
    
    pcRoot->AddChild( new VLayoutSpacer( "space" ) );
    pcRoot->AddChild( m_pcStatusView )->SetBorders( Rect( 10.0f, 5.0f, 5.0f, 5.0f ) );
    pcRoot->AddChild( new VLayoutSpacer( "space" ) );
    pcRoot->AddChild( pcButtonBar );

    pcRoot->SameWidth( "ok", "cancel", NULL );
    
    SetRoot( pcRoot );
}

DownloadRequesterView::~DownloadRequesterView()
{
    if ( m_pcTermMsg != NULL ) {
	m_cMsgTarget.SendMessage( m_pcTermMsg );
    }
}

void DownloadRequesterView::AllAttached()
{
    m_pcOkBut->SetTarget( this );
    m_pcCancelBut->SetTarget( this );
}

void DownloadRequesterView::HandleMessage( os::Message* pcMessage )
{
    switch( pcMessage->GetCode() )
    {
	case ID_OK:
	{
	    if ( s_cDownloadPath.empty() ) {
		const char* pzHome = getenv( "HOME" );
		if ( pzHome != NULL ) {
		    s_cDownloadPath = pzHome;
		    if ( s_cDownloadPath[s_cDownloadPath.size()-1] != '/' ) {
			s_cDownloadPath += '/';
		    }
		} else {
		    s_cDownloadPath = "/tmp/";
		}
	    }
	    m_pcFileReq = new FileRequester( FileRequester::SAVE_REQ,
					     new Messenger( this ),
					     (s_cDownloadPath + m_cPreferredName).c_str(),
					     FileRequester::NODE_FILE,
					     false );
	    m_pcFileReq->Show();
	    m_pcFileReq->MakeFocus();
	    GetWindow()->Hide();
	    break;
	}
	case ID_CANCEL:
	    m_pcTermMsg = &m_cCancelMsg;
//	    m_cMsgTarget.SendMessage( &m_cCancelMsg );
	    GetWindow()->PostMessage( M_QUIT );
	    break;
	case M_SAVE_REQUESTED:
	{
	    if ( m_pcFileReq != NULL ) {
		s_cDownloadPath = m_pcFileReq->GetPath();
		if ( s_cDownloadPath[s_cDownloadPath.size()-1] != '/' ) {
		    s_cDownloadPath += '/';
		}
		m_pcFileReq->PostMessage( M_QUIT );
		m_pcFileReq = NULL;
	    }
	    std::string cPath;
	    if ( pcMessage->FindString( "file/path", &cPath ) == 0 ) {
		m_cOkMsg.AddString( "path", cPath );
		m_pcTermMsg = &m_cOkMsg;
//		m_cMsgTarget.SendMessage( &m_cOkMsg );
	    } else {
		m_pcTermMsg = &m_cCancelMsg;
//		m_cMsgTarget.SendMessage( &m_cCancelMsg );
	    }
	    GetWindow()->PostMessage( M_QUIT );
	    break;
	}
	default:
	    View::HandleMessage( pcMessage );
	    break;
    }
}



DownloadRequester::DownloadRequester( const Rect& cFrame,
				      const std::string& cTitle,
				      const std::string& cURL,
				      const std::string& cPreferredName,
				      const std::string& cType,
				      off_t		 nContentSize,
				      const Messenger&   cMsgTarget,
				      const Message&     cOkMsg,
				      const Message&     cCancelMessage ) :
	Window( cFrame, "donwload_dialog", cTitle )
{
    m_pcTopView = new DownloadRequesterView( GetBounds(), cURL, cPreferredName, cType, nContentSize, cMsgTarget, cOkMsg, cCancelMessage );
    AddChild( m_pcTopView );
}

DownloadRequester::~DownloadRequester()
{
}

















DownloadProgressView::DownloadProgressView( const Rect& cFrame,
					    const std::string& cURL,
					    const std::string& cPath,
					    off_t nTotalSize,
					    bigtime_t nStartTime,
					    const os::Messenger& cMsgTarget,
					    const os::Message&   cCancelMessage ) :
	LayoutView( cFrame, "download_progress_diag", NULL, CF_FOLLOW_ALL ),
	m_cMsgTarget( cMsgTarget ), m_cCancelMsg( cCancelMessage )
{
    m_nTotalSize = nTotalSize;
    m_nStartTime = nStartTime;
    m_pcTermMsg  = &m_cCancelMsg;
    
    VLayoutNode* pcRoot = new VLayoutNode( "root" );

    m_pcProgressBar = new ProgressBar( Rect(0,0,0,0), "progress_bar" );
    
    m_pcCancelBut = new Button( Rect(0,0,0,0), "cancel", "Cancel", new Message( ID_CANCEL ) );
    
    HLayoutNode* pcButtonBar    = new HLayoutNode( "button_bar" );

    m_pcStatusView = new StatusView( Rect(), "status_view" );

    m_pcStatusView->AddLabel( "Download to:", cPath );
    m_pcStatusView->AddLabel( "Progress:", (m_nTotalSize==-1) ? "0" : String().Format( "0/%s", get_size_str( m_nTotalSize ).c_str() ) );
    m_pcStatusView->AddLabel( "Transfer rate:", "" );
    
    
    pcButtonBar->AddChild( new HLayoutSpacer( "space" ) );
    pcButtonBar->AddChild( m_pcCancelBut )->SetBorders( Rect( 0, 0, 10, 10 ) );
    
    pcRoot->AddChild( new VLayoutSpacer( "space" ) );
    pcRoot->AddChild( new StringView( Rect(), "", "Saving:" ) );
    pcRoot->AddChild( new StringView( Rect(), "", cURL.c_str() ) )->SetBorders( Rect( 10.0f, 5.0f, 10.0f, 5.0f ) );
    pcRoot->AddChild( m_pcProgressBar )->SetBorders( Rect( 10.0f, 5.0f, 10.0f, 5.0f ) );
    pcRoot->AddChild( new VLayoutSpacer( "space" ) );
    pcRoot->AddChild( m_pcStatusView )->SetBorders( Rect( 10.0f, 5.0f, 5.0f, 5.0f ) );
    pcRoot->AddChild( new VLayoutSpacer( "space" ) );
    pcRoot->AddChild( pcButtonBar );
    
    SetRoot( pcRoot );
}

DownloadProgressView::~DownloadProgressView()
{
    if ( m_pcTermMsg != NULL ) {
	m_cMsgTarget.SendMessage( m_pcTermMsg );
    }
}

void DownloadProgressView::AllAttached()
{
    m_pcCancelBut->SetTarget( this );
}

void DownloadProgressView::HandleMessage( os::Message* pcMessage )
{
    switch( pcMessage->GetCode() )
    {
	case ID_CANCEL:
	    m_cMsgTarget.SendMessage( &m_cCancelMsg );
	    GetWindow()->PostMessage( M_QUIT );
	    break;
	default:
	    LayoutView::HandleMessage( pcMessage );
	    break;
    }
}

void DownloadProgressView::UpdateProgress( off_t nBytesReceived )
{
    double vBytesPerSec = double(nBytesReceived) / (double(get_system_time() - m_nStartTime) / 1000000.0);

    os::String cSpeedStr;
    if ( vBytesPerSec < 1024.0 ) {
	cSpeedStr.Format( "%.2f bytes/s", vBytesPerSec );
    } else {
	cSpeedStr.Format( "%.2f KB/s", vBytesPerSec / 1024.0 );
    }
    m_pcStatusView->SetText( 2, cSpeedStr );
    if ( m_nTotalSize != -1 ) {
	int nETA = int(double(m_nTotalSize - nBytesReceived) / vBytesPerSec);
	String cETA;
	if ( nETA < 60 ) {
	    cETA.Format( "%02d seconds", nETA );
	} else if ( nETA < 60*60 ) {
	    cETA.Format( "%d:%02d minutes", nETA/60, nETA % 60 );
	} else {
	    int nHours = nETA / (60*60);
	    nETA -= nHours * 60*60;
	    cETA.Format( "%d:%02d:%02d hours", nHours, nETA/60, nETA % 60 );
	}
	m_pcStatusView->SetText( 1, String().Format( "%s / %s (ETA: %s)",
						     get_size_str( nBytesReceived ).c_str(),
						     get_size_str( m_nTotalSize ).c_str(),
						     cETA.c_str() ) );
	m_pcProgressBar->SetProgress( double(nBytesReceived) / double(m_nTotalSize) );
    } else {
	m_pcStatusView->SetText( 1, get_size_str( nBytesReceived ).c_str() );
    }
    GetRoot()->Layout();
}


DownloadProgress::DownloadProgress( const Rect& cFrame,
				    const std::string& cTitle,
				    const std::string& cURL,
				    const std::string& cPath,
				    off_t nTotalSize,
				    bigtime_t nStartTime,
				    const Messenger& cMsgTarget,
				    const Message& cCancelMessage ) :
	Window( cFrame, "donwload_progress_dialog", cTitle ), m_cTitle(cTitle)
{
    m_nTotalSize = nTotalSize;
    m_pcTopView = new DownloadProgressView( GetBounds(), cURL, cPath, nTotalSize, nStartTime, cMsgTarget, cCancelMessage );
    AddChild( m_pcTopView );
}

DownloadProgress::~DownloadProgress()
{
}

void DownloadProgress::UpdateProgress( off_t nBytesReceived )
{
    Lock();
    m_pcTopView->UpdateProgress( nBytesReceived );
    SetTitle( String().Format( "(%Ld%%) - %s", nBytesReceived * 100 / m_nTotalSize, m_cTitle.c_str() ) );
    Unlock();
}

void DownloadProgress::Terminate()
{
    if ( SafeLock() >= 0 ) {
	m_pcTopView->ClearTermMsg();
	PostMessage( M_QUIT );
	Unlock();
    }
}
