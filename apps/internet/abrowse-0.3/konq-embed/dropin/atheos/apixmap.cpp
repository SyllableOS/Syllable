#include <stdio.h>

#include "apixmap.h"
#include <qpaintdevicemetrics.h>
#include <gui/bitmap.h>
#include <gui/view.h>

class QPixmapPrivate
{
public:
    os::Bitmap* m_pcBitmap;
    os::View*	m_pcView;
    os::Rect	m_cValidRect;
    bool	m_bIsTranstarent;
};

QPixmap::QPixmap()
{
    d = new QPixmapPrivate;
    d->m_pcBitmap = NULL;
    d->m_pcView   = new os::View( os::Rect( 0, 0, -1, -1 ), "bitmap_view" );
    d->m_cValidRect = os::Rect( 100000.0f, 100000.0f, 0.0f, 0.0f );
    d->m_bIsTranstarent = false;
}

QPixmap::QPixmap( const QPixmap& cOther )
{
    d = new QPixmapPrivate;
    d->m_pcBitmap = NULL;
    d->m_pcView   = new os::View( os::Rect( 0, 0, -1, -1 ), "bitmap_view" );
    d->m_cValidRect = os::Rect( 100000.0f, 100000.0f, 0.0f, 0.0f );
    d->m_bIsTranstarent = false;
    *this = cOther;
}

QPixmap::QPixmap( int w, int h )
{
    d = new QPixmapPrivate;
    if ( w > 0 && h > 0 ) {
	d->m_pcBitmap = new os::Bitmap( w, h, os::CS_RGB16, os::Bitmap::ACCEPT_VIEWS );
    } else {
	d->m_pcBitmap = NULL;
    }
    d->m_pcView   = new os::View( os::Rect( 0, 0, w - 1, h - 1 ), "bitmap_view" );
    if ( d->m_pcBitmap != NULL ) {
	d->m_pcBitmap->AddChild( d->m_pcView );
    }
    d->m_cValidRect = os::Rect( 100000.0f, 100000.0f, 0.0f, 0.0f );
    d->m_bIsTranstarent = false;
//    d->m_pcView->FillRect( d->m_pcView->GetBounds(), os::Color32_s( 255, 255, 255 ) );
}

QPixmap::QPixmap( os::Bitmap* pcBitmap )
{
    d = new QPixmapPrivate;
    d->m_pcBitmap = pcBitmap;
    d->m_pcView   = new os::View( pcBitmap->GetBounds(), "bitmap_view" );
    d->m_pcBitmap->AddChild( d->m_pcView );
    d->m_cValidRect = os::Rect( 100000.0f, 100000.0f, 0.0f, 0.0f );
    d->m_bIsTranstarent = false;
}

QPixmap::~QPixmap()
{
    delete d->m_pcBitmap;
    d->m_pcBitmap = (os::Bitmap*) 0x12345678;
    delete d;
}

QPixmap& QPixmap::operator=( const QPixmap& cOther )
{
    d->m_bIsTranstarent = cOther.d->m_bIsTranstarent;
    d->m_cValidRect = cOther.d->m_cValidRect;
    if ( cOther.d->m_pcBitmap != NULL ) {
	resize( cOther.width(), cOther.height() );

	if ( d->m_pcBitmap != NULL ) {
	    d->m_pcBitmap->RemoveChild( d->m_pcView );
	    delete d->m_pcBitmap;
	}
	d->m_pcBitmap = new os::Bitmap( cOther.width(), cOther.height(), cOther.d->m_pcBitmap->GetColorSpace(), os::Bitmap::ACCEPT_VIEWS );
	d->m_pcView->ResizeTo( cOther.width() - 1, cOther.height() - 1 );
	d->m_pcBitmap->AddChild( d->m_pcView );
	
	os::Rect cBounds = cOther.d->m_pcBitmap->GetBounds();
	d->m_pcView->SetDrawingMode( os::DM_COPY );
	d->m_pcView->DrawBitmap( cOther.d->m_pcBitmap, cBounds, cBounds );
	d->m_pcView->Sync();
    } else {
	if ( d->m_pcBitmap != NULL ) {
	    d->m_pcBitmap->RemoveChild( d->m_pcView );
	    delete d->m_pcBitmap;
	    d->m_pcBitmap = NULL;
	}
    }
    return( *this );
}

QSize QPixmap::size() const
{
    if ( d->m_pcBitmap != NULL ) {
	os::Rect cBounds = d->m_pcBitmap->GetBounds();
	QSize cSize( cBounds.Width() + 1, cBounds.Height() + 1 );
	return( cSize );
    } else {
	return( QSize( 0, 0 ) );
    }
}

QRect QPixmap::rect() const
{
    if ( d->m_pcBitmap != NULL ) {
	os::Rect cBounds = d->m_pcBitmap->GetBounds();
	QRect cRect( QPoint( cBounds.left, cBounds.top ), QPoint( cBounds.right, cBounds.bottom ) );
	return( cRect );
    } else {
	return( QRect( 0, 0, 0, 0 ) );
    }
}

int QPixmap::width() const
{
    return( size().width() );
}

int QPixmap::height() const
{
    return( size().height() );
}

void QPixmap::resize( int w, int h )
{
    QSize cOldSize = size();
    if ( cOldSize.width() == w && cOldSize.height() == h ) {
	return;
    }
    if ( d->m_pcBitmap != NULL ) {
	d->m_pcBitmap->RemoveChild( d->m_pcView );
	delete d->m_pcBitmap;
    }
    if ( w > 0 && h > 0 ) {
	d->m_pcBitmap = new os::Bitmap( w, h, os::CS_RGB16, os::Bitmap::ACCEPT_VIEWS );
	d->m_pcView->ResizeTo( w - 1, h - 1 );
	d->m_pcBitmap->AddChild( d->m_pcView );
//	d->m_pcView->FillRect( d->m_pcView->GetBounds(), os::Color32_s( 255, 255, 255 ) );
	
    } else {
	d->m_pcBitmap = NULL;
    }
}

bool QPixmap::isNull() const
{
    return( d->m_pcBitmap == NULL );
}
bool QPixmap::IsTransparent() const
{
    return( d->m_bIsTranstarent );
}

void QPixmap::SetIsTransparent( bool bTrans )
{
    d->m_bIsTranstarent = bTrans;
}

QBitmap* QPixmap::mask() const
{
//    printf( "Warning: QPixmap::%s() not implemented\n", __FUNCTION__ );
    return( 0 );
}

int QPixmap::metric( int id ) const
{
    switch( id )
    {
	case QPaintDeviceMetrics::PdmWidth:
	    printf( "Warning: QPixmap::metric( PdmWidth ) not implemented\n" );
	    return( 200 );
	case QPaintDeviceMetrics::PdmHeight:
	    printf( "Warning: QPixmap::metric( PdmHeight ) not implemented\n" );
	    return( 200 );
	case QPaintDeviceMetrics::PdmWidthMM:
	    printf( "Warning: QPixmap::metric( PdmWidthMM ) not implemented\n" );
	    return( 200 );
	case QPaintDeviceMetrics::PdmHeightMM:
	    printf( "Warning: QPixmap::metric( PdmHeightMM ) not implemented\n" );
	    return( 200 );
	case QPaintDeviceMetrics::PdmNumColors:
	    printf( "Warning: QPixmap::metric( PdmNumColors ) not implemented\n" );
	    return( 65536 );
	case QPaintDeviceMetrics::PdmDepth:
	    printf( "Warning: QPixmap::metric( PdmDepth ) not implemented\n" );
	    return( 16 );
	case QPaintDeviceMetrics::PdmDpiX:
	    printf( "Warning: QPixmap::metric( PdmDpiX ) not implemented\n" );
	    return( 72 );
	case QPaintDeviceMetrics::PdmDpiY:
	    printf( "Warning: QPixmap::metric( PdmDpiY ) not implemented\n" );
	    return( 72 );
	default:
	    printf( "Error: QPixmap::metric( %d ) unknown ID\n", id );
	    return( -1 );
    }
}

void QPixmap::ExpandValidRect( const QRect& cRect ) const
{
    d->m_cValidRect |= os::Rect( cRect.left(), cRect.top(), cRect.right(), cRect.bottom() );
}

QRect QPixmap::GetValidRect() const
{
    return( QRect( QPoint( d->m_cValidRect.left, d->m_cValidRect.top ), QPoint( d->m_cValidRect.right, d->m_cValidRect.bottom ) ) );
}

os::View* QPixmap::GetView() const
{
    return( d->m_pcView );
}

os::Bitmap* QPixmap::GetBitmap() const
{
    return( d->m_pcBitmap );
}
