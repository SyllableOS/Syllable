#include <assert.h>

#include "toolbar.h"
#include "utils.h"

#include <gui/bitmap.h>
#include <util/resources.h>

using namespace os;


#define BUTTON_WIDTH  20.0f
#define BUTTON_HEIGHT 20.0f


ToolButton::ToolButton( const std::string& cImagePath, const os::Message& cMsg ) : m_cMessage( cMsg )
{
    try {
	Resources cRes( get_image_id() );
	ResStream* pcStream = cRes.GetResourceStream( cImagePath );
	if ( pcStream == NULL ) {
	    printf( "Error: failed to load resource '%s'\n", cImagePath.c_str() );
	} else {
	    m_pcImage = load_bitmap( pcStream );
	    delete pcStream;
	}
    } catch( ... ) {
	m_pcImage = NULL;
    }
    m_bIsEnabled = true;
}

ToolButton::~ToolButton()
{
    delete m_pcImage;
}


ToolBar::ToolBar( const os::Rect& cFrame, const std::string& cName ) : View( cFrame, cName )
{
    m_nSelectedButton = -1;
}

void ToolBar::Paint( const Rect& cUpdateRect )
{
    Rect cFrame( 0.0f, 0.0f, 15.0f, 15.0f );

    cFrame += Point( (BUTTON_WIDTH - 16.0f) * 0.5f, (BUTTON_HEIGHT - 16.0f) * 0.5f );

    SetDrawingMode( DM_COPY );
    FillRect( cUpdateRect, get_default_color( COL_NORMAL ) );
    SetDrawingMode( DM_BLEND );
    for ( uint i = 0 ; i < m_cButtons.size() ; ++i ) {
	if ( m_cButtons[i]->m_pcImage != NULL ) {
	    if ( int(i) == m_nSelectedButton ) {
		DrawBitmap( m_cButtons[i]->m_pcImage, cFrame.Bounds(), cFrame + Point( 2.0f, 2.0f ) );
	    } else {
		DrawBitmap( m_cButtons[i]->m_pcImage, cFrame.Bounds(), cFrame );
	    }
	    if ( m_cButtons[i]->m_bIsEnabled == false ) {
		SetFgColor( get_default_color( COL_NORMAL ) );
		for ( float y = cFrame.top ; y <= cFrame.bottom ; y += 2.0f ) {
		    for ( float x = cFrame.left + float(int(y) % 2) ; x <= cFrame.right ; x += 2.0f ) {
			DrawLine( Point( x, y ), Point( x, y ) );
		    }
		}
	    }
	} else {
	    FillRect( cFrame );
	}
	cFrame += Point( BUTTON_WIDTH, 0.0f );
    }
}

Rect ToolBar::GetButtonRect( int nIndex, bool bFull )
{
    Rect cFrame( 0.0f, 0.0f, BUTTON_WIDTH - 1.0f, BUTTON_HEIGHT - 1.0f );
    cFrame += Point( BUTTON_WIDTH * float(nIndex), 0.0f );
    if ( bFull == false ) {
	cFrame.Resize( -(BUTTON_WIDTH - 16.0f) * 0.5f, -(BUTTON_HEIGHT - 16.0f) * 0.5f, (BUTTON_WIDTH - 16.0f) * 0.5f, (BUTTON_HEIGHT - 16.0f) * 0.5f );
    }
    return( cFrame );
}

void ToolBar::MouseMove( const Point& /*cNewPos*/, int /*nCode*/, uint32 /*nButtons*/, Message* /*pcData*/ )
{
}

void ToolBar::MouseDown( const Point& cPosition, uint32 nButton )
{
    if ( nButton != 1 ) {
	return;
    }
    Rect cFrame( 0.0f, 0.0f, 15.0f, 15.0f );

    cFrame += Point( (BUTTON_WIDTH - 16.0f) * 0.5f, (BUTTON_HEIGHT - 16.0f) * 0.5f );
    for ( uint i = 0 ; i < m_cButtons.size() ; ++i ) {
	if ( cFrame.DoIntersect( cPosition )  ) {
	    if ( m_cButtons[i]->m_bIsEnabled ) {
		m_nSelectedButton = i;
	    }
	    break;
	}
	cFrame += Point( BUTTON_WIDTH, 0.0f );
    }
    if ( m_nSelectedButton != -1 ) {
	Invalidate( GetButtonRect( m_nSelectedButton, true ) );
	Flush();
    }
}

void ToolBar::MouseUp( const Point& cPosition, uint32 nButton, Message* /*pcData*/ )
{
    if ( nButton != 1 ) {
	return;
    }
    if ( m_nSelectedButton != -1 ) {
	Invalidate( GetButtonRect( m_nSelectedButton, true ) );
	Flush();
	if ( GetButtonRect( m_nSelectedButton, false ).DoIntersect( cPosition ) && m_cTarget.IsValid() ) {
	    m_cTarget.SendMessage( &m_cButtons[m_nSelectedButton]->m_cMessage );
	}
	m_nSelectedButton = -1;
    }
}

Point ToolBar::GetPreferredSize( bool /*bLargest*/ ) const
{
    return( Point( BUTTON_WIDTH * float(m_cButtons.size()) - 1.0f, BUTTON_HEIGHT - 1.0f ) );
}

void ToolBar::SetTarget( const os::Messenger& cTarget )
{
    m_cTarget = cTarget;
}

void ToolBar::AddButton( const std::string& cImagePath, const os::Message& cMsg )
{
    m_cButtons.push_back( new ToolButton( cImagePath, cMsg ) );
}

void ToolBar::EnableButton( int nIndex, bool bEnabled )
{
    assert( nIndex >= 0 && nIndex < int(m_cButtons.size()) );
    m_cButtons[nIndex]->m_bIsEnabled = bEnabled;
    Invalidate( GetButtonRect( nIndex, true ) );
}
