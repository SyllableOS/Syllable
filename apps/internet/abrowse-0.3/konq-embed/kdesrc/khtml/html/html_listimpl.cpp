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
#include "html_listimpl.h"

using namespace DOM;

#include "css/cssproperties.h"
#include "rendering/render_list.h"
#include "misc/htmlhashes.h"
#include "xml/dom_docimpl.h"

using namespace khtml;

const DOMString HTMLUListElementImpl::nodeName() const
{
    return "UL";
}

ushort HTMLUListElementImpl::id() const
{
    return ID_UL;
}

void HTMLUListElementImpl::parseAttribute(AttrImpl *attr)
{
    switch(attr->attrId)
    {
    case ATTR_TYPE:
        addCSSProperty(CSS_PROP_LIST_STYLE_TYPE, attr->value());
        break;
    default:
        HTMLElementImpl::parseAttribute(attr);
    }
}

void HTMLUListElementImpl::attach(KHTMLView *w)
{
    HTMLElementImpl::attach(w);
    if(document->parseMode() != DocumentImpl::Strict )
	style()->setFlowAroundFloats(true);
}

// -------------------------------------------------------------------------

const DOMString HTMLDirectoryElementImpl::nodeName() const
{
    return "DIR";
}

ushort HTMLDirectoryElementImpl::id() const
{
    return ID_DIR;
}

void HTMLDirectoryElementImpl::attach(KHTMLView *w)
{
    HTMLElementImpl::attach(w);
    if(document->parseMode() != DocumentImpl::Strict )
	style()->setFlowAroundFloats(true);
}

// -------------------------------------------------------------------------

const DOMString HTMLMenuElementImpl::nodeName() const
{
    return "MENU";
}

ushort HTMLMenuElementImpl::id() const
{
    return ID_MENU;
}

void HTMLMenuElementImpl::attach(KHTMLView *w)
{
    HTMLElementImpl::attach(w);
    if(document->parseMode() != DocumentImpl::Strict )
	style()->setFlowAroundFloats(true);
}

// -------------------------------------------------------------------------

const DOMString HTMLOListElementImpl::nodeName() const
{
    return "OL";
}

ushort HTMLOListElementImpl::id() const
{
    return ID_OL;
}

void HTMLOListElementImpl::parseAttribute(AttrImpl *attr)
{
    switch(attr->attrId)
    {
    case ATTR_TYPE:
        if ( strcmp( attr->value(), "a" ) == 0 )
            addCSSProperty(CSS_PROP_LIST_STYLE_TYPE, "lower-alpha");
        else if ( strcmp( attr->value(), "A" ) == 0 )
            addCSSProperty(CSS_PROP_LIST_STYLE_TYPE, "upper-alpha");
        else if ( strcmp( attr->value(), "i" ) == 0 )
            addCSSProperty(CSS_PROP_LIST_STYLE_TYPE, "lower-roman");
        else if ( strcmp( attr->value(), "I" ) == 0 )
            addCSSProperty(CSS_PROP_LIST_STYLE_TYPE, "upper-roman");
        else if ( strcmp( attr->value(), "1" ) == 0 )
            addCSSProperty(CSS_PROP_LIST_STYLE_TYPE, "decimal");
        break;
    case ATTR_START:
            _start = attr->val() ? attr->val()->toInt() : 1;
    default:
        HTMLUListElementImpl::parseAttribute(attr);
    }
}

void HTMLOListElementImpl::attach(KHTMLView *w)
{
    HTMLElementImpl::attach(w);
    if(document->parseMode() != DocumentImpl::Strict )
	style()->setFlowAroundFloats(true);
}

// -------------------------------------------------------------------------

const DOMString HTMLLIElementImpl::nodeName() const
{
    return "LI";
}

ushort HTMLLIElementImpl::id() const
{
    return ID_LI;
}


void HTMLLIElementImpl::parseAttribute(AttrImpl *attr)
{
    switch(attr->attrId)
    {
    case ATTR_VALUE:
        if(m_render && m_render->isListItem())
        {
            RenderListItem *list = static_cast<RenderListItem *>(m_render);
            // ### work out what to do when attribute removed - use default of some sort?
            long v = attr->val() ? attr->val()->toInt() : 0;
            list->setValue(v);
        }
        break;
    case ATTR_TYPE:
        if ( strcmp( attr->value(), "a" ) == 0 )
            addCSSProperty(CSS_PROP_LIST_STYLE_TYPE, "lower-alpha");
        else if ( strcmp( attr->value(), "A" ) == 0 )
            addCSSProperty(CSS_PROP_LIST_STYLE_TYPE, "upper-alpha");
        else if ( strcmp( attr->value(), "i" ) == 0 )
            addCSSProperty(CSS_PROP_LIST_STYLE_TYPE, "lower-roman");
        else if ( strcmp( attr->value(), "I" ) == 0 )
            addCSSProperty(CSS_PROP_LIST_STYLE_TYPE, "upper-roman");
        else if ( strcmp( attr->value(), "1" ) == 0 )
            addCSSProperty(CSS_PROP_LIST_STYLE_TYPE, "decimal");
        else
            addCSSProperty(CSS_PROP_LIST_STYLE_TYPE, attr->value());
        break;
    default:
        HTMLElementImpl::parseAttribute(attr);
    }
}

// -------------------------------------------------------------------------


const DOMString HTMLDListElementImpl::nodeName() const
{
    return "DL";
}

ushort HTMLDListElementImpl::id() const
{
    return ID_DL;
}

