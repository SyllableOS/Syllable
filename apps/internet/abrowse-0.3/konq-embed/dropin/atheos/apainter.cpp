#include <stdio.h>
#include <assert.h>

#include "apainter.h"
#include <qwidget.h>
#include "apixmap.h"

#include <gui/bitmap.h>
#include <gui/font.h>

#include <stack>

#define BULLET_WIDHT  6
#define BULLET_HEIGHT 6

uint8 g_anBullet8[] = {
    0xff,0xff,0xff,0x00,0x00,0xff,0xff,0xff,
    0xff,0xff,0x00,0x00,0x00,0x00,0xff,0xff,
    0xff,0x00,0x00,0x00,0x00,0x00,0x00,0xff,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0xff,0x00,0x00,0x00,0x00,0x00,0x00,0xff,
    0xff,0xff,0x00,0x00,0x00,0x00,0xff,0xff,
    0xff,0xff,0xff,0x00,0x00,0xff,0xff,0xff,
};
uint8 g_anBullet[] = {
    0xff,0x00,0x00,0x00,0x00,0xff,
    0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,
    0xff,0x00,0x00,0x00,0x00,0xff
};

static os::Bitmap* g_pcBulletBitmap = NULL;

struct State
{
    os::Color32_s cColor;
    os::Point	  cPenPos;
//    os::Font*	  pcFont;
    QFont	  cFont;
    int		  nTransX;
    int		  nTransY;
};

class QPainterPrivate
{
public:
    QPainterPrivate()
    {
	m_nXOffset = 0;
	m_nYOffset = 0;
	m_eRasterOp = Qt::CopyROP;
    }
    QPaintDevice* m_pcDevice;
    os::View*	  m_pcView;
    QFont        m_font;
    QPen	 m_pen;
    QBrush	 m_brush;
    Qt::RasterOp m_eRasterOp;
    int		 m_nXOffset;
    int		 m_nYOffset;

    std::stack<State>	m_cStack;
};

QPainter::QPainter()
{
    d = new QPainterPrivate;
    d->m_pcDevice = NULL;
    d->m_pcView   = NULL;
}

QPainter::QPainter( QWidget* pcWidget )
{
    d = new QPainterPrivate;
    d->m_pcDevice = pcWidget;
    d->m_pcView   = pcWidget;
}

QPainter::QPainter( QPixmap* pcPixmap )
{
    d = new QPainterPrivate;
    d->m_pcDevice = pcPixmap;
    d->m_pcView   = pcPixmap->GetView();
}

QPainter::~QPainter()
{
    if ( d->m_pcView != NULL ) {
//	d->m_pcView->Flush();
    }
    delete d;
}
   
QFontMetrics QPainter::fontMetrics() const
{
//    printf( "Warning:: QPainter::%s() not impelemented\n", __FUNCTION__ );
    return( QFontMetrics( d->m_font.GetFont() ) );
}

void QPainter::setFont( const QFont& cFont )
{
//    if ( d->m_pcView != NULL && cFont.GetFont() != NULL ) {
//	d->m_pcView->SetFont( cFont.GetFont() );
//    }
    d->m_font = cFont;
}

const QFont& QPainter::font() const
{
//    printf( "Warning:: QPainter::%s() not impelemented\n", __FUNCTION__ );
//    if ( d->m_pcView != NULL ) {
//	d->m_font.SetFont( d->m_pcView->GetFont() );
//    }
    return( d->m_font );
}

bool QPainter::hasClipping() const
{
    printf( "Warning:: QPainter::%s() not impelemented\n", __FUNCTION__ );
    return( false );
}

void QPainter::setClipping( bool )
{
    printf( "Warning:: QPainter::%s() not impelemented\n", __FUNCTION__ );
}
    
void QPainter::setPen( const QColor& cColor )
{
//    printf( "Warning:: QPainter::%s:1() not impelemented\n", __FUNCTION__ );
    if ( d->m_pcView != NULL ) {
	d->m_pcView->SetFgColor( cColor.red(), cColor.green(), cColor.blue() );
    }
}

void QPainter::setPen( PenStyle eStyle )
{
    if ( eStyle != 0 ) {
	printf( "Warning:: QPainter::%s:2( %d ) not impelemented\n", __FUNCTION__, eStyle );
    }
}

void QPainter::setPen( const QPen& cPen )
{
//    printf( "Warning:: QPainter::%s:3() not impelemented\n", __FUNCTION__ );
    setBrush( cPen.color() );
}

void QPainter::setBrush( const QColor& cColor )
{
//    printf( "Warning:: QPainter::%s:1() not impelemented\n", __FUNCTION__ );
    if ( d->m_pcView != NULL ) {
	d->m_pcView->SetFgColor( cColor.red(), cColor.green(), cColor.blue() );
    }
    d->m_pen = cColor;
}

void QPainter::setBrush( BrushStyle )
{
}

void QPainter::setBrush( const QBrush& cBrush )
{
    setBrush( cBrush.color() );
//    printf( "Warning:: QPainter::%s:2() not impelemented\n", __FUNCTION__ );
}
    
const QPen& QPainter::pen() const
{
//    printf( "Warning:: QPainter::%s() not impelemented\n", __FUNCTION__ );
    if ( d->m_pcView != NULL ) {
	os::Color32_s sCol = d->m_pcView->GetFgColor();
	d->m_pen = QPen( QColor( sCol.red, sCol.green, sCol.blue ) );
    }
    return( d->m_pen );
}
const QBrush& QPainter::brush() const
{
    if ( d->m_pcView != NULL ) {
	os::Color32_s sCol = d->m_pcView->GetFgColor();
	d->m_brush = QBrush( QColor( sCol.red, sCol.green, sCol.blue ) );
    }
    return( d->m_brush );
}

void QPainter::setRasterOp( RasterOp eOp )
{
    d->m_eRasterOp = eOp;
    if ( d->m_pcView == NULL ) {
	return;
    }
    switch( eOp )
    {
	case Qt::CopyROP:
	    d->m_pcView->SetDrawingMode( os::DM_COPY );
	    break;
	case Qt::XorROP:
	    d->m_pcView->SetDrawingMode( os::DM_INVERT );
	    break;
	default:
	    printf( "Warning: QPainter::setRasterOp() unknown mode %d set\n", eOp );
	    break;
	    
    }
}

Qt::RasterOp QPainter::rasterOp() const
{
    return( d->m_eRasterOp );
}

void QPainter::save()
{
    State sState;
    if ( d->m_pcView != NULL ) {
	sState.cColor	= d->m_pcView->GetFgColor();
	sState.cPenPos  = d->m_pcView->GetPenPosition();
	sState.cFont    = d->m_font;
//	sState.pcFont	= new os::Font( *d->m_pcView->GetFont() );
    } else {
//	sState.pcFont = NULL;
    }
    sState.nTransX  = d->m_nXOffset;
    sState.nTransY  = d->m_nYOffset;
    d->m_cStack.push( sState );
//    printf( "Warning:: QPainter::%s() not impelemented\n", __FUNCTION__ );
}

void QPainter::restore()
{
    State sState = d->m_cStack.top();
    d->m_cStack.pop();
    if ( d->m_pcView != NULL /*&& sState.pcFont != NULL*/ ) {
	d->m_pcView->SetFgColor( sState.cColor );
	d->m_pcView->MovePenTo( sState.cPenPos );
//	d->m_pcView->SetFont( sState.pcFont );
//	sState.pcFont->Release();
    }
    d->m_nXOffset = sState.nTransX;
    d->m_nYOffset = sState.nTransY;
    d->m_font     = sState.cFont;
//    printf( "Warning:: QPainter::%s() not impelemented\n", __FUNCTION__ );
}
    
void QPainter::begin( QPixmap* pcPixmap )
{
    d->m_pcDevice = pcPixmap;
    d->m_pcView   = pcPixmap->GetView();
}

void QPainter::end()
{
    if ( d->m_pcView != NULL ) {
//	d->m_pcView->Flush();
    }
}

QPaintDevice* QPainter::device() const
{
    return( d->m_pcDevice );
}

void QPainter::setClipRect( const QRect & )
{
    printf( "Warning:: QPainter::%s() not impelemented\n", __FUNCTION__ );
}

void QPainter::scale( double sx, double sy )
{
    printf( "Warning:: QPainter::scale(%f, %f) not impelemented\n", sx, sy );
}

void QPainter::translate(int x,int y )
{
    d->m_nXOffset = x;
    d->m_nYOffset = y;
}

void QPainter::drawText( int x, int y, const QString& cString, int len )
{
    if ( d->m_pcView != NULL ) {
	os::drawing_mode eOldMode = d->m_pcView->GetDrawingMode();
	d->m_pcView->SetDrawingMode( os::DM_OVER );
	d->m_pcView->SetFont( d->m_font.GetFont() );

	QCString cUtf8Str = cString.utf8();
	const char* pzStr = cUtf8Str.data();
	int nByteLen;
	if ( len == -1 ) {
	    nByteLen = strlen( pzStr );
	} else {
	    nByteLen = 0;
	    const char* p = pzStr;
	    for ( int i = 0 ; i < len ; ++i ) {
		int nCharLen = os::utf8_char_length( *p );
		p += nCharLen;
		nByteLen += nCharLen;
	    }
	}
	
	d->m_pcView->DrawString( pzStr, nByteLen, os::Point( x + d->m_nXOffset, y + d->m_nYOffset ) );
	d->m_pcView->SetDrawingMode( eOldMode );
    }
}

void QPainter::drawText( int x, int y, int w, int h, int flags,
			 const QString& cString, int len, QRect */*br=0*/,
			 char **/*internal*/ )
{
    if ( d->m_pcView != NULL ) {
//	os::drawing_mode eOldMode = d->m_pcView->GetDrawingMode();
//	d->m_pcView->SetFont( d->m_font.GetFont() );

	os::font_height sHeight;
	d->m_font.GetFont()->GetHeight( &sHeight );
	QRect cStrRect = QFontMetrics( d->m_font ).boundingRect( cString, len );
	if ( flags & Qt::AlignRight ) {
	    x += w - cStrRect.width();
	}
	if ( h == 0 ) {
	    y = y + sHeight.ascender;
	} else {
	    y = y + h / 2 - cStrRect.height() / 2 - sHeight.ascender;
	}
	drawText( x, y, cString, len );
//	d->m_pcView->SetDrawingMode( os::DM_OVER );
//	d->m_pcView->DrawString( cString.ascii(), os::Point( x + d->m_nXOffset, y + d->m_nYOffset ) );
//	d->m_pcView->SetDrawingMode( eOldMode );
    }
}

void QPainter::drawLine( int x1, int y1, int x2, int y2 )
{
    if ( d->m_pcView != NULL ) {
	d->m_pcView->DrawLine( os::Point( x1 + d->m_nXOffset, y1 + d->m_nYOffset ), os::Point( x2 + d->m_nXOffset, y2 + d->m_nYOffset ) );
    }
}

void QPainter::drawRect( int x, int y, int w, int h )
{
    if ( d->m_pcView == NULL ) {
	return;
    }
    x += d->m_nXOffset;
    y += d->m_nYOffset;
    
    d->m_pcView->MovePenTo( x, y );
    d->m_pcView->DrawLine( os::Point( x + w - 1, y ) );
    d->m_pcView->DrawLine( os::Point( x + w - 1, y + h - 1 ) );
    d->m_pcView->DrawLine( os::Point( x, y + h - 1 ) );
    d->m_pcView->DrawLine( os::Point( x, y ) );
}

void QPainter::drawEllipse( int x, int y, int /*w*/, int /*h*/ )
{
//    if ( w != 6 || h != 6 ) {
//	printf( "Warning:: QPainter::drawEllipse( %d,%d ) unsuported size\n", w, h );
//    }
    if ( d->m_pcView == NULL ) {
	return;
    }
    if ( g_pcBulletBitmap == NULL ) {
	g_pcBulletBitmap = new os::Bitmap( BULLET_WIDHT, BULLET_HEIGHT, os::CS_CMAP8 );
	memcpy( g_pcBulletBitmap->LockRaster(), g_anBullet, BULLET_WIDHT * BULLET_HEIGHT );
    }
    os::Rect cRect( 0, 0, BULLET_WIDHT - 1, BULLET_HEIGHT - 1 );
    os::drawing_mode eOldMode = d->m_pcView->GetDrawingMode();
    d->m_pcView->SetDrawingMode( os::DM_OVER );
    d->m_pcView->DrawBitmap( g_pcBulletBitmap, cRect , cRect + os::Point( x + d->m_nXOffset, y + d->m_nYOffset ) );
    d->m_pcView->SetDrawingMode( eOldMode );
//    printf( "Warning:: QPainter::%s() not impelemented\n", __FUNCTION__ );
}

void QPainter::drawArc( int /*x*/, int /*y*/, int /*w*/, int /*h*/, int /*a*/, int /*alen*/ )
{
//    printf( "Warning:: QPainter::%s() not impelemented\n", __FUNCTION__ );
}

void QPainter::drawLineSegments( const QPointArray& cPoints, int index, int nlines )
{
    if ( d->m_pcView == NULL ) {
	return;
    }
    
    if ( nlines == -1 ) {
	nlines = cPoints.count() - index;
    } else {
	nlines *= 2;
    }
    for ( int i = index ; i < nlines + index ; i += 2 ) {
	d->m_pcView->MovePenTo(os::Point( cPoints[i].x() + d->m_nXOffset, cPoints[i].y() + d->m_nYOffset ) );
	d->m_pcView->DrawLine( os::Point( cPoints[i+1].x() + d->m_nXOffset, cPoints[i+1].y() + d->m_nYOffset ) );
    }
    
}

void QPainter::drawPolyline( const QPointArray& cPoints, int index, int npoints )
{
    if ( d->m_pcView == NULL ) {
	return;
    }
    
    if ( npoints == -1 ) {
	npoints = cPoints.count() - index;
    }
    d->m_pcView->MovePenTo( cPoints[index].x() + d->m_nXOffset, cPoints[index].y() + d->m_nYOffset );
    for ( int i = index + 1 ; i < npoints + index ; ++i ) {
	d->m_pcView->DrawLine( os::Point( cPoints[i].x() + d->m_nXOffset, cPoints[i].y() + d->m_nYOffset ) );
    }
}

void QPainter::drawPolygon( const QPointArray& cPoints, bool /*winding*/, int index, int npoints )
{
    if ( d->m_pcView == NULL ) {
	return;
    }
    if ( npoints == -1 ) {
	npoints = cPoints.count() - index;
    }
    d->m_pcView->MovePenTo( cPoints[index].x() + d->m_nXOffset, cPoints[index].y() + d->m_nYOffset );
    for ( int i = index + 1 ; i < npoints + index ; ++i ) {
//	printf( "%d: %d, %d\n", i, cPoints[i].x(), cPoints[i].y() );
	d->m_pcView->DrawLine( os::Point( cPoints[i].x() + d->m_nXOffset, cPoints[i].y() + d->m_nYOffset ) );
    }
}


void QPainter::drawPixmap (int x, int y, const QPixmap& cPixmap, int sx, int sy, int sw, int sh )
{
    if ( d->m_pcView == NULL ) {
	return;
    }
    os::Bitmap* pcBitmap = cPixmap.GetBitmap();
    if ( pcBitmap == NULL ) {
	return;
    }
    cPixmap.GetView()->Sync();
    os::Rect cBounds( 0, 0, sw - 1, sh - 1 );

//    printf( "Render pixmap (%d,%d), (%.2f,%.2f)\n", sw, sh, cBounds.Width(), cBounds.Height() );
//    printf( "Render pixmap %p to %p (%d,%d %d,%d) (%d,%d)\n", cPixmap.GetView(), d->m_pcView, x, y, sw, sh, sx, sy );
    os::drawing_mode eOldMode;

    eOldMode = d->m_pcView->GetDrawingMode();
    if ( cPixmap.IsTransparent() ) {
	d->m_pcView->SetDrawingMode( os::DM_BLEND );
    } else {
	d->m_pcView->SetDrawingMode( os::DM_COPY );
    }
    
    d->m_pcView->DrawBitmap( pcBitmap, (cBounds + os::Point( sx, sy )) & pcBitmap->GetBounds(), cBounds + os::Point( x + d->m_nXOffset, y + d->m_nYOffset ) );
    d->m_pcView->SetDrawingMode( eOldMode );
    d->m_pcView->Sync();
}

void QPainter::drawPixmap( const QPoint& cPos, const QPixmap& cPixmap )
{
    drawPixmap( cPos.x(), cPos.y(), cPixmap, 0, 0, cPixmap.width(), cPixmap.height() );
}

void QPainter::drawPixmap( const QPoint& cPos, const QPixmap& cPixmap, const QRect &sr )
{
    drawPixmap( cPos.x(), cPos.y(), cPixmap, sr.x(), sr.y(), sr.width(), sr.height() );
}

void QPainter::fillRect( int x, int y, int w, int h, const QColor& cColor )
{
    x += d->m_nXOffset;
    y += d->m_nYOffset;

    
    if ( d->m_pcView != NULL ) {
	d->m_pcView->FillRect( os::Rect( x, y, x + w - 1, y + h - 1 ), os::Color32_s( cColor.red(), cColor.green(), cColor.blue() ) );
    }
}

void QPainter::drawTiledPixmap( int x, int y, int w, int h, const QPixmap& cPixmap, int sx, int sy )
{
    if ( d->m_pcView == NULL ) {
	return;
    }
    os::Bitmap* pcBitmap = cPixmap.GetBitmap();
    if ( pcBitmap == NULL ) {
	return;
    }
    
    pcBitmap->Sync();

    x += d->m_nXOffset;
    y += d->m_nYOffset;
    
    QSize cSize = cPixmap.size();
//    printf( "Draw %d, %d  (%d, %d)\n", w, h, cSize.width(), cSize.height() );

    os::drawing_mode eOldMode = d->m_pcView->GetDrawingMode();

    if ( cPixmap.IsTransparent() ) {
	d->m_pcView->SetDrawingMode( os::DM_BLEND );
    } else {
	d->m_pcView->SetDrawingMode( os::DM_COPY );
    }
    sx = sx % cSize.width();
    sy = sy % cSize.height();
    
    for ( int yi = 0 ; ; ) {
	int _sx = sx;
	int nDeltaY = cSize.height() - sy;
	if ( yi + nDeltaY > h ) {
	    nDeltaY = h - yi;
	}
	if ( nDeltaY <= 0 ) {
	    break;
	}
	for ( int xi = 0 ; ; ) {
	    int nDeltaX = cSize.width() - _sx;
	    if ( xi + nDeltaX > w ) {
		nDeltaX = w - xi;
	    }
	    if ( nDeltaX <= 0 ) {
		break;
	    }
	    os::Rect cBounds( 0, 0, nDeltaX - 1, nDeltaY - 1 );
    
	    d->m_pcView->DrawBitmap( pcBitmap, (cBounds + os::Point( _sx, sy )) & pcBitmap->GetBounds(), cBounds + os::Point( x + xi, y + yi ) );

	    _sx = 0;
	    xi += nDeltaX;
	}
	yi += nDeltaY;
	sy = 0;
    }
    d->m_pcView->SetDrawingMode( eOldMode );
    d->m_pcView->Flush();
//    d->m_pcView->Sync();
    
}





inline double qcos( double a )
{
    double r;
    __asm__ (
	"fcos"
	: "=t" (r) : "0" (a) );
    return(r);
}

inline double qsin( double a )
{
    double r;
    __asm__ (
	"fsin"
	: "=t" (r) : "0" (a) );
    return(r);
}

double qsincos( double a, bool calcCos=FALSE )
{
    return calcCos ? qcos(a) : qsin(a);
}




void qDrawShadePanel( QPainter *p, int x, int y, int w, int h,
		      const QColorGroup &g, bool sunken,
		      int lineWidth, const QBrush *fill )
{
    if ( w == 0 || h == 0 )
	return;
    if ( !( w > 0 && h > 0 && lineWidth >= 0 ) ) {
#if defined(CHECK_RANGE)
    	qWarning( "qDrawShadePanel() Invalid parameters." );
#endif
    }
//    printf( "draw panel (%d,%d - %d,%d)\n", x, y, w, h );
    QPen oldPen = p->pen();			// save pen
    QPointArray a( 4*lineWidth );
    if ( sunken )
	p->setPen( g.dark() );
    else
	p->setPen( g.light() );
    int x1, y1, x2, y2;
    int i;
    int n = 0;
    x1 = x;
    y1 = y2 = y;
    x2 = x+w-2;
    for ( i=0; i<lineWidth; i++ ) {		// top shadow
	a.setPoint( n++, x1, y1++ );
	a.setPoint( n++, x2--, y2++ );
    }
    x2 = x1;
    y1 = y+h-2;
    for ( i=0; i<lineWidth; i++ ) {		// left shadow
	a.setPoint( n++, x1++, y1 );
	a.setPoint( n++, x2++, y2-- );
    }
    p->drawLineSegments( a );
    n = 0;
    if ( sunken )
	p->setPen( g.light() );
    else
	p->setPen( g.dark() );
    x1 = x;
    y1 = y2 = y+h-1;
    x2 = x+w-1;
    for ( i=0; i<lineWidth; i++ ) {		// bottom shadow
	a.setPoint( n++, x1++, y1-- );
	a.setPoint( n++, x2, y2-- );
    }
    x1 = x2;
    y1 = y;
    y2 = y+h-lineWidth-1;
    for ( i=0; i<lineWidth; i++ ) {		// right shadow
	a.setPoint( n++, x1--, y1++ );
	a.setPoint( n++, x2--, y2 );
    }
    p->drawLineSegments( a );
    if ( fill ) {				// fill with fill color
	QBrush oldBrush = p->brush();
	p->setPen( Qt::NoPen );
	p->setBrush( *fill );
	p->drawRect( x+lineWidth, y+lineWidth, w-lineWidth*2, h-lineWidth*2 );
	p->setBrush( oldBrush );
    }
    p->setPen( oldPen );			// restore pen
}
