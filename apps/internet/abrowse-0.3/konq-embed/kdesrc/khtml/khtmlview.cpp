/* This file is part of the KDE project
 *
 * Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>
 *                     1999 Lars Knoll <knoll@kde.org>
 *                     1999 Antti Koivisto <koivisto@kde.org>
 *                     2000 Dirk Mueller <mueller@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "khtmlview.moc"
#ifdef __ATHEOS__
#include <qpoint.h>
#include <qrect.h>
#include <qsize.h>
#include <qregion.h>
#endif

#include "khtml_part.h"
#include "khtml_events.h"

#include "html/html_documentimpl.h"
#include "html/html_inlineimpl.h"
#include "rendering/render_object.h"
#include "rendering/render_root.h"
#include "rendering/render_style.h"
#include "misc/htmlhashes.h"
#include "misc/helper.h"
#include "khtml_settings.h"
#include "khtmlview.h"

#include <kcursor.h>

#include <qpixmap.h>
#include <qstring.h>
#include <qpainter.h>
#include <qpalette.h>
#include <qevent.h>
#include <qdatetime.h>
#ifndef __ATHEOS__
#include <qprinter.h>
#endif
#include <qpaintdevicemetrics.h>
#include <qtimer.h>
#include <kapp.h>
#ifndef __ATHEOS__
#include <kimageio.h>
#endif
#include <kdebug.h>

#ifdef __ATHEOS__
#include <atheos/time.h>
#include <util/messenger.h>
#endif

#define PAINT_BUFFER_HEIGHT 128

template class QList<KHTMLView>;

QList<KHTMLView> *KHTMLView::lstViews = 0L;

using namespace DOM;
using namespace khtml;

class KHTMLViewPrivate {
public:
    KHTMLViewPrivate()
    {
        reset();
        tp=0;
        paintBuffer=0;
#ifdef __ATHEOS__
	m_bMouseDownSeen = false;
#endif	
    }
    ~KHTMLViewPrivate()
    {
        delete tp; tp = 0;
        delete paintBuffer; paintBuffer =0;
    }
    void reset()
    {
        underMouse = 0;
        linkPressed = false;
        useSlowRepaints = false;
        originalNode = 0;
	borderTouched = false;
        vmode = QScrollView::Auto;
        hmode = QScrollView::Auto;
	borderX = 30;
	borderY = 30;
    }

    QPainter *tp;
    QPixmap  *paintBuffer;
    NodeImpl *underMouse;

    // the node that was selected when enter was pressed
    ElementImpl *originalNode;

    bool borderTouched;
    bool borderStart;

    QScrollView::ScrollBarMode vmode;
    QScrollView::ScrollBarMode hmode;
    bool linkPressed;
    bool useSlowRepaints;

    int borderX, borderY;


#ifdef __ATHEOS__
    os::Messenger m_cMsgTarget;
    bool	  m_bMouseDownSeen;
#endif
    
};


KHTMLView::KHTMLView( KHTMLPart *part, QWidget *parent, const char *name)
    : QScrollView( parent, name, WResizeNoErase | WRepaintNoErase )
{
    m_part = part;
    d = new KHTMLViewPrivate;
    QScrollView::setVScrollBarMode(d->vmode);
    QScrollView::setHScrollBarMode(d->hmode);

    // initialize QScrollview
    enableClipper(true);
    viewport()->setMouseTracking(true);
    viewport()->setBackgroundMode(NoBackground);

#ifdef __ATHEOS__
//    printf( "Warning: KHTMLView::KHTMLView() not setting cursor\n" );
#else
    KImageIO::registerFormats();
    setCursor(arrowCursor);
#endif
    init();

    viewport()->show();
}

KHTMLView::~KHTMLView()
{
    if (m_part)
    {
        //WABA: Is this Ok? Do I need to deref it as well?
        //Does this need to be done somewhere else?
        DOM::DocumentImpl *doc = m_part->xmlDocImpl();
        if (doc)
            doc->detach();
    }
    lstViews->removeRef( this );
    if(lstViews->isEmpty())
    {
        delete lstViews;
        lstViews = 0;
    }

    delete d;
}

void KHTMLView::init()
{
    if ( lstViews == 0L )
        lstViews = new QList<KHTMLView>;
    lstViews->setAutoDelete( FALSE );
    lstViews->append( this );

    if(!d->paintBuffer) d->paintBuffer = new QPixmap(PAINT_BUFFER_HEIGHT, PAINT_BUFFER_HEIGHT);
   if(!d->tp) d->tp = new QPainter();

    setFocusPolicy(QWidget::StrongFocus);
    viewport()->setFocusPolicy( QWidget::WheelFocus );
    viewport()->setFocusProxy(this);

  _marginWidth = -1; // undefined
  _marginHeight = -1;
  _width = 0;
  _height = 0;

  setAcceptDrops(true);
  resizeContents(visibleWidth(), visibleHeight());
}

void KHTMLView::clear()
{
    viewport()->erase();

    if(d->useSlowRepaints)
        setStaticBackground(false);

    d->reset();
    emit cleared();
    QScrollView::setHScrollBarMode(d->hmode);       
    if (d->vmode==Auto)
        QScrollView::setVScrollBarMode(AlwaysOn);
    else
        QScrollView::setVScrollBarMode(d->vmode);    
    resizeContents(visibleWidth(), visibleHeight());    
}

#ifndef __ATHEOS__
void KHTMLView::hideEvent(QHideEvent* e)
{
//    viewport()->setBackgroundMode(PaletteBase);
    QScrollView::hideEvent(e);
}

void KHTMLView::showEvent(QShowEvent* e)
{
//    viewport()->setBackgroundMode(NoBackground);
    QScrollView::showEvent(e);
}

void KHTMLView::resizeEvent (QResizeEvent* e)
{
    QScrollView::resizeEvent(e);

    int w = visibleWidth();
    int h = visibleHeight();

    layout();

    //  this is to make sure we get the right width even if the scrolbar has dissappeared
    // due to the size change.
    if(visibleHeight() != h || visibleWidth() != w)
        layout();

    KApplication::sendPostedEvents(viewport(), QEvent::Paint);
}
#endif // __ATHEOS__

void KHTMLView::drawContents( QPainter *p, int ex, int ey, int ew, int eh )
{
    if(!m_part->xmlDocImpl()) {
#ifdef __ATHEOS__
//	printf( "Warning: KHTMLView::drawContents:1() not calling fillrect\n" );
        p->fillRect(ex, ey, ew, eh, QColor( 255, 255, 255 ) );
#else
        p->fillRect(ex, ey, ew, eh, palette().normal().brush(QColorGroup::Base));
#endif
        return;
    }

    //kdDebug( 6000 ) << "drawContents x=" << ex << ",y=" << ey << ",w=" << ew << ",h=" << eh << endl;

    if ( d->paintBuffer->width() < visibleWidth() ) {
        d->paintBuffer->resize(visibleWidth(),PAINT_BUFFER_HEIGHT);
    }
    int py=0;
    while (py < eh) {
        int ph = eh-py < PAINT_BUFFER_HEIGHT ? eh-py : PAINT_BUFFER_HEIGHT;
        d->tp->begin(d->paintBuffer);
        d->tp->translate(-ex, -ey-py);
#ifdef __ATHEOS__
//	printf( "Warning: KHTMLView::drawContents:2() not calling fillrect\n" );
        d->tp->fillRect(ex, ey+py, ew, ph, QColor( 255, 255, 255 ) );
#else
        d->tp->fillRect(ex, ey+py, ew, ph, palette().normal().brush(QColorGroup::Base));
#endif
        m_part->xmlDocImpl()->renderer()->print(d->tp, ex, ey+py, ew, ph, 0, 0);
        d->tp->end();

        p->drawPixmap(ex, ey+py, *d->paintBuffer, 0, 0, ew, ph);
        py += PAINT_BUFFER_HEIGHT;
    }

#ifdef __ATHEOS__
//	printf( "Warning: KHTMLView::drawContents:2() not sending event\n" );
#else
    khtml::DrawContentsEvent event( p, ex, ey, ew, eh );
    QApplication::sendEvent( m_part, &event );
#endif
}

void KHTMLView::layout(bool)
{
    //### take care of frmaes (hide scrollbars,...)
    if( m_part->xmlDocImpl() ) {
        DOM::DocumentImpl *document = m_part->xmlDocImpl();

        khtml::RenderRoot* root = static_cast<khtml::RenderRoot *>(document->renderer());

        if (document->isHTMLDocument()) {
            NodeImpl *body = static_cast<HTMLDocumentImpl*>(document)->body();
            if(body && body->id() == ID_FRAMESET) {
                QScrollView::setVScrollBarMode(AlwaysOff);
                QScrollView::setHScrollBarMode(AlwaysOff);
                _width = visibleWidth();
                body->renderer()->setLayouted(false);
                body->renderer()->layout();
                root->layout();
                return;
            }
        }

        _height = visibleHeight();
        _width = visibleWidth();

        //QTime qt;
        //qt.start();
            root->layout();
            //kdDebug( 6000 ) << "TIME: layout() dt=" << qt.elapsed() << endl;
    } else {
        _width = visibleWidth();
    }
}

//
// Event Handling
//
/////////////////


void KHTMLView::viewportMousePressEvent( QMouseEvent *_mouse )
{
    if(!m_part->xmlDocImpl()) return;

    int xm, ym;
    viewportToContents(_mouse->x(), _mouse->y(), xm, ym);

    //kdDebug( 6000 ) << "\nmousePressEvent: x=" << xm << ", y=" << ym << endl;


    // Make this frame the active one
    // ### need some visual indication for the active frame.
    /* ### use PartManager (Simon)
       if ( _isFrame && !_isSelected )
       {
        kdDebug( 6000 ) << "activating frame!" << endl;
        topView()->setFrameSelected(this);
    }*/

    DOM::NodeImpl::MouseEvent mev( _mouse->stateAfter(), DOM::NodeImpl::MousePress );
    m_part->xmlDocImpl()->mouseEvent( xm, ym, 0, 0, &mev );

    d->underMouse = mev.innerNode.handle();

    khtml::MousePressEvent event( _mouse, xm, ym, mev.url, mev.innerNode );
    event.setNodePos( mev.nodeAbsX, mev.nodeAbsY );


#ifdef __ATHEOS__
    m_part->event( &event );
#else    
    QApplication::sendEvent( m_part, &event );
#endif    
    emit m_part->nodeActivated(mev.innerNode);
}
#ifndef __ATHEOS__

void KHTMLView::viewportMouseDoubleClickEvent( QMouseEvent *_mouse )
{
    if(!m_part->xmlDocImpl()) return;

    int xm, ym;
    viewportToContents(_mouse->x(), _mouse->y(), xm, ym);

    kdDebug( 6000 ) << "mouseDblClickEvent: x=" << xm << ", y=" << ym << endl;

    DOM::NodeImpl::MouseEvent mev( _mouse->stateAfter(), DOM::NodeImpl::MouseDblClick );
    m_part->xmlDocImpl()->mouseEvent( xm, ym, 0, 0, &mev );

    khtml::MouseDoubleClickEvent event( _mouse, xm, ym, mev.url, mev.innerNode );
    event.setNodePos( mev.nodeAbsX, mev.nodeAbsY );
    QApplication::sendEvent( m_part, &event );

    // ###
    //if ( url.length() )
    //emit doubleClick( url.string(), _mouse->button() );
}

#endif // __ATHEOS__

void KHTMLView::viewportMouseMoveEvent( QMouseEvent * _mouse )
{
    if(!m_part->xmlDocImpl()) return;

    int xm, ym;
    viewportToContents(_mouse->x(), _mouse->y(), xm, ym);

    DOM::NodeImpl::MouseEvent mev( _mouse->stateAfter(), DOM::NodeImpl::MouseMove );
    m_part->xmlDocImpl()->mouseEvent( xm, ym, 0, 0, &mev );

    // execute the scheduled script. This is to make sure the mouseover events come after the mouseout events
    m_part->executeScheduledScript();
    d->underMouse = mev.innerNode.handle();
#ifndef __ATHEOS__
    QCursor c = KCursor::arrowCursor();
    if ( !mev.innerNode.isNull() && mev.innerNode.handle()->style() ) {
        switch( mev.innerNode.handle()->style()->cursor() ) {
        case CURSOR_AUTO:
            if ( mev.url.length() && const_cast<KHTMLSettings *>(m_part->settings())->changeCursor() )
                c = m_part->urlCursor();
            break;
        case CURSOR_CROSS:
            c = KCursor::crossCursor();
            break;
        case CURSOR_POINTER:
            c = m_part->urlCursor();
            break;
        case CURSOR_MOVE:
            c = KCursor::sizeAllCursor();
            break;
        case CURSOR_E_RESIZE:
        case CURSOR_W_RESIZE:
            c = KCursor::sizeHorCursor();
            break;
        case CURSOR_N_RESIZE:
        case CURSOR_S_RESIZE:
            c = KCursor::sizeVerCursor();
            break;
        case CURSOR_NE_RESIZE:
        case CURSOR_SW_RESIZE:
            c = KCursor::sizeBDiagCursor();
            break;
        case CURSOR_NW_RESIZE:
        case CURSOR_SE_RESIZE:
            c = KCursor::sizeFDiagCursor();
            break;
        case CURSOR_TEXT:
            c = KCursor::ibeamCursor();
            break;
        case CURSOR_WAIT:
            c = KCursor::waitCursor();
            break;
        case CURSOR_HELP:
        case CURSOR_DEFAULT:
            break;
        }
    }
    setCursor( c );
#endif // __ATHEOS__
    khtml::MouseMoveEvent event( _mouse, xm, ym, mev.url, mev.innerNode );
    event.setNodePos( mev.nodeAbsX, mev.nodeAbsY );
#ifdef __ATHEOS__
    m_part->event( &event );
#else    
    QApplication::sendEvent( m_part, &event );
#endif
}


void KHTMLView::viewportMouseReleaseEvent( QMouseEvent * _mouse )
{
    if ( !m_part->xmlDocImpl() ) return;

    int xm, ym;
    viewportToContents(_mouse->x(), _mouse->y(), xm, ym);

    //kdDebug( 6000 ) << "\nmouseReleaseEvent: x=" << xm << ", y=" << ym << endl;

    DOM::NodeImpl::MouseEvent mev( _mouse->stateAfter(), DOM::NodeImpl::MouseRelease );
    m_part->xmlDocImpl()->mouseEvent( xm, ym, 0, 0, &mev );

    khtml::MouseReleaseEvent event( _mouse, xm, ym, mev.url, mev.innerNode );
    event.setURLHandlingEnabled( mev.urlHandling );
    event.setNodePos( mev.nodeAbsX, mev.nodeAbsY );
#ifdef __ATHEOS__
    m_part->event( &event );
#else    
    QApplication::sendEvent( m_part, &event );
#endif    
}

#ifndef __ATHEOS__

void KHTMLView::keyPressEvent( QKeyEvent *_ke )
{
//    if(m_part->keyPressHook(_ke)) return;

    int offs = (clipper()->height() < 30) ? clipper()->height() : 30;
    if (_ke->state()&ShiftButton)
      switch(_ke->key())
        {
        case Key_Space:
            scrollBy( 0, -clipper()->height() - offs );
            break;
        }
    else
        switch ( _ke->key() )
        {
        case Key_Down:
        case Key_J:
            scrollBy( 0, 10 );
            break;

        case Key_Space:
        case Key_Next:
            scrollBy( 0, clipper()->height() - offs );
            break;

        case Key_Up:
        case Key_K:
            scrollBy( 0, -10 );
            break;

        case Key_Prior:
            scrollBy( 0, -clipper()->height() + offs );
            break;
        case Key_Right:
        case Key_L:
            scrollBy( 10, 0 );
            break;
        case Key_Left:
        case Key_H:
            scrollBy( -10, 0 );
            break;
        case Key_Enter:
        case Key_Return:
            if (m_part->xmlDocImpl())
	    {
		ElementImpl *e = m_part->xmlDocImpl()->focusNode();
		if (e)
		    e->setActive();
		d->originalNode = e;
	    }
            break;
        case Key_Home:
            setContentsPos( 0, 0 );
            break;
        case Key_End:
            setContentsPos( 0, contentsHeight() - height() );
            break;
        default:
	    _ke->ignore();
            return;
        }
    _ke->accept();
}

void KHTMLView::keyReleaseEvent( QKeyEvent *_ke )
{
    switch(_ke->key())
    {
    case Key_Enter:
    case Key_Return:
	if (m_part->xmlDocImpl())
	{
	    ElementImpl *e = m_part->xmlDocImpl()->focusNode();
	    if (e && e==d->originalNode && (e->id()==ID_A || e->id()==ID_AREA))
	    {
		HTMLAnchorElementImpl *a = static_cast<HTMLAnchorElementImpl *>(e);
		emit m_part->urlSelected( a->areaHref().string(),
					  LeftButton, 0,
					  a->targetRef().string() );
	    }
            if (e)
	        e->setActive(false);
	}
        return;
      break;
    }
    //    if(m_part->keyReleaseHook(_ke)) return;
    QScrollView::keyReleaseEvent( _ke);
}
#endif // __ATHEOS__

bool KHTMLView::focusNextPrevChild( bool next )
{
    if (focusWidget()!=this)
	setFocus();
    if (m_part->xmlDocImpl() && gotoLink(next))
	return true;
    if (m_part->parentPart() && m_part->parentPart()->view())
	return m_part->parentPart()->view()->focusNextPrevChild(next);
    m_part->overURL(QString(), 0);

    return QWidget::focusNextPrevChild(next);
}

void KHTMLView::doAutoScroll()
{
#ifdef __ATHEOS__
    os::Point cMousePos;
    GetMouse( &cMousePos, NULL );
    QPoint pos( cMousePos.x, cMousePos.y );
#else
    QPoint pos = QCursor::pos();
    pos = viewport()->mapFromGlobal( pos );
#endif

    int xm, ym;
    viewportToContents(pos.x(), pos.y(), xm, ym);

    pos = QPoint(pos.x() - viewport()->x(), pos.y() - viewport()->y());
    if ( (pos.y() < 0) || (pos.y() > visibleHeight()) ||
         (pos.x() < 0) || (pos.x() > visibleWidth()) )
    {
        ensureVisible( xm, ym, 0, 5 );
    }
}

DOM::NodeImpl *KHTMLView::nodeUnderMouse() const
{
    return d->underMouse;
}

bool KHTMLView::scrollTo(const QRect &bounds)
{
    int x, y, xe, ye;
    x = bounds.left();
    y = bounds.top();
    xe = bounds.right();
    ye = bounds.bottom();

    kdDebug(6000)<<"scrolling coords: x="<<x<<" y="<<y<<" width="<<xe-x<<" height="<<ye-y<<endl;

    int deltax;
    int deltay;

    int curHeight = visibleHeight();
    int curWidth = visibleWidth();

    if (ye-y>curHeight-d->borderY)
	ye  = y + curHeight - d->borderY;

    if (xe-x>curWidth-d->borderX)
	xe = x + curWidth - d->borderX;

    // is xpos of target left of the view's border?
    if (x < contentsX() + d->borderX )
            deltax = x - contentsX() - d->borderX;
    // is xpos of target right of the view's right border?
    else if (xe + d->borderX > contentsX() + curWidth)
            deltax = xe + d->borderX - ( contentsX() + curWidth );
    else
        deltax = 0;

    // is ypos of target above upper border?
    if (y < contentsY() + d->borderY)
            deltay = y - contentsY() - d->borderY;
    // is ypos of target below lower border?
    else if (ye + d->borderY > contentsY() + curHeight)
            deltay = ye + d->borderY - ( contentsY() + curHeight );
    else
        deltay = 0;

    int maxx = curWidth-d->borderX;
    int maxy = curHeight-d->borderY;

    int scrollX,scrollY;

    scrollX = deltax > 0 ? (deltax > maxx ? maxx : deltax) : deltax == 0 ? 0 : (deltax>-maxx ? deltax : -maxx);
    scrollY = deltay > 0 ? (deltay > maxy ? maxy : deltay) : deltay == 0 ? 0 : (deltay>-maxy ? deltay : -maxy);

    if (contentsX() + scrollX < 0)
	scrollX = -contentsX();
    else if (contentsWidth() - visibleWidth() - contentsX() < scrollX)
	scrollX = contentsWidth() - visibleWidth() - contentsX();

    if (contentsY() + scrollY < 0)
	scrollY = -contentsY();
    else if (contentsHeight() - visibleHeight() - contentsY() < scrollY)
	scrollY = contentsHeight() - visibleHeight() - contentsY();

    scrollBy(scrollX, scrollY);

    // generate abs(scroll.)
    if (scrollX<0)
        scrollX=-scrollX;
    if (scrollY<0)
        scrollY=-scrollY;

    if ( (scrollX!=maxx) && (scrollY!=maxy) )
	return true;
    else return false;

}

bool KHTMLView::gotoLink(bool forward)
{
    if (!m_part->xmlDocImpl())
        return false;

    ElementImpl *currentNode = m_part->xmlDocImpl()->focusNode();
    ElementImpl *nextTarget = m_part->xmlDocImpl()->findNextLink(forward);

    if (!currentNode && !d->borderTouched)
    {
	d->borderStart = forward;
	d->borderTouched = true;

	if (contentsY() != (forward?0:(contentsHeight()-visibleHeight())))
        {
	    setContentsPos(contentsX(), (forward?0:contentsHeight()));
	    if (nextTarget)
	    {
		QRect nextRect = nextTarget->getRect();
		if (nextRect.top()  < contentsY() ||
		    nextRect.bottom() > contentsY()+visibleHeight())
		    return true;
	    }
	    else return true;
	}
    }

    if (!nextTarget || (!currentNode && d->borderStart != forward))
	nextTarget = 0;

    QRect nextRect;
    if (nextTarget)
	nextRect = nextTarget->getRect();
    else
	nextRect = QRect(contentsX()+visibleWidth()/2, (forward?contentsHeight():0), 0, 0);

    if (scrollTo(nextRect))
    {
	if (!nextTarget)
	{
	    if (m_part->xmlDocImpl()->focusNode()) m_part->xmlDocImpl()->setFocusNode(0);
	    d->borderTouched = false;
	    return false;
	}
	else
	{
	    HTMLAnchorElementImpl *anchor = 0;
	    if ( ( nextTarget->id() == ID_A || nextTarget->id() == ID_AREA ) )
		anchor = static_cast<HTMLAnchorElementImpl *>( nextTarget );

	    if (anchor && !anchor->areaHref().isNull()) m_part->overURL(anchor->areaHref().string(), 0);
	    else m_part->overURL(QString(), 0);

	    kdDebug(6000)<<"reached link:"<<nextTarget->nodeName().string()<<endl;

	    m_part->xmlDocImpl()->setFocusNode(nextTarget);
	    emit m_part->nodeActivated(Node(nextTarget));
	}
    }
    return true;
}

bool KHTMLView::gotoNextLink()
{ return gotoLink(true); }

bool KHTMLView::gotoPrevLink()
{ return gotoLink(false); }

void KHTMLView::print()
{
#ifdef __ATHEOS__
    printf( "Warning: KHTMLView::print() not implemented\n" );
#else    
    if(!m_part->xmlDocImpl()) return;
    khtml::RenderRoot *root = static_cast<khtml::RenderRoot *>(m_part->xmlDocImpl()->renderer());
    if(!root) return;

    QPrinter *printer = new QPrinter;
    if(printer->setup(this)) {
        QApplication::setOverrideCursor( waitCursor );
        // set up QPrinter
        printer->setFullPage(false);
        printer->setCreator("KDE 2.1 HTML Library");
        QString docname = m_part->xmlDocImpl()->URL().string();
        if ( !docname.isEmpty() )
	    printer->setDocName(docname);

        QPainter *p = new QPainter;
        p->begin( printer );
	khtml::setPrintPainter( p );

        m_part->xmlDocImpl()->setPaintDevice( printer );

        QPaintDeviceMetrics metrics( printer );

        // this is a simple approximation... we layout the document
        // according to the width of the page, then just cut
        // pages without caring about the content. We should do better
        // in the future, but for the moment this is better than no
        // printing support
        kdDebug(6000) << "printing: physical page width = " << metrics.width()
                      << " height = " << metrics.height() << endl;
        root->setPrintingMode(true);
        root->setWidth(metrics.width());

        QValueList<int> oldSizes = m_part->fontSizes();

        const int printFontSizes[] = { 6, 7, 8, 10, 12, 14, 18, 24,
                                       28, 34, 40, 48, 56, 68, 82, 100, 0 };
        QValueList<int> fontSizes;
        for ( int i = 0; printFontSizes[i] != 0; i++ )
            fontSizes << printFontSizes[ i ];
        m_part->setFontSizes(fontSizes);
        m_part->xmlDocImpl()->applyChanges();

        root->updateSize();

        // ok. now print the pages.
        kdDebug(6000) << "printing: html page width = " << root->docWidth()
                      << " height = " << root->docHeight() << endl;
        kdDebug(6000) << "printing: margins left = " << printer->margins().width()
                      << " top = " << printer->margins().height() << endl;
        kdDebug(6000) << "printing: paper width = " << metrics.width()
                      << " height = " << metrics.height() << endl;
        // if the width is too large to fit on the paper we just scale
        // the whole thing.
        int pageHeight = metrics.height();
        int pageWidth = metrics.width();
        // We print the bottom 'overlap' units again at the top of the next page.
        int overlap = 30;
        p->setClipRect(0,0, pageWidth, pageHeight);
        if(root->docWidth() > metrics.width()) {
            double scale = ((double) metrics.width())/((double) root->docWidth());
            p->scale(scale, scale);
            pageHeight = (int) (pageHeight/scale);
            pageWidth = (int) (pageWidth/scale);
            overlap = (int) (overlap/scale);
        }
        kdDebug(6000) << "printing: scaled html width = " << pageWidth
                      << " height = " << pageHeight << endl;
        int top = 0;
        while(top < root->docHeight()) {
            if(top > 0) printer->newPage();

            root->print(p, 0, top, pageWidth, pageHeight, 0, 0);
            p->translate(0,-(pageHeight-overlap));
            top += (pageHeight-overlap);
        }

        p->end();
        delete p;

        // and now reset the layout to the usual one...
        root->setPrintingMode(false);
	khtml::setPrintPainter( 0 );
        m_part->xmlDocImpl()->setPaintDevice( this );
        m_part->setFontSizes(oldSizes);
        m_part->xmlDocImpl()->applyChanges();
        QApplication::restoreOverrideCursor();
    }
    delete printer;
#endif // __ATHEOS__
}

void KHTMLView::paint(QPainter *p, const QRect &rc, int yOff, bool *more)
{
    if(!m_part->xmlDocImpl()) return;
    khtml::RenderRoot *root = static_cast<khtml::RenderRoot *>(m_part->xmlDocImpl()->renderer());
    if(!root) return;

    m_part->xmlDocImpl()->setPaintDevice(p->device());
    root->setPrintingMode(true);
    root->setWidth(rc.width());

    p->save();
    p->setClipRect(rc);
    p->translate(rc.left(), rc.top());
    double scale = ((double) rc.width()/(double) root->docWidth());
    int height = (int) ((double) rc.height() / scale);
    p->scale(scale, scale);

    root->print(p, 0, yOff, root->docWidth(), height, 0, 0);
    if (more)
        *more = yOff + height < root->docHeight();
    p->restore();

    root->setPrintingMode(false);
    m_part->xmlDocImpl()->setPaintDevice( this );
}


void KHTMLView::useSlowRepaints()
{
    kdDebug(0) << "slow repaints requested" << endl;
    d->useSlowRepaints = true;
    setStaticBackground(true);
}


void KHTMLView::setVScrollBarMode ( ScrollBarMode mode )
{
    d->vmode = mode;
    QScrollView::setVScrollBarMode(mode);
}

void KHTMLView::setHScrollBarMode ( ScrollBarMode mode )
{
    d->hmode = mode;
    QScrollView::setHScrollBarMode(mode);
}

void KHTMLView::restoreScrollBar ( )
{
    int ow = visibleWidth();
    QScrollView::setVScrollBarMode(d->vmode);
    if (visibleWidth() != ow)
    {
        layout();
        updateContents(contentsX(),contentsY(),visibleWidth(),visibleHeight());
    }
}

#ifdef __ATHEOS__

void KHTMLView::SetMsgTarget( const os::Messenger& cMsgTarget )
{
    d->m_cMsgTarget = cMsgTarget;
}

os::Messenger& KHTMLView::GetMsgTarget()
{
    return( d->m_cMsgTarget );
}

void KHTMLView::AllAttached()
{
}

void KHTMLView::AllDetached()
{
}

void KHTMLView::Activated( bool /*bIsActive*/ )
{
}

static int convert_mouse_buttons( uint32 nButtons, uint32 nQualifiers )
{
    int nButtonState = 0;
    if ( nButtons & 0x01 ) {
	nButtonState |= Qt::LeftButton;
    }
    if ( nButtons & 0x02 ) {
	nButtonState |= Qt::RightButton;
    }
    if ( nButtons & 0x04 ) {
	nButtonState |= Qt::MidButton;
    }
    if ( nQualifiers & os::QUAL_SHIFT ) {
	nButtonState |= Qt::ShiftButton;
    }
    if ( nQualifiers & os::QUAL_CTRL ) {
	nButtonState |= Qt::ControlButton;
    }
    if ( nQualifiers & os::QUAL_ALT ) {
	nButtonState |= Qt::AltButton;
    }
    return( nButtonState );
}

void KHTMLView::MouseMove( const os::Point& cNewPos, int /*nCode*/, uint32 nButtons, os::Message* /*pcData*/ )
{
/*    
	NoButton	= 0x0000,
	LeftButton	= 0x0001,
	RightButton	= 0x0002,
	MidButton	= 0x0004,
	MouseButtonMask = 0x0007,
	ShiftButton	= 0x0008,
	ControlButton   = 0x0010,
	AltButton	= 0x0020,
	KeyButtonMask	= 0x0038,
	Keypad		= 0x4000
	*/

    int nButtonState = convert_mouse_buttons( nButtons, GetQualifiers() );

    os::Point cGlobalPos = ConvertToScreen( cNewPos );
    QMouseEvent cEvent( QEvent::MouseMove, QPoint( cNewPos.x, cNewPos.y ), QPoint( cGlobalPos.x, cGlobalPos.y ), 0, nButtonState );
    viewportMouseMoveEvent( &cEvent );
}

void KHTMLView::MouseDown( const os::Point& cPosition, uint32 nButton )
{
    d->m_bMouseDownSeen = true;
    MakeFocus( true );
    uint32 nButtons;

    GetMouse( NULL, &nButtons );
	
    int nButtonState = convert_mouse_buttons( nButtons, GetQualifiers() );
    nButton = convert_mouse_buttons( 1 << (nButton-1), 0 );

    os::Point cGlobalPos = ConvertToScreen( cPosition );
    QMouseEvent cEvent( QEvent::MouseButtonPress, QPoint( cPosition.x, cPosition.y ), QPoint( cGlobalPos.x, cGlobalPos.y ), nButton, nButtonState );
    
    viewportMousePressEvent( &cEvent );
}

void KHTMLView::MouseUp( const os::Point& cPosition, uint32 nButton, os::Message* /*pcData*/ )
{
    uint32 nButtons;

    if ( d->m_bMouseDownSeen == false ) {
	return;
    }
    d->m_bMouseDownSeen = false;
    GetMouse( NULL, &nButtons );
	
    int nButtonState = convert_mouse_buttons( nButtons, GetQualifiers() );
    nButton = convert_mouse_buttons( 1 << (nButton-1), 0 );
    
    os::Point cGlobalPos = ConvertToScreen( cPosition );
    QMouseEvent cEvent( QEvent::MouseButtonRelease, QPoint( cPosition.x, cPosition.y ), QPoint( cGlobalPos.x, cGlobalPos.y ), nButton, nButtonState );
    
    viewportMouseReleaseEvent( &cEvent );
}

void KHTMLView::KeyDown( const char* pzString, const char* pzRawString, uint32 nQualifiers )
{
    os::Rect cBounds = GetBounds();
    int offs = (cBounds.Height() < 30) ? int(cBounds.Height()) : 30;
    if ( nQualifiers & os::QUAL_SHIFT ) {
	switch( pzRawString[0] )
        {
	    case ' ':
		scrollBy( 0, -cBounds.Height() - offs );
		break;
	    default:
		QScrollView::KeyDown( pzString, pzRawString, nQualifiers );
		break;
        }
    } else {
        switch ( pzRawString[0] )
        {
	    case os::VK_DOWN_ARROW:
	    case 'j':
		scrollBy( 0, 10 );
		break;

	    case ' ':
	    case os::VK_PAGE_DOWN:
		scrollBy( 0, cBounds.Height() - offs );
		break;

	    case os::VK_UP_ARROW:
	    case 'K':
		scrollBy( 0, -10 );
		break;

	    case os::VK_PAGE_UP:
		scrollBy( 0, -cBounds.Height() + offs );
		break;
	    case os::VK_RIGHT_ARROW:
	    case 'l':
		scrollBy( 10, 0 );
		break;
	    case os::VK_LEFT_ARROW:
	    case 'h':
		scrollBy( -10, 0 );
		break;
	    case os::VK_ENTER:
//	    case os::VK_RETURN:
		if (m_part->xmlDocImpl())
		{
		    ElementImpl *e = m_part->xmlDocImpl()->focusNode();
		    if (e)
			e->setActive();
		    d->originalNode = e;
		}
		break;
	    case os::VK_HOME:
		setContentsPos( 0, 0 );
		break;
	    case os::VK_END:
		setContentsPos( 0, contentsHeight() - height() );
		break;
	    default:
		QScrollView::KeyDown( pzString, pzRawString, nQualifiers );
		break;
        }
    }
}

void KHTMLView::KeyUp( const char* pzString, const char* pzRawString, uint32 nQualifiers )
{
    QScrollView::KeyUp( pzString, pzRawString, nQualifiers );
}

void KHTMLView::FrameSized( const os::Point& cDelta )
{
    QScrollView::FrameSized( cDelta );
    
    int w = visibleWidth();
    int h = visibleHeight();

    
    layout();


    //  this is to make sure we get the right width even if the scrolbar has dissappeared
    // due to the size change.
    if(visibleHeight() != h || visibleWidth() != w)
        layout();
//    QPainter cPainter( this );
//    drawContents( &cPainter, 0, 0, w, h );
}

#endif
