#include "iconview.h"

#include <gui/bitmap.h>
#include <util/message.h>


IconView::IconView( const Rect& cFrame ) : View( cFrame, "icon_view" )
{
  m_pcBitmap = NULL;
}

void IconView::SetBitmap( Bitmap* pcBitmap )
{
  m_pcBitmap = pcBitmap;
  Paint( GetBounds() );
  Flush();
}

void IconView::Paint( const Rect& cUpdateRect )
{
    if ( m_pcBitmap == NULL ) {
	FillRect( cUpdateRect, Color32_s( 255, 255, 255, 0 ) );
	return;
    }
    uint8* pRaster = m_pcBitmap->LockRaster();
    Rect cBounds = m_pcBitmap->GetBounds();

    Rect cViewBounds = GetBounds();
    int nWidth  = int(cViewBounds.Width()) + 1;
    int nHeight = int(cViewBounds.Height()) + 1;

    Color32_s sBgCol = get_default_color( COL_NORMAL );
    if ( cBounds.Width() + 1 != nWidth )
    {
	int nScale = ( cBounds.Width() + 1 == 32 ) ? 8 : 16;
	SetFgColor( 0, 0, 0 );
    
	for ( int y = 0 ; y < cBounds.Height() + 1 ; ++y ) {
	    DrawLine( Point( 0, y * nScale + nScale - 1 ), Point( nWidth, y * nScale + nScale - 1 ) );
	    DrawLine( Point( y * nScale + nScale - 1, 0 ), Point( y * nScale + nScale - 1, nHeight ) );
	    for ( int x = 0 ; x < cBounds.Width() + 1 ; ++x ) {
		uint nAlpha = pRaster[3];
		uint nOpac  = 255 - nAlpha;
		Color32_s sColor( (uint(sBgCol.red) * nOpac + uint(pRaster[2]) * nAlpha) / 255,
				  (uint(sBgCol.green) * nOpac + uint(pRaster[1]) * nAlpha) / 255,
				  (uint(sBgCol.blue) * nOpac + uint(pRaster[0]) * nAlpha) / 255 );
//		Color32_s sColor( pRaster[2], pRaster[1], pRaster[0], pRaster[3] );
		FillRect( Rect( x*nScale, y*nScale, x*nScale + nScale - 2, y*nScale + nScale - 2 ), sColor );
		pRaster += 4;
	    }
	}
    } else {
	Rect cRect( m_pcBitmap->GetBounds() );
	DrawBitmap( m_pcBitmap, cRect, cRect );
    }
}

void IconView::MouseMove( const Point& cNewPos, int nCode, uint32 nButtons, Message* pcData )
{
}
void IconView::MouseDown( const Point& cPosition, uint32 nButtons )
{
}

void IconView::MouseUp( const Point& cPosition, uint32 nButtons, Message* pcData )
{
  if ( pcData != NULL ) {
    View::MouseUp( cPosition, nButtons, pcData );
    return;
  }
}

void IconView::KeyDown( int nChar, uint32 nQualifiers )
{
}
  
