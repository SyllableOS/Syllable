#include <stdio.h>

#include <qpaintdevicemetrics.h>
#include <qscrollview.h>

class QWidgetPrivate
{
public:
    int m_nHideCount;
};

QWidget::QWidget( QWidget* pcParent, const char* pzName ) : View( os::Rect( 0, 0, 200, 200 ),
								  pzName, os::CF_FOLLOW_LEFT | os::CF_FOLLOW_TOP,
								  os::WID_WILL_DRAW | os::WID_FULL_UPDATE_ON_RESIZE )
{
    d = new QWidgetPrivate;

    d->m_nHideCount = 1;
    Show( false );
    
    if ( pcParent != NULL ) {
	QScrollView* pcSView = dynamic_cast<QScrollView*>(pcParent);
	if ( pcSView != NULL ) {
	    pcParent = pcSView->viewport();
	    if ( pcParent == NULL ) {
		pcParent = pcSView;
	    }
	}
	pcParent->AddChild( this );
    }
}

QWidget::~QWidget()
{
    delete d;
}

void QWidget::clearFocus()
{
    MakeFocus( false );
}

void QWidget::show()
{
    if ( --d->m_nHideCount == 0 ) {
	Show( true );
    }
}

void QWidget::hide()
{
    if ( d->m_nHideCount == 0 ) {
	Show( false );
    }
}

void QWidget::setFocusProxy( QWidget * )
{
//    printf( "Warning: QWidget::%s() not implemented\n", __FUNCTION__ );
}

bool QWidget::focusNextPrevChild( bool next )
{
    printf( "Warning: QWidget::%s() not implemented\n", __FUNCTION__ );
    return( false );
}

QWidget* QWidget::focusWidget() const
{
    printf( "Warning: QWidget::%s() not implemented\n", __FUNCTION__ );
    return( NULL );
}

QPoint QWidget::mapToGlobal( const QPoint& cPos ) const
{
    os::Point cScrPos = ConvertToScreen( os::Point( cPos.x(), cPos.y() ) );
    return( QPoint( cScrPos.x, cScrPos.y ) );
}

QPoint QWidget::mapFromGlobal( const QPoint& cScrPos )
{
    os::Point cPos = ConvertFromScreen( os::Point( cScrPos.x(), cScrPos.y() ) );
    return( QPoint( cPos.x, cPos.y ) );
}

QSize QWidget::sizeHint() const
{
    os::Point cSize = GetPreferredSize( false );

    return( QSize( cSize.x, cSize.y ) );
}

void QWidget::setFont( const QFont & )
{
//    printf( "Warning: QWidget::%s() not implemented\n", __FUNCTION__ );
}
    
void QWidget::resize( int w, int h )
{
    ResizeTo( w - 1, h - 1 );
//    printf( "Warning: QWidget::%s() not implemented\n", __FUNCTION__ );
}
    
int QWidget::x()
{
    return( int(GetFrame().left) );
}

int QWidget::y()
{
    return( int(GetFrame().top) );
}
    
int QWidget::width()
{
    return( int(GetFrame().Width()) + 1 );
}

int QWidget::height()
{
    return( int(GetFrame().Height()) + 1 );
}

QPoint QWidget::pos() const
{
    os::Rect cFrame = GetFrame();
    return( QPoint( cFrame.left, cFrame.top ) );
}

void QWidget::move( const QPoint& cPos )
{
    MoveTo( cPos.x(), cPos.y() );
}

QSize QWidget::size() const
{
    os::Rect cFrame = GetFrame();
    return( QSize( cFrame.Width() + 1, cFrame.Height() + 1 ) );
}

void QWidget::resize( const QSize& cSize )
{
    ResizeTo( cSize.width() - 1, cSize.height() - 1 );
}

void QWidget::erase()
{
    FillRect( GetBounds(), os::Color32_s( 255, 255, 255 ) );
    Flush();
//    printf( "Warning: QWidget::%s() not implemented\n", __FUNCTION__ );
}
/*
const QPalette& QWidget::palette() const
{
    printf( "Warning: QWidget::%s() not implemented\n", __FUNCTION__ );
    return( m_cPalette );
}
*/  
    
QWidget* QWidget::topLevelWidget()
{
//    printf( "Warning: QWidget::%s() not implemented\n", __FUNCTION__ );
    QWidget* pcParent = this;
    for (;;) {
	QWidget* pcTmp = dynamic_cast<QWidget*>(pcParent->GetParent());
	if ( pcTmp == NULL ) {
	    break;
	}
	pcParent = pcTmp;
    }
    return( pcParent );
}

void QWidget::setFocus()
{
    MakeFocus( true );
}

bool QWidget::isTopLevel() const
{
    printf( "Warning: QWidget::%s() not implemented\n", __FUNCTION__ );
    return 0;
}

bool QWidget::isEnabled() const
{
    printf( "Warning: QWidget::%s() not implemented\n", __FUNCTION__ );
    return 1 ;
}

void QWidget::setMouseTracking( bool )
{
//    printf( "Warning: QWidget::%s() not implemented\n", __FUNCTION__ );
}

void QWidget::setBackgroundMode( BackgroundMode )
{
    printf( "Warning: QWidget::%s() not implemented\n", __FUNCTION__ );
}

void QWidget::setCursor( const QCursor& )
{
//    printf( "Warning: QWidget::%s() not implemented\n", __FUNCTION__ );
}
    
void QWidget::setFocusPolicy( FocusPolicy )
{
//    printf( "Warning: QWidget::%s() not implemented\n", __FUNCTION__ );
}

void QWidget::setAcceptDrops( bool /*on*/ )
{
//    printf( "Warning: QWidget::%s() not implemented\n", __FUNCTION__ );
}

void QWidget::focusInEvent( QFocusEvent * )
{
    printf( "Warning: QWidget::%s() not implemented\n", __FUNCTION__ );
}

void QWidget::focusOutEvent( QFocusEvent * )
{
    printf( "Warning: QWidget::%s() not implemented\n", __FUNCTION__ );
}

void QWidget::setEnabled( bool /*bEnabled*/ )
{
//    printf( "Warning: QWidget::%s() not implemented\n", __FUNCTION__ );
}

void QWidget::setAutoMask(bool)
{
//    printf( "Warning: QWidget::%s() not implemented\n", __FUNCTION__ );
}

void QWidget::repaint()
{
    Invalidate();
    Flush();
}


int QWidget::metric( int id ) const
{
    switch( id )
    {
	case QPaintDeviceMetrics::PdmWidth:
	    return( int(GetFrame().Width()) );
	case QPaintDeviceMetrics::PdmHeight:
	    return( int(GetFrame().Height()) );
	case QPaintDeviceMetrics::PdmWidthMM:
	    printf( "Warning: QWidget::metric( PdmWidthMM ) not implemented\n" );
	    return( 200 );
	case QPaintDeviceMetrics::PdmHeightMM:
	    printf( "Warning: QWidget::metric( PdmHeightMM ) not implemented\n" );
	    return( 200 );
	case QPaintDeviceMetrics::PdmNumColors:
	    printf( "Warning: QWidget::metric( PdmNumColors ) not implemented\n" );
	    return( 65536 );
	case QPaintDeviceMetrics::PdmDepth:
	    printf( "Warning: QWidget::metric( PdmDepth ) not implemented\n" );
	    return( 16 );
	case QPaintDeviceMetrics::PdmDpiX:
	    printf( "Warning: QWidget::metric( PdmDpiX ) not implemented\n" );
	    return( 72 );
	case QPaintDeviceMetrics::PdmDpiY:
//	    printf( "Warning: QWidget::metric( PdmDpiY ) not implemented\n" );
	    return( 72 );
	default:
	    printf( "Error: QWidget::metric( %d ) unknown ID\n", id );
	    return( -1 );
    }
}


os::Point QWidget::GetPreferredSize( bool bLargest ) const
{
    return( (bLargest) ? os::Point( 1000000.0f, 1000000.0f ) : os::Point( 0.0f, 0.0f ) );
}

#include "awidget.moc"
