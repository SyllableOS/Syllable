/**
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
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
// -------------------------------------------------------------------------
//#define DEBUG_LAYOUT

#include "dom_string.h"

#include <qpainter.h>
#include <qfontmetrics.h>
#include <qstack.h>

#include "dom_node.h"
#include "dom_textimpl.h"
#include "dom_stringimpl.h"
#include "dom_exception.h"

#include "misc/helper.h"
#include "htmlhashes.h"
#include "khtmlview.h"

#include "render_box.h"
#include "render_style.h"
#include "render_object.h"
#include "render_text.h"
#include "render_replaced.h"
#include "render_root.h"

#include <kdebug.h>
#include <assert.h>

#include "misc/loader.h"

using namespace DOM;
using namespace khtml;


RenderBox::RenderBox()
    : RenderObject()
{
    m_minWidth = -1;
    m_maxWidth = -1;
    m_width = m_height = 0;
    m_x = 0;
    m_y = 0;
    m_marginTop = 0;
    m_marginBottom = 0;
    m_marginLeft = 0;
    m_marginRight = 0;
}

void RenderBox::setStyle(RenderStyle *_style)
{
    RenderObject::setStyle(_style);

    // ### move this into the parser
    // if only horizontal position was defined, vertical should be 50%
    if(!_style->backgroundXPosition().isVariable() && _style->backgroundYPosition().isVariable())
        style()->setBackgroundYPosition(Length(50, Percent));

    switch(_style->position())
    {
    case ABSOLUTE:
    case FIXED:
        setPositioned(true);
        break;
    default:
        setPositioned(false);
        if(_style->isFloating()) {
            setFloating(true);
        } else {
            if(_style->position() == RELATIVE)
                setRelPositioned(true);
        }
    }
}

RenderBox::~RenderBox()
{
    //kdDebug( 6040 ) << "Element destructor: this=" << nodeName().string() << endl;
}

QSize RenderBox::contentSize() const
{
    int w = m_width;
    int h = m_height;
    if(style()->hasBorder())
    {
        w -= borderLeft() + borderRight();
        h -= borderTop() + borderBottom();
    }
    if(style()->hasPadding())
    {
        w -= paddingLeft() + paddingRight();
        h -= paddingTop() + paddingBottom();
    }

    return QSize(w, h);
}

short RenderBox::contentWidth() const
{
    short w = m_width;
    //kdDebug( 6040 ) << "RenderBox::contentWidth(1) = " << m_width << endl;
    if(style()->hasBorder())
        w -= borderLeft() + borderRight();
    if(style()->hasPadding())
        w -= paddingLeft() + paddingRight();

    //kdDebug( 6040 ) << "RenderBox::contentWidth(2) = " << w << endl;
    return w;
}

int RenderBox::contentHeight() const
{
    int h = m_height;
    if(style()->hasBorder())
        h -= borderTop() + borderBottom();
    if(style()->hasPadding())
        h -= paddingTop() + paddingBottom();

    return h;
}

void RenderBox::setPos( int xPos, int yPos )
{
    m_x = xPos; m_y = yPos;
#if 1
    if(containsWidget() && (m_x != xPos || m_y != yPos))
    {
        int x,y;
        absolutePosition(x,y);

        // propagate position change to childs
        for(RenderObject *child = firstChild(); child; child = child->nextSibling()) {
            if(child->isWidget())
                static_cast<RenderWidget*>(child)->placeWidget(x+child->xPos(),y+child->yPos());
        }
    }
#endif
}

QSize RenderBox::contentOffset() const
{
    // ###
    //int xOff = 0;
    //int yOff = 0;
    return QSize(0, 0);
}

QSize RenderBox::paddingSize() const
{
    return QSize(0, 0);
}

QSize RenderBox::size() const
{
    return QSize(0, 0);
}

short RenderBox::width() const
{
    return m_width;
}

int RenderBox::height() const
{
    return m_height;
}


// --------------------- printing stuff -------------------------------

void RenderBox::print(QPainter *p, int _x, int _y, int _w, int _h,
                                  int _tx, int _ty)
{
    _tx += m_x;
    _ty += m_y;

    // default implementation. Just pass things through to the children
    RenderObject *child = firstChild();
    while(child != 0)
    {
        child->print(p, _x, _y, _w, _h, _tx, _ty);
        child = child->nextSibling();
    }
}

void RenderBox::setPixmap(const QPixmap &, const QRect&, CachedImage *image, bool *)
{
    if(image && image->pixmap_size() == image->valid_rect().size() && parent())
        repaint();      //repaint bg when it finished loading
}


void RenderBox::printBoxDecorations(QPainter *p,int, int _y,
                                       int, int _h, int _tx, int _ty)
{
    //kdDebug( 6040 ) << renderName() << "::printDecorations()" << endl;

    int w = width();
    int h = height() + borderTopExtra() + borderBottomExtra();
    _ty -= borderTopExtra();

    int my = QMAX(_ty,_y);
    int mh;
    if (_ty<_y)
        mh= QMAX(0,h-(_y-_ty));
    else
        mh = QMIN(_h,h);

    printBackground(p, style()->backgroundColor(), backgroundImage(), my, mh, _tx, _ty, w, h);

    if(style()->hasBorder())
        printBorder(p, _tx, _ty, w, h, style());
}

void RenderBox::printBackground(QPainter *p, const QColor &c, CachedImage *bg, int clipy, int cliph, int _tx, int _ty, int w, int h)
{
    if(c.isValid())
        p->fillRect(_tx, clipy, w, cliph, c);
    // no progressive loading of the background image
    if(bg && bg->pixmap_size() == bg->valid_rect().size() && !bg->isTransparent() && !bg->isErrorImage()) {
        //kdDebug( 6040 ) << "printing bgimage at " << _tx << "/" << _ty << endl;
        // ### might need to add some correct offsets
        // ### use paddingX/Y

        int sx = 0;
        int sy = 0;

        //hacky stuff
        RenderStyle* sptr = style();
        if ( isHtml() && firstChild() && !backgroundImage() )
            sptr = firstChild()->style();

	int cx = _tx;
	int cy = clipy;
	int cw = w;
	int ch = h;

        // CSS2 chapter 14.2.1
        int pw = m_width - sptr->borderRightWidth() - sptr->borderLeftWidth();
        int ph = m_height - sptr->borderTopWidth() - sptr->borderBottomWidth();
        EBackgroundRepeat bgr = sptr->backgroundRepeat();
        if(bgr == NO_REPEAT || bgr == REPEAT_Y)
            cw = QMIN(bg->pixmap_size().width(), w);
        if(bgr == NO_REPEAT || bgr == REPEAT_X)
            ch = QMIN(bg->pixmap_size().height(), h);

        cx = _tx + sptr->backgroundXPosition().minWidth(pw) - sptr->backgroundXPosition().minWidth(cw);
        cy = _ty + sptr->backgroundYPosition().minWidth(ph) - sptr->backgroundYPosition().minWidth(ch);
        if( !sptr->backgroundAttachment() ) {
            QRect r = viewRect();
            //kdDebug(0) << "fixed background r.y=" << r.y() << endl;
	    if( isHtml() ) {
		cy += r.y();
		cx += r.x();
	    }
            sx = cx - r.x();
            sy = cy - r.y();
        }
        else
            // make sure that the pixmap is tiled correctly
            // because we clip the tiling to the visible area (for speed reasons)
            if(bg->pixmap_size().height() && sptr->backgroundAttachment());

        //        sy = (clipy - _ty) % bg->pixmap_size().height();

	p->drawTiledPixmap(cx, cy, cw, ch, bg->tiled_pixmap(c), sx, sy);
    }
}

void RenderBox::outlineBox(QPainter *p, int _tx, int _ty, const char *color)
{
    p->setPen(QPen(QColor(color), 1, Qt::DotLine));
    p->setBrush( Qt::NoBrush );
    p->drawRect(_tx, _ty, m_width, m_height);
}


void RenderBox::close()
{
    setParsing(false);
    calcWidth();
    calcHeight();
    calcMinMaxWidth();
    if(containingBlockWidth() < m_minWidth && parent())
        containingBlock()->updateSize();
    else if(!isInline() || isReplaced())
    {
        layout();
    }
}

short RenderBox::containingBlockWidth() const
{
    if (style()->htmlHacks() && style()->flowAroundFloats() && containingBlock()->isFlow()
            && style()->width().isVariable())
        return static_cast<RenderFlow*>(containingBlock())->lineWidth(m_y);
    else
        return containingBlock()->contentWidth();
}

bool RenderBox::absolutePosition(int &xPos, int &yPos, bool f)
{
    if ( style()->position() == FIXED )
	f = true;
    RenderObject *o = container();
    if( o && o->absolutePosition(xPos, yPos, f))
    {
        if(!isInline() || isReplaced())
            xPos += m_x, yPos += m_y;

        if(isRelPositioned())
            relativePositionOffset(xPos, yPos);
        return true;
    }
    else
    {
        xPos = yPos = 0;
        return false;
    }
}

void RenderBox::addChild(RenderObject *newChild, RenderObject *beforeChild)
{
    RenderObject::addChild(newChild, beforeChild);

    if(newChild->isWidget())
    {
        RenderObject* o = this;
        while(o) {
            o->setContainsWidget();
            o = o->parent();
        }
    }
}

void RenderBox::updateSize()
{

#ifdef DEBUG_LAYOUT
    kdDebug( 6040 ) << renderName() << "(RenderBox) " << this << " ::updateSize()" << endl;
#endif


    int oldMin = m_minWidth;
    int oldMax = m_maxWidth;
    setMinMaxKnown(false);
    calcMinMaxWidth();

    if ((isInline() || isFloating() || containsSpecial()) && parent())
    {
        if (!isInline())
            setLayouted(false);
        parent()->updateSize();
        return;
    }

    if(m_minWidth > containingBlockWidth() || m_minWidth != oldMin ||
        m_maxWidth != oldMax || isReplaced())
    {
        setLayouted(false);
        if(containingBlock() != this) containingBlock()->updateSize();
    }
    else
        updateHeight();
}

void RenderBox::updateHeight()
{

#ifdef DEBUG_LAYOUT
    kdDebug( 6040 ) << renderName() << "(RenderBox) " << this << " ::updateHeight()" << endl;
#endif

    RenderObject* cb = containingBlock();
    if (parsing())
    {
        if (layouted())
        {
            setLayouted(false);
            if(cb != this) cb->updateHeight();
        } else
            root()->updateHeight();

        return;
    }

    if(!isInline() || isReplaced())
    {
        int oldHeight = m_height;
        setLayouted(false);

        if (hasOverhangingFloats())
            if(cb != this) cb->updateHeight();

        layout();

        if(m_height != oldHeight) {
            if(cb != this) cb->updateHeight();
        } else {
            cb->repaint();
        }
    }
}

void RenderBox::position(int x, int y, int, int, int, bool, bool)
{
    m_x = x + marginLeft();
    m_y = y;
    // ### paddings
    //m_width = width;
}

short RenderBox::verticalPositionHint() const
{
    switch(style()->verticalAlign())
    {
    case BASELINE:
        //kdDebug( 6040 ) << "aligned to baseline" << endl;
        return contentHeight();
    case SUB:
        // ###
    case SUPER:
        // ###
    case TOP:
        return PositionTop;
    case TEXT_TOP:
        return fontMetrics(style()->font()).ascent();
    case MIDDLE:
        return contentHeight()/2;
    case BOTTOM:
        return PositionBottom;
    case TEXT_BOTTOM:
        return fontMetrics(style()->font()).descent();
    }
    return 0;
}


short RenderBox::baselineOffset() const
{
    switch(style()->verticalAlign())
    {
    case BASELINE:
//      kdDebug( 6040 ) << "aligned to baseline" << endl;
        return m_height;
    case SUB:
        // ###
    case SUPER:
        // ###
    case TOP:
        return -1000;
    case TEXT_TOP:
        return fontMetrics(style()->font()).ascent();
    case MIDDLE:
        return -fontMetrics(style()->font()).width('x')/2;
    case BOTTOM:
        return 1000;
    case TEXT_BOTTOM:
        return fontMetrics(style()->font()).descent();
    }
    return 0;
}

int RenderBox::bidiHeight() const
{
    return contentHeight();
}


void RenderBox::repaint()
{
    //kdDebug( 6040 ) << "repaint!" << endl;
    int ow = style() ? style()->outlineWidth() : 0;
    repaintRectangle(-ow, -ow, m_width+ow*2, m_height+ow*2);
}

void RenderBox::repaintRectangle(int x, int y, int w, int h)
{
    x += m_x;
    y += m_y;
    // kdDebug( 6040 ) << "RenderBox(" << renderName() << ")::repaintRectangle (" << x << "/" << y << ") (" << w << "/" << h << ")" << endl;
    RenderObject *o = container();
    if( o ) o->repaintRectangle(x, y, w, h);
}

void RenderBox::relativePositionOffset(int &tx, int &ty)
{
    if(!style()->left().isVariable())
        tx += style()->left().width(containingBlockWidth());
    else if(!style()->right().isVariable())
        tx -= style()->right().width(containingBlockWidth());
    if(!style()->top().isVariable())
    {
        if (!style()->top().isPercent()
                || containingBlock()->style()->height().isFixed())
            ty += style()->top().width(containingBlockHeight());
    }
    else if(!style()->bottom().isVariable())
    {
        if (!style()->bottom().isPercent()
                || containingBlock()->style()->height().isFixed())
            ty -= style()->bottom().width(containingBlockHeight());
    }
}

void RenderBox::calcWidth()
{
#ifdef DEBUG_LAYOUT
    kdDebug( 6040 ) << "RenderBox("<<renderName()<<")::calcWidth()" << endl;
#endif
    if (isPositioned())
    {
        calcAbsoluteHorizontal();
    }
    else
    {
        Length w = style()->width();
        if (isReplaced())
        {
            if(w.isVariable())
            {
                Length h = style()->height();
                if(intrinsicHeight() > 0 && (h.isFixed() || h.isPercent()))
                {
                    int myh = h.width(containingBlockHeight());
                    int iw = (int)((((double)(myh))/((double) intrinsicHeight()))*((double)intrinsicWidth()));
                    w = Length(iw, Fixed);
                }
                else
                    w = Length(intrinsicWidth(), Fixed);
            }
            else if(!w.isFixed())
            {
                RenderObject* cb = containingBlock();
                if(cb->isBody())
                    while(cb && !cb->isRoot())
                        cb = cb->parent();

                if(cb)
                    w = Length(w.width(cb->contentWidth()), Fixed);
                else
                    w = Length(intrinsicWidth(), Fixed);
            }
        }

        Length ml = style()->marginLeft();
        Length mr = style()->marginRight();

        int cw = containingBlockWidth();

        if (cw<0) cw = 0;

        m_marginLeft = 0;
        m_marginRight = 0;

        if (isInline())
        {
            // just calculate margins
            m_marginLeft = ml.minWidth(cw);
            m_marginRight = mr.minWidth(cw);
            if (isReplaced())
            {
                m_width = w.width(cw);
                m_width += paddingLeft() + paddingRight() + borderLeft() + borderRight();

                if(m_width < m_minWidth) m_width = m_minWidth;
            }

            return;
        }
        else if (w.type == Variable)
        {
//          kdDebug( 6040 ) << "variable" << endl;
            m_marginLeft = ml.minWidth(cw);
            m_marginRight = mr.minWidth(cw);
            if (cw) m_width = cw - m_marginLeft - m_marginRight;

//          kdDebug( 6040 ) <<  m_width <<"," << cw <<"," <<
//              m_marginLeft <<"," <<  m_marginRight << endl;

            if(m_width < m_minWidth) m_width = m_minWidth;

            if (isFloating())
            {
                calcMinMaxWidth();
                if(m_width > m_maxWidth) m_width = m_maxWidth;
            }
        }
        else
        {
//          kdDebug( 6040 ) << "non-variable " << w.type << ","<< w.value << endl;
            m_width = w.width(cw);
            m_width += paddingLeft() + paddingRight() + borderLeft() + borderRight();

            if(m_width < m_minWidth) m_width = m_minWidth;

            calcHorizontalMargins(ml,mr,cw);
        }

        if (cw && cw != m_width + m_marginLeft + m_marginRight && !isFloating() && !isInline())
        {
            if (style()->direction()==LTR)
                m_marginRight = cw - m_width - m_marginLeft;
            else
                m_marginLeft = cw - m_width - m_marginRight;
        }
    }

#ifdef DEBUG_LAYOUT
    kdDebug( 6040 ) << "RenderBox::calcWidth(): m_width=" << m_width << " containingBlockWidth()=" << containingBlockWidth() << endl;
    kdDebug( 6040 ) << "m_marginLeft=" << m_marginLeft << " m_marginRight=" << m_marginRight << endl;
#endif
}

void RenderBox::calcHorizontalMargins(const Length& ml, const Length& mr, int cw)
{
    if (isFloating())
    {
        m_marginLeft = ml.minWidth(cw);
        m_marginRight = mr.minWidth(cw);
    }
    else
    {
        if ( (ml.type == Variable && mr.type == Variable) ||
             (ml.type != Variable &&
                containingBlock()->style()->textAlign() == KONQ_CENTER) )
        {
            m_marginRight = (cw - m_width)/2;
            m_marginLeft = cw - m_width - m_marginRight;
        }
        else if (mr.type == Variable)
        {
            m_marginLeft = ml.width(cw);
            m_marginRight = cw - m_width - m_marginLeft;
        }
        else if (ml.type == Variable)
        {
            m_marginRight = mr.width(cw);
            m_marginLeft = cw - m_width - m_marginRight;
        }
        else
        {
            m_marginLeft = ml.minWidth(cw);
            m_marginRight = mr.minWidth(cw);
        }
    }
}

void RenderBox::calcHeight()
{

#ifdef DEBUG_LAYOUT
    kdDebug( 6040 ) << "RenderBox::calcHeight()" << endl;
#endif

    //cell height is managed by table, inline elements do not have a height property.
    if ( isTableCell() || (isInline() && !isReplaced()) )
        return;

    if (isPositioned())
        calcAbsoluteVertical();
    else
    {
        Length h = style()->height();
        if (isReplaced())
        {
            if(h.isVariable())
            {
                Length w = style()->width();
                if(intrinsicWidth() > 0 && (w.isFixed() || w.isPercent()))
                {
                    int myw = w.width(containingBlockWidth());
                    int ih = (int) ((((double)(myw))/((double)intrinsicWidth()))*((double)intrinsicHeight()));
                    h = Length(ih, Fixed);
                }
                else
                    h = Length(intrinsicHeight(), Fixed);

            }
            else if(!h.isFixed())
            {
                RenderObject* cb = containingBlock();
                if(cb->isBody())
                {
                    while(cb && !cb->isRoot())
                        cb = cb->parent();

                    if(cb)
                        h = Length(h.width(static_cast<RenderRoot*>(cb)->view()->visibleHeight()), Fixed);
                    else
                        h = Length(intrinsicHeight(), Fixed);
                }
                else
                    h = Length(h.width(cb->contentHeight()), Fixed);
            }
        }

        Length tm = style()->marginTop();
        Length bm = style()->marginBottom();
        Length ch = containingBlock()->style()->height();

        // margins are calculated with respect to the _width_ of
        // the containing block (8.3)
        int cw = containingBlockWidth();

        m_marginTop = tm.minWidth(cw);
        m_marginBottom = bm.minWidth(cw);

        // for tables, calculate margins only
        if (isTable())
            return;

        if (h.isFixed())
            m_height = QMAX (h.value + borderTop() + paddingTop()
                + borderBottom() + paddingBottom() , m_height);
        else if (h.isPercent())
        {
            if (ch.isFixed())
                m_height = QMAX (h.width(ch.value) + borderTop() + paddingTop()
                    + borderBottom() + paddingBottom(), m_height);
        }
    }
}


void RenderBox::calcAbsoluteHorizontal()
{
    const int AUTO = -666666;
    int l,r,w,ml,mr,cw;

    int pab = borderLeft()+ borderRight()+ paddingLeft()+ paddingRight();

    l=r=ml=mr=w=AUTO;
    cw = containingBlockWidth()
        +containingBlock()->paddingLeft() +containingBlock()->paddingRight();

    if(!style()->left().isVariable())
        l = style()->left().width(cw);
    if(!style()->right().isVariable())
        r = style()->right().width(cw);
    if(!style()->width().isVariable())
        w = style()->width().width(cw);
    else if (isReplaced())
        w = intrinsicWidth();
    if(!style()->marginLeft().isVariable())
        ml = style()->marginLeft().width(cw);
    if(!style()->marginRight().isVariable())
        mr = style()->marginRight().width(cw);


//    printf("h1: w=%d, l=%d, r=%d, ml=%d, mr=%d\n",w,l,r,ml,mr);

    int static_distance=0;
    if ((style()->direction()==LTR && (l==AUTO && r==AUTO ))
            || style()->left().isStatic())
    {
        // calc hypothetical location in the normal flow
        // used for 1) left=static-position
        //          2) left, right, width are all auto -> calc top -> 3.
        //          3) precalc for case 2 below

        // all positioned elements are blocks, so that
        // would be at the left edge
        RenderObject* po = parent();
        while (po && po!=containingBlock()) {
            static_distance+=po->xPos();
            po=po->parent();
        }

        if (l==AUTO || style()->left().isStatic())
            l = static_distance;
    }

    else if ((style()->direction()==RTL && (l==AUTO && r==AUTO ))
            || style()->right().isStatic())
    {
            RenderObject* po = parent();

            static_distance = cw - po->width();

            while (po && po!=containingBlock()) {
                static_distance-=po->xPos();
                po=po->parent();
            }

        if (r==AUTO || style()->right().isStatic())
            r = static_distance;
    }


    if (l!=AUTO && w!=AUTO && r!=AUTO)
    {
        // left, width, right all given, play with margins
        int ot = l + w + r + pab;

        if (ml==AUTO && mr==AUTO)
        {
            // both margins auto, solve for equality
            ml = (cw - ot)/2;
            mr = cw - ot - ml;
        }
        else if (ml==AUTO)
            // solve for left margin
            ml = cw - ot - mr;
        else if (mr==AUTO)
            // solve for right margin
            mr = cw - ot - ml;
        else
        {
            // overconstrained, solve according to dir
            if (style()->direction()==LTR)
                r = cw - ( l + w + ml + mr + pab);
            else
                l = cw - ( r + w + ml + mr + pab);
        }
    }
    else
    {
        // one or two of (left, width, right) missing, solve

        // auto margins are ignored
        if (ml==AUTO) ml = 0;
        if (mr==AUTO) mr = 0;

        //1. solve left & width.
        if (l==AUTO && w==AUTO && r!=AUTO)
        {
            w = QMIN(m_maxWidth, cw - ( r + ml + mr + pab));
            l = cw - ( r + w + ml + mr + pab);
        }
        else

        //2. solve left & right. use static positioning.
        if (l==AUTO && w!=AUTO && r==AUTO)
        {
            if (style()->direction()==RTL)
            {
                r = static_distance;
                l = cw - ( r + w + ml + mr + pab);
            }
            else
            {
                l = static_distance;
                r = cw - ( l + w + ml + mr + pab);
            }

        }
        else

        //3. solve width & right.
        if (l!=AUTO && w==AUTO && r==AUTO)
        {
            w = QMIN(m_maxWidth, cw - ( l + ml + mr + pab));
            r = cw - ( l + w + ml + mr + pab);
        }
        else

        //4. solve left
        if (l==AUTO && w!=AUTO && r!=AUTO)
            l = cw - ( r + w + ml + mr + pab);
        else

        //5. solve width
        if (l!=AUTO && w==AUTO && r!=AUTO)
            w = cw - ( r + l + ml + mr + pab);
        else

        //6. solve right
        if (l!=AUTO && w!=AUTO && r==AUTO)
            r = cw - ( l + w + ml + mr + pab);
    }

    m_width = w + pab;
    m_marginLeft = ml;
    m_marginRight = mr;
    m_x = l + ml + containingBlock()->borderLeft();

//    printf("h: w=%d, l=%d, r=%d, ml=%d, mr=%d\n",w,l,r,ml,mr);
}


void RenderBox::calcAbsoluteVertical()
{
    // css2 spec 10.6.4 & 10.6.5

    // based on
    // http://www.w3.org/Style/css2-updates/REC-CSS2-19980512-errata
    // (actually updated 2000-10-24)
    // that introduces static-position value for top, left & right

    const int AUTO = -666666;
    int t,b,h,mt,mb,ch;

    t=b=h=mt=mb=AUTO;

    int pab = borderTop()+borderBottom()+paddingTop()+paddingBottom();

    Length hl = containingBlock()->style()->height();
    if (hl.isFixed())
        ch = hl.value + containingBlock()->paddingTop()
             + containingBlock()->paddingBottom();
    else
        ch = containingBlock()->height();

    if(!style()->top().isVariable())
        t = style()->top().width(ch);
    if(!style()->bottom().isVariable())
        b = style()->bottom().width(ch);
    if(!style()->height().isVariable())
    {
        h = style()->height().width(ch);
        if (m_height-pab>h)
            h=m_height-pab;
    }
    else if (isReplaced())
        h = intrinsicHeight();

    if(!style()->marginTop().isVariable())
        mt = style()->marginTop().width(ch);
    if(!style()->marginBottom().isVariable())
        mb = style()->marginBottom().width(ch);

    int static_top=0;
    if ((t==AUTO && b==AUTO ) || style()->top().isStatic())
    {
        // calc hypothetical location in the normal flow
        // used for 1) top=static-position
        //          2) top, bottom, height are all auto -> calc top -> 3.
        //          3) precalc for case 2 below

        RenderObject* ro = previousSibling();
        while ( ro && ro->isPositioned())
            ro = ro->previousSibling();

        if (ro) static_top = ro->yPos()+ro->marginBottom()+ro->height();

        RenderObject* po = parent();
        while (po && po!=containingBlock()) {
            static_top+=po->yPos();
            po=po->parent();
        }

        if (h==AUTO || style()->top().isStatic())
            t = static_top;
    }



    if (t!=AUTO && h!=AUTO && b!=AUTO)
    {
        // top, height, bottom all given, play with margins
        int ot = h + t + b + pab;

        if (mt==AUTO && mb==AUTO)
        {
            // both margins auto, solve for equality
            mt = (ch - ot)/2;
            mb = ch - ot - mt;
        }
        else if (mt==AUTO)
            // solve for top margin
            mt = ch - ot - mb;
        else if (mb==AUTO)
            // solve for bottom margin
            mb = ch - ot - mt;
        else
            // overconstrained, solve for bottom
            b = ch - ( h+t+mt+mb+pab);
    }
    else
    {
        // one or two of (top, height, bottom) missing, solve

        // auto margins are ignored
        if (mt==AUTO) mt = 0;
        if (mb==AUTO) mb = 0;

        //1. solve top & height. use content height.
        if (t==AUTO && h==AUTO && b!=AUTO)
        {
            h = m_height-pab;
            t = ch - ( h+b+mt+mb+pab);
        }
        else

        //2. solve top & bottom. use static positioning.
        if (t==AUTO && h!=AUTO && b==AUTO)
        {
            t = static_top;
            b = ch - ( h+t+mt+mb+pab);
        }
        else

        //3. solve height & bottom. use content height.
        if (t!=AUTO && h==AUTO && b==AUTO)
        {
            h = m_height-pab;
            b = ch - ( h+t+mt+mb+pab);
        }
        else

        //4. solve top
        if (t==AUTO && h!=AUTO && b!=AUTO)
            t = ch - ( h+b+mt+mb+pab);
        else

        //5. solve height
        if (t!=AUTO && h==AUTO && b!=AUTO)
            h = ch - ( t+b+mt+mb+pab);
        else

        //6. solve bottom
        if (t!=AUTO && h!=AUTO && b==AUTO)
            b = ch - ( h+t+mt+mb+pab);
    }


    if (m_height<h+pab) //content must still fit
        m_height = h+pab;

    m_marginTop = mt;
    m_marginBottom = mb;
    m_y = t + mt + containingBlock()->borderTop();

//    printf("v: h=%d, t=%d, b=%d, mt=%d, mb=%d, m_y=%d\n",h,t,b,mt,mb,m_y);

}


int RenderBox::lowestPosition() const
{
    return m_height + marginBottom();
}

int RenderBox::rightmostPosition() const
{
    return m_width + marginLeft();
}

#undef DEBUG_LAYOUT
