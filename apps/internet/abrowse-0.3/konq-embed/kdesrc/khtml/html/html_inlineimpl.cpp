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
// -------------------------------------------------------------------------
#include "html_inlineimpl.h"

#include "html_documentimpl.h"
using namespace DOM;

#include <kdebug.h>

#include "htmlhashes.h"
#include "css/csshelper.h"
#include "css/cssproperties.h"
#include "css/cssstyleselector.h"

#include "rendering/render_br.h"

using namespace khtml;

HTMLAnchorElementImpl::HTMLAnchorElementImpl(DocumentImpl *doc)
    : HTMLElementImpl(doc)
{
    href = 0;
    target = 0;
}

HTMLAnchorElementImpl::~HTMLAnchorElementImpl()
{
    if( href ) href->deref();
    if( target ) target->deref();
}

const DOMString HTMLAnchorElementImpl::nodeName() const
{
    return "A";
}

ushort HTMLAnchorElementImpl::id() const
{
    return ID_A;
}

bool HTMLAnchorElementImpl::mouseEvent( int _x, int _y,
                                        int _tx, int _ty,
                                        MouseEvent *ev)
{
    bool inside = HTMLElementImpl::mouseEvent( _x, _y, _tx, _ty, ev);

    if ( inside && ev->url==0 && !ev->noHref
         && (!(m_render && m_render->style() && m_render->style()->visiblity() == HIDDEN)) )
    {
	//kdDebug() << "HTMLAnchorElementImpl::mouseEvent" << _tx << "/" << _ty <<endl;
	// set the url
	if(target && href)
	    ev->url = DOMString("target://") + DOMString(target) + DOMString("/#") + DOMString(href);
	else
	    ev->url = href;
    }

    return inside;
}

void HTMLAnchorElementImpl::parseAttribute(AttrImpl *attr)
{
    switch(attr->attrId)
    {
    case ATTR_HREF:
    {
        DOMString s = khtml::parseURL(attr->val());
        href = s.implementation();
        if(href) href->ref();
        break;
    }
    case ATTR_TARGET:
    	target = attr->val();
        target->ref();
	break;
    default:
	HTMLElementImpl::parseAttribute(attr);
    }
}

// -------------------------------------------------------------------------

HTMLBRElementImpl::HTMLBRElementImpl(DocumentImpl *doc) : HTMLElementImpl(doc)
{
}

HTMLBRElementImpl::~HTMLBRElementImpl()
{
}

const DOMString HTMLBRElementImpl::nodeName() const
{
    return "BR";
}

ushort HTMLBRElementImpl::id() const
{
    return ID_BR;
}

void HTMLBRElementImpl::parseAttribute(AttrImpl *attr)
{
    switch(attr->attrId)
    {
    case ATTR_CLEAR:
    {
	DOMString str = attr->value();
	if( strcasecmp (str,"all")==0 || str.isEmpty() ) str = "both";
	addCSSProperty(CSS_PROP_CLEAR, str);
	break;
    }
    default:
    	HTMLElementImpl::parseAttribute(attr);
    }
}

void HTMLBRElementImpl::attach(KHTMLView *w)
{
    //kdDebug( 6030 ) << "HTMLBRElementImpl::attach" << endl;
    setStyle(document->styleSelector()->styleForElement(this));
    khtml::RenderObject *r = _parent->renderer();
    if(r)
    {
	m_render = new RenderBR();
	m_render->setStyle(m_style);
	r->addChild(m_render, _next ? _next->renderer() : 0);
    }
    NodeBaseImpl::attach( w );
}

// -------------------------------------------------------------------------

HTMLFontElementImpl::HTMLFontElementImpl(DocumentImpl *doc)
    : HTMLElementImpl(doc)
{
}

HTMLFontElementImpl::~HTMLFontElementImpl()
{
}

const DOMString HTMLFontElementImpl::nodeName() const
{
    return "FONT";
}

ushort HTMLFontElementImpl::id() const
{
    return ID_FONT;
}

void HTMLFontElementImpl::parseAttribute(AttrImpl *attr)
{
    switch(attr->attrId)
    {
    case ATTR_SIZE:
    {
	DOMString s = attr->value();
	if(s != 0) {
	    int num = s.toInt();
	    if ( *s.unicode() == '+' || *s.unicode() == '-' ) {
		num += 3;
	    }
	    DOMString size;
	    switch (num)
	    {
		// size = 3 is the normal size according to html specs
		case 1:
		    size = "x-small";
		    break;
		case 2:
		    size = "small";
		    break;
		case 3:
		    size = "medium";
		    break;
		case 4:
		    size = "large";
		    break;
		case 5:
		    size = "x-large";
		    break;
		default:
		    if (num >= 6)
			size = "xx-large";
		    else if (num < 1)
			size = "xx-small";

		    break;
	    }
	    if( !size.isNull() )
		addCSSProperty(CSS_PROP_FONT_SIZE, size);
	}
	break;
    }
    case ATTR_COLOR:
	addCSSProperty(CSS_PROP_COLOR, attr->value());
	// HTML4 compatibility hack
	addCSSProperty(CSS_PROP_TEXT_DECORATION_COLOR, attr->value());
	break;
    case ATTR_FACE:
	addCSSProperty(CSS_PROP_FONT_FAMILY, attr->value());
	break;
    default:
	HTMLElementImpl::parseAttribute(attr);
    }
}


void HTMLFontElementImpl::attach(KHTMLView *w)
{
    HTMLElementImpl::attach(w);
#if 0
    // the font element needs special handling because it has to behave like
    // an inline or block level element depending on context.

    setStyle(document->styleSelector()->styleForElement(this));
    if(_parent && _parent->renderer())
    {
	if(_parent->style()->display() != INLINE)
	    m_style->setDisplay(BLOCK);
	m_render = khtml::RenderObject::createObject(this);

	if(m_render)
	{
	    _parent->renderer()->addChild(m_render, _next ? _next->renderer() : 0);
	}
    }

    NodeBaseImpl::attach(w);
#endif
}


// -------------------------------------------------------------------------

HTMLModElementImpl::HTMLModElementImpl(DocumentImpl *doc, ushort tagid) : HTMLElementImpl(doc)
{
    _id = tagid;
}

HTMLModElementImpl::~HTMLModElementImpl()
{
}

const DOMString HTMLModElementImpl::nodeName() const
{
    return getTagName(_id);
}

ushort HTMLModElementImpl::id() const
{
    return _id;
}

// -------------------------------------------------------------------------

HTMLQuoteElementImpl::HTMLQuoteElementImpl(DocumentImpl *doc)
    : HTMLElementImpl(doc)
{
}

HTMLQuoteElementImpl::~HTMLQuoteElementImpl()
{
}

const DOMString HTMLQuoteElementImpl::nodeName() const
{
    return "Q";
}

ushort HTMLQuoteElementImpl::id() const
{
    return ID_Q;
}

