#include "statusbar.h"

using namespace os;

StatusBar::StatusBar( const os::Rect& cFrame, const std::string& cTitle ) : View( cFrame, cTitle )
{
    GetFontHeight( &m_sFontHeight );
}


Point StatusBar::GetPreferredSize( bool bLargest ) const
{
    if ( bLargest ) {
	return( Point( COORD_MAX, 12.0f + m_sFontHeight.ascender + m_sFontHeight.descender + m_sFontHeight.line_gap ) );
    } else {
	return( Point( 12.0f, 12.0f + m_sFontHeight.ascender + m_sFontHeight.descender + m_sFontHeight.line_gap ) );
    }
}

void StatusBar::FrameSized( const Point& cDelta )
{
    if ( cDelta.x < 0.0f ) {
	Rect cBounds = GetBounds();
	cBounds.left = cBounds.right - 5.0f;
	Invalidate( cBounds );
    } else if ( cDelta.x > 0.0f ) {
	Rect cBounds = GetBounds();
	cBounds.left = cBounds.right - 5.0f - cDelta.x;
	Invalidate( cBounds );
    }
    if ( cDelta.y < 0.0f ) {
	Rect cBounds = GetBounds();
	cBounds.top = cBounds.bottom - 5.0f;
	Invalidate( cBounds );
    } else if ( cDelta.y > 0.0f ) {
	Rect cBounds = GetBounds();
	cBounds.top = cBounds.bottom - 5.0f - cDelta.y;
	Invalidate( cBounds );
    }
    Flush();
}

void StatusBar::Paint( const Rect& /*cUpdateRect*/ )
{
    Rect cBounds = GetBounds();
    DrawFrame( cBounds, FRAME_RAISED );
    cBounds.Resize( 4.0f, 4.0f, -4.0f, -4.0f );
    DrawFrame( cBounds, FRAME_RECESSED );
    SetFgColor( 0, 0, 0 );
    DrawString( m_cStatusStr, Point( 8.0f, 6.0f + ceil( m_sFontHeight.ascender + m_sFontHeight.line_gap * 0.5f ) ) );
}

void StatusBar::SetStatus( const std::string& cText )
{
    m_cStatusStr = cText;
    Rect cBounds = GetBounds();
    cBounds.Resize( 6.0f, 6.0f, -6.0f, -6.0f );
    Invalidate( cBounds );
    Flush();
}
