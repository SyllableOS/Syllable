#include <gui/bitmap.h>
#include <gui/guidefines.h>
#include <util/message.h>
#include <util/variant.h>
#include <gui/image.h>
#include <gui/font.h>

#include <math.h>

#include "charmap.h"

using namespace os;

class CharMapView::Private
{
	public:
	Private() {
		m_nLine = 0;
		m_pcMouseOverMsg = NULL;
		m_pcScrollBarMsg = NULL;
		m_cMPos.x = 9999;
		m_cSPos.x = 9999;
	}
	~Private() {
		if( m_pcScrollBarMsg )
			delete m_pcScrollBarMsg;
		if( m_pcMouseOverMsg )
			delete m_pcMouseOverMsg;
	}

	public:
	int			m_nLine;
	Message*	m_pcMouseOverMsg;
	Message*	m_pcScrollBarMsg;
	IPoint		m_cMPos;
	IPoint		m_cSPos;
	int			m_nCols;
	int			m_nRows;
	std::vector<uint32> m_cMap;
};

CharMapView::CharMapView( const Rect& cRect, const String& cName, Message* pcMsg, uint32 nResizeMask, uint32 nFlags )
:Control( cRect, cName, "", pcMsg, nResizeMask, nFlags )
{
	m = new Private;

	FontChanged( GetFont() );
	FrameSized( Point() );
}

CharMapView::~CharMapView()
{
	delete m;
}

void CharMapView::SetMouseOverMessage( Message* pcMsg )
{
	if( m->m_pcMouseOverMsg )
		delete m->m_pcMouseOverMsg;
	m->m_pcMouseOverMsg = pcMsg;
}

void CharMapView::SetScrollBarChangeMessage( os::Message* pcMsg )
{
	if( m->m_pcScrollBarMsg )
		delete m->m_pcScrollBarMsg;
	m->m_pcScrollBarMsg = pcMsg;
}

void CharMapView::AttachedToWindow()
{
	SetBgColor( GetParent()->GetBgColor(  ) );
	FontChanged( GetFont() );
	FrameSized( Point() );
	View::AttachedToWindow();
}

void CharMapView::Paint( const Rect& cUpdateRect )
{
	_Render( this, cUpdateRect );
}

void CharMapView::FrameSized( const Point & cDelta )
{
	Point cEM = GetTextExtent( "M" );
	float vSq = std::max( cEM.x, cEM.y );
	int nIndex = m->m_nLine*m->m_nCols;
	m->m_nCols = int( GetBounds().Width() / (vSq*1.5) );
	m->m_nRows = int( GetBounds().Height() / (vSq*1.5) );
	m->m_nLine = nIndex / m->m_nCols;
	_UpdateScrollBar();
}

void CharMapView::FontChanged( Font* pcNewFont )
{
	std::vector<uint32> cChars;

	cChars = pcNewFont->GetSupportedCharacters();
	
	SetCharMap( cChars );
	FrameSized( Point() );
	_UpdateScrollBar();
}

void CharMapView::_UpdateScrollBar()
{
	if( m->m_pcScrollBarMsg ) {
		Message *pcMsg = new Message( *m->m_pcScrollBarMsg );
		pcMsg->AddInt32( "total", GetNumberOfRows() );
		pcMsg->AddInt32( "current", m->m_nLine );
		pcMsg->AddInt32( "height", m->m_nRows );
		Invoke( pcMsg );
	}
}

Point CharMapView::GetPreferredSize( bool bLargest ) const
{
	if( bLargest )  {
		return Point( COORD_MAX, COORD_MAX );
	} else {
		return Point( 10, 10 );
	}
}

uint32 CharMapView::GetNumberOfRows() const
{
	return ( m->m_cMap.size() / m->m_nCols );
}

void CharMapView::SetCharMap( std::vector<uint32> cMap )
{
	m->m_cMap = cMap;
	SetLine( 0 );
}

void CharMapView::SetLine( int nLine )
{
	m->m_nLine = nLine;
	Invalidate();
	Flush();
}

IPoint CharMapView::GetCharPosition( const Point& cPos )
{
	IPoint cRes;
	Rect cBounds( GetBounds() );
	float vCW, vCH;

	vCW = ( cBounds.Width() + 1 ) / m->m_nCols;
	vCH = ( cBounds.Height() + 1 ) / m->m_nRows;

	cRes.x = int(cPos.x / vCW);
	cRes.y = int(cPos.y / vCH);

	return cRes;
}

uint32 CharMapView::GetCharAtPosition( const IPoint& cPos )
{
	uint32 nIndex = (m->m_nLine + cPos.y ) * m->m_nCols + cPos.x;

	if( nIndex < m->m_cMap.size() )
		return m->m_cMap[nIndex];
	else
		return 0;
}

void CharMapView::MouseMove( const Point& cNewPos, int nCode, uint32 nButtons, Message* pcData )
{
	if( nCode == MOUSE_EXITED ) {
		_Invalidate( m->m_cMPos );
		m->m_cMPos.x = 9999;
		_Invalidate( m->m_cSPos );
		m->m_cSPos.x = 9999;
		Flush();
		if( m->m_pcMouseOverMsg ) {
			Message *pcMsg = new Message( *m->m_pcMouseOverMsg );
			Invoke( pcMsg );
		}
	} else {
		IPoint cPos = GetCharPosition( cNewPos );

		if( m->m_cMPos != cPos ) {
			IPoint cOld( m->m_cMPos );

			m->m_cMPos = cPos;
			_Invalidate( cPos );
			_Invalidate( cOld );
			Flush();
	
			if( m->m_pcMouseOverMsg ) {
				Message *pcMsg = new Message( *m->m_pcMouseOverMsg );
				uint32 nChar = GetCharAtPosition( cPos );

				char zTitle[ 8 ];
				zTitle[unicode_to_utf8( zTitle, nChar )] = 0;
				pcMsg->AddString( "utf-8", zTitle );
				pcMsg->AddInt32( "unicode", nChar );
				Invoke( pcMsg );
			}
		}
	}
}

void CharMapView::MouseDown( const Point& cPosition, uint32 nButtons )
{
	if( nButtons == 1 ) {
		m->m_cSPos = m->m_cMPos;
		_Invalidate( m->m_cSPos );
		Flush();
	}
}

void CharMapView::MouseUp( const Point& cPosition, uint32 nButtons, Message* pcData )
{
	if( m->m_cSPos.x != 9999 ) {
		Message *pcMsg = new Message( *(GetMessage()) );
		uint32 nChar = GetCharAtPosition( m->m_cSPos );

		char zTitle[ 8 ];
		zTitle[unicode_to_utf8( zTitle, nChar )] = 0;
		pcMsg->AddString( "utf-8", zTitle );
		pcMsg->AddInt32( "unicode", nChar );
		Invoke( pcMsg );
	}
	_Invalidate( m->m_cSPos );
	m->m_cSPos.x = 9999;
	Flush();
}

void CharMapView::_MakeBuffer( )
{
}

/***************************************************************************************************/

void CharMapView::_Invalidate( IPoint cBlock )
{
	Rect cBounds( GetBounds() );
	float vCW = ( cBounds.Width() + 1 ) / m->m_nCols;
	float vCH = ( cBounds.Height() + 1 ) / m->m_nRows;
	Invalidate( Rect( cBlock.x * vCW, cBlock.y * vCH, ( cBlock.x+1 ) * vCW, ( cBlock.y+1 ) * vCH ) );
}

void CharMapView::_Render( View* pcView, const Rect& cUpdateRect )
{
	int x, y;
	int xstart, ystart;
	int xstop, ystop;
	Rect cBounds( pcView->GetBounds() );
	float vCW, vCH;

	vCW = ( cBounds.Width() + 1 ) / m->m_nCols;
	vCH = ( cBounds.Height() + 1 ) / m->m_nRows;

	xstart = int( ( cUpdateRect.left - cBounds.left ) / vCW );
	ystart = int( ( cUpdateRect.top - cBounds.top ) / vCH );

	xstop = int( ( cUpdateRect.right - cBounds.left ) / vCW + 1 );
	ystop = int( ( cUpdateRect.bottom - cBounds.top ) / vCH + 1 );

	for( y = ystart; y < ystop; y++ ) {
		for( x = xstart; x < xstop; x++ ) {
			uint32 nChar = GetCharAtPosition( IPoint( x, y ) );
			char zTitle[ 8 ];
			zTitle[unicode_to_utf8( zTitle, nChar )] = 0;
			Rect cCellBounds( x * vCW, y * vCH, ( x + 1 ) * vCW - 1, ( y + 1 ) * vCH - 1 );
			if( x == m->m_cSPos.x && y == m->m_cSPos.y ) {
				pcView->DrawFrame( cCellBounds, FRAME_THIN | FRAME_RECESSED );
			} else {
				pcView->DrawFrame( cCellBounds, FRAME_THIN );
			}
			if( x == m->m_cMPos.x && y == m->m_cMPos.y ) {
				pcView->SetFgColor( Color32( 0x0, 0xAA, 0xFF ) );
				Rect cBorder( cCellBounds );
				cBorder.Resize( 4, 4, -4, -4 );
				pcView->MovePenTo( Point( cBorder.left+2, cBorder.top ) );
				pcView->DrawLine( Point( cBorder.right-2, cBorder.top ) );
				pcView->DrawLine( Point( cBorder.right, cBorder.top+2 ) );
				pcView->DrawLine( Point( cBorder.right, cBorder.bottom-2 ) );
				pcView->DrawLine( Point( cBorder.right-2, cBorder.bottom ) );
				pcView->DrawLine( Point( cBorder.left+2, cBorder.bottom ) );
				pcView->DrawLine( Point( cBorder.left, cBorder.bottom-2 ) );
				pcView->DrawLine( Point( cBorder.left, cBorder.top+2 ) );
				pcView->DrawLine( Point( cBorder.left+2, cBorder.top ) );
			}
			pcView->SetFgColor( Color32( 0x0, 0x0, 0x0 ) );
			pcView->DrawText( cCellBounds, zTitle, DTF_CENTER );
		}
	}
}

/***************************************************************************************************/

void CharMapView::_reserved1() {}
void CharMapView::_reserved2() {}
void CharMapView::_reserved3() {}
void CharMapView::_reserved4() {}
void CharMapView::_reserved5() {}
