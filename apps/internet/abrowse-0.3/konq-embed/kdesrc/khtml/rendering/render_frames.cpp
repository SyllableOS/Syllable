/**
 * This file is part of the KDE project.
 *
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 2000 Simon Hausmann <hausmann@kde.org>
 *           (C) 2000 Stefan Schimanski (1Stein@gmx.de)
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
 *
 * $Id$
 */
//#define DEBUG_LAYOUT

#include "render_frames.h"
#include "html_baseimpl.h"
#include "html_objectimpl.h"
#include "htmltags.h"
#include "khtmlview.h"
#include "khtml_part.h"

#include <kapp.h>
#include <kdebug.h>

#ifdef __ATHEOS__
#include <qcursor.h>
#include <qframe.h>
#else
#include <qlabel.h>
#endif
#include <qstringlist.h>

#include <assert.h>
#include <kdebug.h>

using namespace khtml;
using namespace DOM;


RenderFrameSet::RenderFrameSet( HTMLFrameSetElementImpl *frameSet, KHTMLView *view,
                                QList<khtml::Length> *rows, QList<khtml::Length> *cols )
: RenderBox()
{
  // init RenderObject attributes
    setInline(false);

  m_frameset = frameSet;

  m_rows = rows;
  m_cols = cols;
  m_rowHeight = 0;
  m_colWidth = 0;

  m_resizing = false;

  m_hSplit = -1;
  m_vSplit = -1;

  m_hSplitVar = 0;
  m_vSplitVar = 0;

  m_view = view;
}

RenderFrameSet::~RenderFrameSet()
{
  if ( m_rowHeight ) {
    delete [] m_rowHeight;
    m_rowHeight = 0;
  }
  if ( m_colWidth ) {
    delete [] m_colWidth;
    m_colWidth = 0;
  }

  delete [] m_hSplitVar;
  delete [] m_vSplitVar;
}

void RenderFrameSet::layout( )
{

    if ( strcmp( parent()->renderName(), "RenderFrameSet" ) != 0 )
    {
        m_width = m_view->visibleWidth();
        m_height = m_view->visibleHeight();
    }

#ifdef DEBUG_LAYOUT
    kdDebug( 6040 ) << renderName() << "(FrameSet)::layout( ) width=" << width() << ", height=" << height() << endl;
#endif

    int remainingWidth = m_width - (m_frameset->totalCols()-1)*m_frameset->border();
    if(remainingWidth<0) remainingWidth=0;
    int remainingHeight = m_height - (m_frameset->totalRows()-1)*m_frameset->border();
    if(remainingHeight<0) remainingHeight=0;
    int widthAvailable = remainingWidth;
    int heightAvailable = remainingHeight;

    if(m_rowHeight) delete [] m_rowHeight;
    if(m_colWidth) delete [] m_colWidth;
    m_rowHeight = new int[m_frameset->totalRows()];
    m_colWidth = new int[m_frameset->totalCols()];

    int i;
    int totalRelative = 0;
    int colsRelative = 0;
    int rowsRelative = 0;
    int rowsPercent = 0;
    int colsPercent = 0;
    int remainingRelativeWidth = 0;

    if(m_rows)
    {
	// another one for bad html. If all rows have a fixed width, convert the numbers to percentages.
	bool allFixed = true;
	int totalFixed = 0;
	for(i = 0; i< m_frameset->totalRows(); i++) {
	    if(m_rows->at(i)->type != Fixed)
		allFixed = false;
	    else
		totalFixed += m_rows->at(i)->value;
	}
	if ( allFixed && totalFixed ) {
	    for(i = 0; i< m_frameset->totalRows(); i++) {
		m_rows->at(i)->type = Percent;
		m_rows->at(i)->value = m_rows->at(i)->value *100 / totalFixed;
	    }
	}

	    // fixed rows first, then percent and then relative
        for(i = 0; i< m_frameset->totalRows(); i++)
        {
             if(m_rows->at(i)->type == Fixed || m_rows->at(i)->type == Percent)
            {
                m_rowHeight[i] = QMAX(m_rows->at(i)->width(heightAvailable), 14);
                if( m_rowHeight[i] > remainingHeight )
                    m_rowHeight[i] = remainingHeight;
                 remainingHeight -= m_rowHeight[i];
                if( m_rows->at(i)->type == Percent)
                    rowsPercent++;
                if( m_rows->at(i)->type == Percent)
                    rowsPercent++;
            }
            else if(m_rows->at(i)->type == Relative)
            {
                totalRelative += m_rows->at(i)->value;
                rowsRelative++;
            }
        }
        // ###
        if(remainingHeight < 0) remainingHeight = 0;

        if ( !totalRelative && rowsRelative )
          remainingRelativeWidth = remainingHeight/rowsRelative;

        for(i = 0; i< m_frameset->totalRows(); i++)
         {
            if(m_rows->at(i)->type == Relative)
            {
                if ( totalRelative )
                  m_rowHeight[i] = m_rows->at(i)->value*remainingHeight/totalRelative;
                else
                  m_rowHeight[i] = remainingRelativeWidth;
                remainingHeight -= m_rowHeight[i];
                totalRelative--;
            }
        }

        // support for totally broken frame declarations
        if(remainingHeight)
        {
            // just distribute it over all columns...
            int rows = m_frameset->totalRows();
            if ( rowsPercent )
                rows = rowsPercent;
            for(i = 0; i< m_frameset->totalRows(); i++) {
                if( !rowsPercent || m_rows->at(i)->type == Percent ) {
                    int toAdd = remainingHeight/rows;
                    rows--;
                    m_rowHeight[i] += toAdd;
                    remainingHeight -= toAdd;
                }
            }
        }
    }
    else
        m_rowHeight[0] = m_height;

    if(m_cols)
    {
	// another one for bad html. If all cols have a fixed width, convert the numbers to percentages.
	// also reproduces IE and NS behaviour.
	bool allFixed = true;
	int totalFixed = 0;
	for(i = 0; i< m_frameset->totalCols(); i++) {
	    if(m_cols->at(i)->type != Fixed)
		allFixed = false;
	    else
		totalFixed += m_cols->at(i)->value;
	}
	if ( allFixed && totalFixed) {
	    for(i = 0; i< m_frameset->totalCols(); i++) {
		m_cols->at(i)->type = Percent;
		m_cols->at(i)->value = m_cols->at(i)->value * 100 / totalFixed;
	    }
	}

        totalRelative = 0;
        remainingRelativeWidth = 0;

        for(i = 0; i< m_frameset->totalCols(); i++)
        {
            if(m_cols->at(i)->type == Fixed || m_cols->at(i)->type == Percent)
            {
                m_colWidth[i] = QMAX(m_cols->at(i)->width(widthAvailable), 14);
                if( m_colWidth[i] > remainingWidth )
                    m_colWidth[i] = remainingWidth;
                remainingWidth -= m_colWidth[i];
                if( m_cols->at(i)->type == Percent)
                    colsPercent++;
            }
            else if(m_cols->at(i)->type == Relative)
            {
                totalRelative += m_cols->at(i)->value;
                colsRelative++;
            }
        }
        // ###
        if(remainingWidth < 0) remainingWidth = 0;

        if ( !totalRelative && colsRelative )
          remainingRelativeWidth = remainingWidth/colsRelative;

        for(i = 0; i < m_frameset->totalCols(); i++)
        {
            if(m_cols->at(i)->type == Relative)
            {
                if ( totalRelative )
                  m_colWidth[i] = m_cols->at(i)->value*remainingWidth/totalRelative;
                else
                  m_colWidth[i] = remainingRelativeWidth;
                remainingWidth -= m_colWidth[i];
                totalRelative--;
            }
        }

        // support for totally broken frame declarations
        if(remainingWidth)
        {
            // just distribute it over all columns...
            int cols = m_frameset->totalCols();
            if ( colsPercent )
                cols = colsPercent;
            for(i = 0; i< m_frameset->totalCols(); i++) {
                if( !colsPercent || m_cols->at(i)->type == Percent ) {
                    int toAdd = remainingWidth/cols;
                    cols--;
                    m_colWidth[i] += toAdd;
                    remainingWidth -= toAdd;
                }
            }
        }

    }
    else
        m_colWidth[0] = m_width;

    positionFrames();

    RenderObject *child = firstChild();
    if ( !child )
      return;

    if(!m_hSplitVar && !m_vSplitVar)
    {
#ifdef DEBUG_LAYOUT
        kdDebug( 6031 ) << "calculationg fixed Splitters" << endl;
#endif
        if(!m_vSplitVar && m_frameset->totalCols() > 1)
        {
            m_vSplitVar = new bool[m_frameset->totalCols()];
            for(int i = 0; i < m_frameset->totalCols(); i++) m_vSplitVar[i] = true;
        }
        if(!m_hSplitVar && m_frameset->totalRows() > 1)
        {
            m_hSplitVar = new bool[m_frameset->totalRows()];
            for(int i = 0; i < m_frameset->totalRows(); i++) m_hSplitVar[i] = true;
        }

        for(int r = 0; r < m_frameset->totalRows(); r++)
        {
            for(int c = 0; c < m_frameset->totalCols(); c++)
            {
                bool fixed = false;

                if ( strcmp( child->renderName(), "RenderFrameSet" ) == 0 )
                  fixed = static_cast<RenderFrameSet *>(child)->frameSetImpl()->noResize();
                else
                  fixed = static_cast<RenderFrame *>(child)->frameImpl()->noResize();

                /*
                if(child->id() == ID_FRAMESET)
                    fixed = (static_cast<HTMLFrameSetElementImpl *>(child))->noResize();
                else if(child->id() == ID_FRAME)
                    fixed = (static_cast<HTMLFrameElementImpl *>(child))->noResize();
                */

                if(fixed)
                {
#ifdef DEBUG_LAYOUT
                    kdDebug( 6031 ) << "found fixed cell " << r << "/" << c << "!" << endl;
#endif
                    if( m_frameset->totalCols() > 1)
                    {
                        if(c>0) m_vSplitVar[c-1] = false;
                        m_vSplitVar[c] = false;
                    }
                    if( m_frameset->totalRows() > 1)
                    {
                        if(r>0) m_hSplitVar[r-1] = false;
                        m_hSplitVar[r] = false;
                    }
                    child = child->nextSibling();
                    if(!child) goto end2;
                }
#ifdef DEBUG_LAYOUT
                else
                    kdDebug( 6031 ) << "not fixed: " << r << "/" << c << "!" << endl;
#endif
            }
        }

    }
 end2:
    setLayouted();
}

void RenderFrameSet::positionFrames()
{
  int r;
  int c;

  RenderObject *child = firstChild();
  if ( !child )
    return;

  //  NodeImpl *child = _first;
  //  if(!child) return;

  int yPos = 0;

  for(r = 0; r < m_frameset->totalRows(); r++)
  {
    int xPos = 0;
    for(c = 0; c < m_frameset->totalCols(); c++)
    {
    //      HTMLElementImpl *e = static_cast<HTMLElementImpl *>(child);
      child->setPos( xPos, yPos );
#ifdef DEBUG_LAYOUT
      kdDebug(6040) << "child frame at (" << xPos << "/" << yPos << ") size (" << m_colWidth[c] << "/" << m_rowHeight[r] << ")" << endl;
#endif
      child->setSize( m_colWidth[c], m_rowHeight[r] );

      child->layout( );

      xPos += m_colWidth[c] + m_frameset->border();
      child = child->nextSibling();

      if ( !child )
        return;
    /*
            e->renderer()->setPos(xPos, yPos);
            e->setWidth(colWidth[c]);
            e->setAvailableWidth(colWidth[c]);
            e->setDescent(rowHeight[r]);
            e->layout();
            xPos += colWidth[c] + border;
            child = child->nextSibling();
            if(!child) return;
    */
        }
        yPos += m_rowHeight[r] + m_frameset->border();
    }
}

bool RenderFrameSet::userResize( int _x, int _y, DOM::NodeImpl::MouseEventType type )
{
  bool res = false;

  if ( !m_resizing && type == DOM::NodeImpl::MouseMove || type == DOM::NodeImpl::MousePress )
  {
#ifdef DEBUG_LAYOUT
    kdDebug( 6031 ) << "mouseEvent:check" << endl;
#endif

    m_hSplit = -1;
    m_vSplit = -1;
    //bool resizePossible = true;

    // check if we're over a horizontal or vertical boundary
    int pos = m_colWidth[0];
    for(int c = 1; c < m_frameset->totalCols(); c++)
    {
      if(_x >= pos && _x <= pos+m_frameset->border())
      {
        if(m_vSplitVar && m_vSplitVar[c-1] == true) m_vSplit = c-1;
#ifdef DEBUG_LAYOUT
        kdDebug( 6031 ) << "vsplit!" << endl;
#endif
        res = true;
        break;
      }
      pos += m_colWidth[c] + m_frameset->border();
    }

    pos = m_rowHeight[0];
    for(int r = 1; r < m_frameset->totalRows(); r++)
    {
      if( _y >= pos && _y <= pos+m_frameset->border())
      {
        if(m_hSplitVar && m_hSplitVar[r-1] == true) m_hSplit = r-1;
#ifdef DEBUG_LAYOUT
        kdDebug( 6031 ) << "hsplitvar = " << m_hSplitVar << endl;
        kdDebug( 6031 ) << "hsplit!" << endl;
#endif
        res = true;
        break;
      }
      pos += m_rowHeight[r] + m_frameset->border();
    }
#ifdef DEBUG_LAYOUT
    kdDebug( 6031 ) << m_hSplit << "/" << m_vSplit << endl;
#endif

    QCursor cursor;
    if(m_hSplit != -1 && m_vSplit != -1)
    {
      cursor = Qt::sizeAllCursor;
    }
    else if( m_vSplit != -1 )
    {
      cursor = Qt::splitHCursor;
    }
    else if( m_hSplit != -1 )
    {
      cursor = Qt::splitVCursor;
    }

    if(type == DOM::NodeImpl::MousePress)
    {
      m_resizing = true;
#ifdef __ATHEOS__
#if ATHDBLEVEL > 3
      printf( "Warning: RenderFrameSet::userResize() not calling KApplication::setOverrideCursor()\n" );
#endif      
#else
      KApplication::setOverrideCursor(cursor);
#endif
      m_vSplitPos = _x;
      m_hSplitPos = _y;
    }
    else
      static_cast<KHTMLView *>(m_view)->setCursor(cursor);

  }

  // ### need to draw a nice movin indicator for the resize.
  // ### check the resize is not going out of bounds.
  if(m_resizing && type == DOM::NodeImpl::MouseRelease)
  {
    m_resizing = false;
#ifdef __ATHEOS__
#if ATHDBLEVEL > 3
      printf( "Warning: RenderFrameSet::userResize() not calling KApplication::restoreOverrideCursor()\n" );
#endif      
#else
    KApplication::restoreOverrideCursor();
#endif
    if(m_vSplit != -1 )
    {
#ifdef DEBUG_LAYOUT
      kdDebug( 6031 ) << "split xpos=" << _x << endl;
#endif
      int delta = m_vSplitPos - _x;
      m_colWidth[m_vSplit] -= delta;
      m_colWidth[m_vSplit+1] += delta;
    }
    if(m_hSplit != -1 )
    {
#ifdef DEBUG_LAYOUT
      kdDebug( 6031 ) << "split ypos=" << _y << endl;
#endif
      int delta = m_hSplitPos - _y;
      m_rowHeight[m_hSplit] -= delta;
      m_rowHeight[m_hSplit+1] += delta;
    }

    positionFrames( );
  }

  return res;
}

/**************************************************************************************/

RenderPart::RenderPart( QScrollView *view )
: RenderWidget( view )
{
  // init RenderObject attributes
    setInline(false);

  m_view = view;
}

RenderPart::~RenderPart()
{
}

void RenderPart::setWidget( QWidget *widget )
{
  setQWidget(widget);
  if ( m_widget )
    m_widget->show();
  layout();
  repaint();
}

void RenderPart::layout( )
{
  if ( m_widget )
    m_widget->resize( m_width, m_height );
  if (isPositioned()) {
    calcAbsoluteHorizontal();
    calcAbsoluteVertical();
  }
}

void RenderPart::partLoadingErrorNotify()
{
}

/***************************************************************************************/

RenderFrame::RenderFrame( QScrollView *view, DOM::HTMLFrameElementImpl *frame )
: RenderPart( view )
{
  m_frame = frame;
}

RenderFrame::~RenderFrame()
{
}

void RenderFrame::setWidget( QWidget *widget )
{
#ifdef DEBUG_LAYOUT
    kdDebug(6031) << "RenderFrame::setWidget()" << endl;
#endif
    RenderPart::setWidget(widget);
    slotViewCleared();
    if( dynamic_cast<KHTMLView*>(widget) != NULL ) {
        connect( widget, SIGNAL( cleared() ), this, SLOT( slotViewCleared() ) );
    }
}

void RenderFrame::slotViewCleared()
{
    if( dynamic_cast<QScrollView*>(m_widget) != NULL ) {
#ifdef DEBUG_LAYOUT
        kdDebug(6031) << "frame is a scrollview!" << endl;
#endif
        QScrollView *view = static_cast<QScrollView *>(m_widget);
        if(!m_frame->frameBorder || !((static_cast<HTMLFrameSetElementImpl *>(m_frame->_parent))->frameBorder()))
            view->setFrameStyle(QFrame::NoFrame);
        view->setVScrollBarMode(m_frame->scrolling);
        view->setHScrollBarMode(m_frame->scrolling);
        if( dynamic_cast<KHTMLView*>(view) != NULL ) {
#ifdef DEBUG_LAYOUT
            kdDebug(6031) << "frame is a KHTMLview!" << endl;
#endif
            KHTMLView *htmlView = static_cast<KHTMLView *>(view);
            if(m_frame->marginWidth != -1) htmlView->setMarginWidth(m_frame->marginWidth);
            if(m_frame->marginHeight != -1) htmlView->setMarginHeight(m_frame->marginHeight);
        }
    }
}

void RenderFrame::slotWidgetDestructed()
{
    RenderWidget::slotWidgetDestructed();
}

/****************************************************************************************/

RenderPartObject::RenderPartObject( QScrollView *view, DOM::HTMLElementImpl *o )
: RenderPart( view )
{
  // init RenderObject attributes
    setInline(true);

  m_obj = o;
}

RenderPartObject::~RenderPartObject()
{
}

void RenderPartObject::updateWidget()
{
  QString url;
  QString serviceType;

  if(m_obj->id() == ID_OBJECT) {

      // check for embed child object
     HTMLObjectElementImpl *o = static_cast<HTMLObjectElementImpl *>(m_obj);
     HTMLEmbedElementImpl *embed = 0;
     NodeImpl *child = o->firstChild();
     while ( child ) {
        if ( child->id() == ID_EMBED )
           embed = static_cast<HTMLEmbedElementImpl *>( child );

        child = child->nextSibling();
     }

     if ( !embed )
     {
        url = o->url;
        serviceType = o->serviceType;
        if(serviceType.isEmpty() || serviceType.isNull()) {
           if(o->classId.contains(QString::fromLatin1("D27CDB6E-AE6D-11cf-96B8-444553540000"))) serviceType = "application/x-shockwave-flash";

           // add more plugins here
        }

        if((url.isEmpty() || url.isNull())) {
           // look for a SRC attribute in the params
           NodeImpl *child = o->firstChild();
           while ( child ) {
              if ( child->id() == ID_PARAM ) {
                 HTMLParamElementImpl *p = static_cast<HTMLParamElementImpl *>( child );

                 if ( p->name().lower()==QString::fromLatin1("src") ||
                      p->name().lower()==QString::fromLatin1("movie") ||
                      p->name().lower()==QString::fromLatin1("code") )
                 {
                    url = p->value();
                    break;
                 }
              }
              child = child->nextSibling();
           }
        }

        // add all <param>'s to the QStringList argument of the part
        QStringList params;
        NodeImpl *child = o->firstChild();
        while ( child ) {
           if ( child->id() == ID_PARAM ) {
              HTMLParamElementImpl *p = static_cast<HTMLParamElementImpl *>( child );

              QString aStr = p->name();
              aStr += QString::fromLatin1("=\"");
              aStr += p->value();
              aStr += QString::fromLatin1("\"");
              params.append(aStr);
           }

           child = child->nextSibling();
        }

        if ( url.isEmpty() && serviceType.isEmpty() ) {
#ifdef DEBUG_LAYOUT
            kdDebug() << "RenderPartObject::close - empty url and serverType" << endl;
#endif
            return;
        }

        KHTMLPart *part = static_cast<KHTMLView *>(m_view)->part();

        params.append( QString::fromLatin1("NSPLUGINEMBED=\"YES\"") );
        params.append( QString::fromLatin1("NSPLUGINBASEURL=\"%1\"").arg( part->url().url() ) );

        part->requestObject( this, url, serviceType, params );
     } else
     {
        // render embed object
        url = embed->url;
        serviceType = embed->serviceType;

        if ( url.isEmpty() && serviceType.isEmpty() ) {
#ifdef DEBUG_LAYOUT
            kdDebug() << "RenderPartObject::close - empty url and serverType" << endl;
#endif
            return;
        }

        KHTMLPart *part = static_cast<KHTMLView *>(m_view)->part();

        embed->param.append( QString::fromLatin1("NSPLUGINEMBED=\"YES\"") );
        embed->param.append( QString::fromLatin1("NSPLUGINBASEURL=\"%1\"").arg( part->url().url() ) );

        part->requestObject( this, url, serviceType, embed->param );
     }
  } else if ( m_obj->id() == ID_EMBED ) {

     HTMLEmbedElementImpl *o = static_cast<HTMLEmbedElementImpl *>(m_obj);
     url = o->url;
     serviceType = o->serviceType;

     if ( url.isEmpty() && serviceType.isEmpty() ) {
#ifdef DEBUG_LAYOUT
         kdDebug() << "RenderPartObject::close - empty url and serverType" << endl;
#endif
         return;
     }

     KHTMLPart *part = static_cast<KHTMLView *>(m_view)->part();

     o->param.append( QString::fromLatin1("NSPLUGINEMBED=\"YES\"") );
     o->param.append( QString::fromLatin1("NSPLUGINBASEURL=\"%1\"").arg( part->url().url() ) );

     part->requestObject( this, url, serviceType, o->param );

  } else {
      assert(m_obj->id() == ID_IFRAME);
      HTMLIFrameElementImpl *o = static_cast<HTMLIFrameElementImpl *>(m_obj);
      url = o->url.string();
      if( url.isEmpty()) return;
      KHTMLView *v = static_cast<KHTMLView *>(m_view);
      v->part()->requestFrame( this, url, o->name.string(), QStringList(), true );
  }
  setLayouted(false);

}

void RenderPartObject::close()
{
  updateWidget();
  layout();

  RenderPart::close();
}


void RenderPartObject::partLoadingErrorNotify()
{
    HTMLEmbedElementImpl *embed = 0;

    if( m_obj->id()==ID_OBJECT ) {

        // check for embed child object
        HTMLObjectElementImpl *o = static_cast<HTMLObjectElementImpl *>(m_obj);
        NodeImpl *child = o->firstChild();
        while ( child ) {
            if ( child->id() == ID_EMBED )
                embed = static_cast<HTMLEmbedElementImpl *>( child );

            child = child->nextSibling();
        }

    } else if( m_obj->id()==ID_EMBED )
        embed = static_cast<HTMLEmbedElementImpl *>(m_obj);

    // Display vender download page
    if( embed && !embed->pluginPage.isEmpty() ) {
        KHTMLPart *part = static_cast<KHTMLView *>(m_view)->part();
        KParts::BrowserExtension *ext = part->browserExtension();
        if ( ext ) ext->createNewWindow( KURL(embed->pluginPage) );
    }
}


void RenderPartObject::setWidget( QWidget *w )
{
    if( dynamic_cast<KHTMLView*>(w) != NULL ) {
        connect( w, SIGNAL( cleared() ), this, SLOT( slotViewCleared() ) );
    }
    if ( w && m_width == 0 && m_height == 0 ) {
      QSize hint = w->sizeHint();
      m_width = hint.width();
      m_height = hint.height();
      m_minWidth = m_maxWidth = m_width;
  }

  RenderPart::setWidget( w );
  slotViewCleared(); // WABA ??
}

void RenderPartObject::setSize( int w, int h )
{
  m_width = w;
  m_height = h;
  m_minWidth = m_maxWidth = w;
}

void RenderPartObject::layout( )
{
    // ### shouldn't this be relative to the containingBlock?????
  setSize( style()->width().minWidth( m_view->visibleWidth() ),
           style()->height().minWidth( m_view->visibleHeight() ) );
  calcHorizontalMargins(style()->marginLeft(),style()->marginRight(),
          containingBlockWidth());
  RenderPart::layout();
}


void RenderPartObject::slotViewCleared()
{
  if( dynamic_cast<QScrollView*>(m_widget) != NULL ) {
#ifdef DEBUG_LAYOUT
      kdDebug(6031) << "iframe is a scrollview!" << endl;
#endif
      QScrollView *view = static_cast<QScrollView *>(m_widget);
      int frameStyle = QFrame::NoFrame;
      QScrollView::ScrollBarMode scroll = QScrollView::Auto;
      int marginw = 0;
      int marginh = 0;
      if ( m_obj->id() == ID_IFRAME) {
	  HTMLIFrameElementImpl *m_frame = static_cast<HTMLIFrameElementImpl *>(m_obj);
	  if(m_frame->frameBorder)
	      frameStyle = QFrame::Box;
	  scroll = m_frame->scrolling;
	  marginw = m_frame->marginWidth;
	  marginh = m_frame->marginHeight;
      }
      view->setFrameStyle(QFrame::NoFrame);
      view->setVScrollBarMode(scroll);
      view->setHScrollBarMode(scroll);
      if( dynamic_cast<KHTMLView*>(view) != NULL ) {
#ifdef DEBUG_LAYOUT
          kdDebug(6031) << "frame is a KHTMLview!" << endl;
#endif
          KHTMLView *htmlView = static_cast<KHTMLView *>(view);
          if(marginw != -1) htmlView->setMarginWidth(marginw);
          if(marginh != -1) htmlView->setMarginHeight(marginh);
        }
  }
}

void RenderPartObject::slotWidgetDestructed()
{
    RenderWidget::slotWidgetDestructed();
}



#include "render_frames.moc"
