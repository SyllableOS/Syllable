#include <stdio.h>

#include <qpoint.h>
#include "ascrollview.h"
#include "apainter.h"

#include <gui/scrollbar.h>
#include <util/message.h>

enum { ID_VSCROLL, ID_HSCROLL };

class ViewPort : public QWidget
{
public:
    ViewPort( QScrollView* pcParent ) : QWidget( pcParent, "sv_viewport" ) { m_pcParent = pcParent; }

    virtual void Paint( const os::Rect& cUpdateRect )
    {
	QPainter cPainter( this );
	m_pcParent->drawContents( &cPainter, cUpdateRect.left, cUpdateRect.top, cUpdateRect.Width() + 1, cUpdateRect.Height() + 1 );
    }
private:
    QScrollView* m_pcParent;
    
};

class QScrollViewPrivate
{
public:
    ViewPort*		       m_pcViewPort;
    os::ScrollBar*	       m_pcHScrollBar;
    os::ScrollBar*	       m_pcVScrollBar;
    QScrollView::ScrollBarMode m_eVScrollBarMode;
    QScrollView::ScrollBarMode m_eHScrollBarMode;
    int			       m_nContentWidth;
    int			       m_nContentHeight;
};



QScrollView::QScrollView( QWidget* pcParent, const char* pzName, int /*flags*/ ) : QFrame( pcParent, pzName )
{
    d = new QScrollViewPrivate;
    d->m_pcHScrollBar = NULL;
    d->m_pcVScrollBar = NULL;
    d->m_pcViewPort = NULL;
    d->m_pcViewPort = new ViewPort( this );
    d->m_pcViewPort->SetResizeMask( os::CF_FOLLOW_ALL );
    d->m_pcViewPort->SetFrame( GetBounds() /*os::Rect( 0.0f, 0.0f, 100000.0f, 100000.0f )*/ );
    d->m_eHScrollBarMode = Auto;
    d->m_eVScrollBarMode = Auto;
    d->m_nContentWidth  = 0;
    d->m_nContentHeight = 0;
}

QScrollView::~QScrollView()
{
    delete d;
}

QWidget* QScrollView::viewport()
{
    return( d->m_pcViewPort );
}


void QScrollView::setContentsPos( int x, int y )
{
    if ( x < 0 ) {
	x = 0;
    } else if ( x > contentsWidth() - visibleWidth() ) {
	x = contentsWidth() - visibleWidth();
    }
    if ( y < 0 ) {
	y = 0;
    } else if ( y > contentsHeight() - visibleHeight() ) {
	y = contentsHeight() - visibleHeight();
    }
    d->m_pcViewPort->ScrollTo( -x, -y );
    Flush();
    UpdateScrollBars();
}

void QScrollView::resizeContents( int w, int h )
{
//    d->m_pcViewPort->ResizeTo( w - 1, h - 1 );
    d->m_nContentWidth  = w;
    d->m_nContentHeight = h;
    
    os::Point cOffset = -d->m_pcViewPort->GetScrollOffset();

    if ( w - visibleWidth() ) {
	cOffset.x = 0.0f;
    } else if ( cOffset.x > w - visibleWidth() ) {
	cOffset.x = w - visibleWidth();
    }
    
    if ( h < visibleHeight() ) {
	cOffset.y = 0.0f;
    } else if ( cOffset.y > h - visibleHeight() ) {
	cOffset.y = h - visibleHeight();
	
    }
    d->m_pcViewPort->ScrollTo( -cOffset );
    Flush();
    UpdateScrollBars();
}

void QScrollView::scrollBy( int x, int y )
{
    setContentsPos( contentsX() + x, contentsY() + y );
}

void QScrollView::contentsToViewport(int x, int y, int& vx, int& vy)
{
    vx = x; // + contentsX();
    vy = y; // + contentsY();
}

void QScrollView::viewportToContents(int vx, int vy, int& x, int& y)
{
    x = vx + contentsX();
    y = vy + contentsY();
}

int QScrollView::contentsX()
{
    return( -int(d->m_pcViewPort->GetScrollOffset().x) );
}

int QScrollView::contentsY()
{
    return( -int(d->m_pcViewPort->GetScrollOffset().y) );
}

int QScrollView::contentsWidth() const
{
    return( d->m_nContentWidth );
//    return( int(d->m_pcViewPort->GetFrame().Width()) + 1 );
}

int QScrollView::contentsHeight() const
{
    return( d->m_nContentHeight );
//    return( int(d->m_pcViewPort->GetFrame().Height()) + 1 );
}

void QScrollView::ensureVisible( int x, int y )
{
    ensureVisible( x, y, 0, 0 );
}

void QScrollView::ensureVisible(int x, int y, int xmargin, int ymargin)
{
    int nOldX = contentsX();
    int nOldY = contentsY();
    
    if ( x < nOldX + xmargin ) {
	nOldX = x - xmargin;
    }
    if ( x > nOldX + visibleWidth() - xmargin ) {
	nOldX = x - visibleWidth() + xmargin;
    }
    if ( y < nOldY + ymargin ) {
	nOldY = y - ymargin;
    }
    if ( y > nOldY + visibleHeight() - ymargin ) {
	nOldY = y - visibleHeight() + ymargin;
    }
    setContentsPos( nOldX, nOldY );
}

    
int QScrollView::visibleWidth()
{
    return( int(d->m_pcViewPort->GetFrame().Width()) + 1 );
//    return( int(GetFrame().Width()) + 1 );
}

int QScrollView::visibleHeight()
{
    return( int(d->m_pcViewPort->GetFrame().Height()) + 1 );
//    return( int(GetFrame().Height()) + 1 );
}

void QScrollView::setVScrollBarMode( ScrollBarMode eMode )
{
    d->m_eVScrollBarMode = eMode;
    UpdateScrollBars();
}

void QScrollView::setHScrollBarMode( ScrollBarMode eMode )
{
    d->m_eHScrollBarMode = eMode;
    UpdateScrollBars();
}

void QScrollView::setStaticBackground( bool )
{
    printf( "Warning: QScrollView::%s() not implemented\n", __FUNCTION__ );
}

bool QScrollView::focusNextPrevChild( bool /*next*/ )
{
    printf( "Warning: QScrollView::%s() not implemented\n", __FUNCTION__ );
    return( false );
}

void QScrollView::addChild( QWidget* child, int x, int y )
{
    if ( child->GetWindow() != GetWindow() ) {
	d->m_pcViewPort->AddChild( child );
    }
    child->MoveTo( x, y );
}

void QScrollView::enableClipper( bool )
{
//    printf( "Warning: QScrollView::%s() not implemented\n", __FUNCTION__ );
}

void QScrollView::updateContents( int x, int y, int w, int h )
{
    d->m_pcViewPort->Invalidate( os::Rect( x, y, x+w-1, y+h-1 ) );
    d->m_pcViewPort->Flush();
}

void QScrollView::repaintContents( int x, int y, int w, int h, bool /*erase*/ )
{
//    printf( "Warning: QScrollView::%s() not implemented\n", __FUNCTION__ );
    QPainter cPainter( d->m_pcViewPort );

    drawContents( &cPainter, x, y, w, h );
}

void QScrollView::Paint( const os::Rect& cUpdateRect )
{
    FillRect( cUpdateRect, os::Color32_s( 255, 255, 255 ) );
    QPainter cPainter( d->m_pcViewPort );
    drawContents( &cPainter, cUpdateRect.left, cUpdateRect.top, cUpdateRect.Width() + 1, cUpdateRect.Height() + 1 );
}

void QScrollView::UpdateScrollBars()
{
    if ( d->m_pcVScrollBar == NULL ) {
	if ( (d->m_eVScrollBarMode == Auto && visibleHeight() < contentsHeight()) || d->m_eVScrollBarMode == AlwaysOn ) {
	    d->m_pcVScrollBar = new os::ScrollBar( os::Rect(0,0,0,0), "vscroll", new os::Message( ID_VSCROLL ), 0, 0, os::VERTICAL,
						   os::CF_FOLLOW_RIGHT | os::CF_FOLLOW_TOP | os::CF_FOLLOW_BOTTOM );
	    os::Rect cFrame = GetBounds();
	    cFrame.left = cFrame.right - d->m_pcVScrollBar->GetPreferredSize(false).x;
	    if ( d->m_pcHScrollBar != NULL ) {
		cFrame.bottom -= d->m_pcHScrollBar->GetBounds().Height() + 1;
	    }
	    d->m_pcVScrollBar->SetFrame( cFrame );
	    AddChild( d->m_pcVScrollBar );
	    d->m_pcVScrollBar->SetTarget( this );
	    if ( d->m_pcHScrollBar != NULL ) {
		d->m_pcHScrollBar->ResizeBy( -(cFrame.Width() + 1), 0 );
	    }
	    d->m_pcViewPort->ResizeBy( -(cFrame.Width() + 1), 0 );
	}
    } else {
	if ( (d->m_eVScrollBarMode == Auto && visibleHeight() >= contentsHeight()) || d->m_eVScrollBarMode == AlwaysOff ) {
	    if ( d->m_pcHScrollBar != NULL ) {
		d->m_pcHScrollBar->ResizeBy( d->m_pcVScrollBar->GetBounds().Width() + 1, 0 );
	    }
	    d->m_pcViewPort->ResizeBy( d->m_pcVScrollBar->GetBounds().Width() + 1, 0 );
	    RemoveChild( d->m_pcVScrollBar );
	    delete d->m_pcVScrollBar;
	    d->m_pcVScrollBar = NULL;
	}
    }
    if ( d->m_pcHScrollBar == NULL ) {
	if ( (d->m_eHScrollBarMode == Auto && visibleWidth() < contentsWidth()) || d->m_eHScrollBarMode == AlwaysOn ) {
	    d->m_pcHScrollBar = new os::ScrollBar( os::Rect(0,0,0,0), "hscroll", new os::Message( ID_HSCROLL ), 0, 0, os::HORIZONTAL,
						   os::CF_FOLLOW_BOTTOM | os::CF_FOLLOW_LEFT | os::CF_FOLLOW_RIGHT );
	    os::Rect cFrame = GetBounds();
	    cFrame.top = cFrame.bottom - d->m_pcHScrollBar->GetPreferredSize(false).y;
	    if ( d->m_pcVScrollBar != NULL ) {
		cFrame.right -= d->m_pcVScrollBar->GetBounds().Width() + 1;
	    }
	    d->m_pcHScrollBar->SetFrame( cFrame );
	    AddChild( d->m_pcHScrollBar );
	    d->m_pcHScrollBar->SetTarget( this );

	    if ( d->m_pcVScrollBar != NULL ) {
		d->m_pcVScrollBar->ResizeBy( 0, -(cFrame.Height() + 1) );
	    }
	    d->m_pcViewPort->ResizeBy( 0, -(cFrame.Height() + 1) );
	}
    } else {
	if ( (d->m_eHScrollBarMode == Auto && visibleWidth() >= contentsWidth()) || d->m_eHScrollBarMode == AlwaysOff ) {
	    if ( d->m_pcVScrollBar != NULL ) {
		d->m_pcVScrollBar->ResizeBy( 0, d->m_pcHScrollBar->GetBounds().Height() + 1 );
	    }
	    d->m_pcViewPort->ResizeBy( 0, d->m_pcHScrollBar->GetBounds().Height() + 1 );
	    RemoveChild( d->m_pcHScrollBar );
	    delete d->m_pcHScrollBar;
	    d->m_pcHScrollBar = NULL;
	}
    }

    if ( d->m_pcHScrollBar != NULL ) {
	d->m_pcHScrollBar->SetMinMax( 0, contentsWidth() - visibleWidth() );
	d->m_pcHScrollBar->SetValue( contentsX(), false );
	d->m_pcHScrollBar->SetProportion( float(visibleWidth()) / float(contentsWidth()) );
	d->m_pcHScrollBar->SetSteps( 10.0f, float(visibleWidth()) * 0.8f );
    }
    if ( d->m_pcVScrollBar != NULL ) {
	d->m_pcVScrollBar->SetMinMax( 0, contentsHeight() - visibleHeight() );
	d->m_pcVScrollBar->SetValue( contentsY(), false );
	d->m_pcVScrollBar->SetProportion( float(visibleHeight()) / float(contentsHeight()) );
	d->m_pcVScrollBar->SetSteps( 10.0f, float(visibleHeight()) * 0.8f );
    }
}


void QScrollView::HandleMessage( os::Message* pcMessage )
{
    switch( pcMessage->GetCode() )
    {
	case ID_HSCROLL:
	    if ( d->m_pcHScrollBar != NULL ) {
		setContentsPos( d->m_pcHScrollBar->GetValue(), contentsY() );
	    }
	    break;
	case ID_VSCROLL:
	    if ( d->m_pcVScrollBar != NULL ) {
		setContentsPos( contentsX(), d->m_pcVScrollBar->GetValue() );
	    }
	    break;
    }
}

void QScrollView::WheelMoved( const os::Point& cDelta )
{
    scrollBy( 0, cDelta.y * 30.0f );
}

os::Point QScrollView::GetPreferredSize( bool bLargest ) const
{
    return( (bLargest) ? os::Point( 1000000.0f, 1000000.0f ) : os::Point( 0.0f, 0.0f ) );
}

#include "ascrollview.moc"
