/**
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *	     (C) 2000 Simon Hausmann <hausmann@kde.org>
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

#ifndef HTML_BASEIMPL_H
#define HTML_BASEIMPL_H 1

#include "dtd.h"
#include "html_elementimpl.h"
#include "khtmllayout.h"
#include <qscrollview.h>

class KHTMLView;

namespace khtml {
    class RenderFrame;
    class RenderPartObject;
}

namespace DOM {

class DOMString;
    class CSSStyleSheetImpl;

class HTMLBodyElementImpl : public HTMLElementImpl
{
public:
    HTMLBodyElementImpl(DocumentImpl *doc);
    ~HTMLBodyElementImpl();

    virtual const DOMString nodeName() const;
    virtual ushort id() const;

    virtual tagStatus startTag() const { return BODYStartTag; }
    virtual tagStatus endTag() const { return BODYEndTag; }

    virtual void parseAttribute(AttrImpl *);
    void attach(KHTMLView *w);

    CSSStyleSheetImpl *sheet() const { return m_styleSheet; }

protected:
    CSSStyleSheetImpl *m_styleSheet;
    DOMString bgImage;
    DOMString bgColor;
};

// -------------------------------------------------------------------------

class HTMLFrameElementImpl : public HTMLElementImpl
{
    friend class khtml::RenderFrame;
    friend class khtml::RenderPartObject;
public:
    HTMLFrameElementImpl(DocumentImpl *doc);

    ~HTMLFrameElementImpl();

    virtual const DOMString nodeName() const;
    virtual ushort id() const;

    virtual tagStatus startTag() const { return FRAMEStartTag; }
    virtual tagStatus endTag() const { return FRAMEEndTag; }

    virtual void parseAttribute(AttrImpl *);
    virtual void attach(KHTMLView *w);
    virtual void detach();

    bool noResize() { return noresize; }

    virtual bool isSelectable() const;
    virtual void setFocus(bool);

protected:
    DOMString url;
    DOMString name;
    KHTMLView *view;
    KHTMLView *parentWidget;

    int marginWidth;
    int marginHeight;
    bool frameBorder : 1;
    bool frameBorderSet : 1;
    bool noresize : 1;
    QScrollView::ScrollBarMode scrolling;
};

// -------------------------------------------------------------------------

class HTMLFrameSetElementImpl : public HTMLElementImpl
{
public:
    HTMLFrameSetElementImpl(DocumentImpl *doc);

    ~HTMLFrameSetElementImpl();

    virtual const DOMString nodeName() const;
    virtual ushort id() const;

    virtual tagStatus startTag() const { return FRAMESETStartTag; }
    virtual tagStatus endTag() const { return FRAMESETEndTag; }

    virtual void parseAttribute(AttrImpl *);
    virtual void attach(KHTMLView *w);

    virtual bool mouseEvent( int _x, int _y,
                             int _tx, int _ty,
                             MouseEvent *ev );

    virtual khtml::FindSelectionResult findSelectionNode( int _x, int _y, int _tx, int _ty,
                                                   DOM::Node & node, int & offset );

    bool frameBorder() { return frameborder; }
    bool noResize() { return noresize; }

    int totalRows() const { return m_totalRows; }
    int totalCols() const { return m_totalCols; }
    int border() const { return m_border; }

protected:
    QList<khtml::Length> *m_rows;
    QList<khtml::Length> *m_cols;

    int m_totalRows;
    int m_totalCols;

    // mozilla and others use this in the frameset, although it's not standard html4
    int m_border;
    bool frameborder : 1;
    bool frameBorderSet : 1;
    bool noresize : 1;
    bool m_resizing : 1;  // is the user resizing currently

    KHTMLView *view;
};

// -------------------------------------------------------------------------

class HTMLHeadElementImpl : public HTMLElementImpl
{
public:
    HTMLHeadElementImpl(DocumentImpl *doc);

    ~HTMLHeadElementImpl();

    virtual const DOMString nodeName() const;
    virtual ushort id() const;

    virtual tagStatus startTag() const { return HEADStartTag; }
    virtual tagStatus endTag() const { return HEADEndTag; }
};

// -------------------------------------------------------------------------

class HTMLHtmlElementImpl : public HTMLElementImpl
{
public:
    HTMLHtmlElementImpl(DocumentImpl *doc);

    ~HTMLHtmlElementImpl();

    virtual const DOMString nodeName() const;
    virtual ushort id() const;

    virtual tagStatus startTag() const { return HTMLStartTag; }
    virtual tagStatus endTag() const { return HTMLEndTag; }

    virtual void attach(KHTMLView *w);

};


// -------------------------------------------------------------------------

class HTMLIFrameElementImpl : public HTMLFrameElementImpl
{
public:
    HTMLIFrameElementImpl(DocumentImpl *doc);

    ~HTMLIFrameElementImpl();

    virtual const DOMString nodeName() const;
    virtual ushort id() const;

    virtual tagStatus startTag() const { return IFRAMEStartTag; }
    virtual tagStatus endTag() const { return IFRAMEEndTag; }

    virtual void parseAttribute(AttrImpl *attr);
    virtual void attach(KHTMLView *w);
    virtual void applyChanges(bool = true, bool = true);
protected:
    bool needWidgetUpdate;
};


}; //namespace

#endif

