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
#include "dom_elementimpl.h"

#include "dom_exception.h"
#include "dom_node.h"
#include "dom_textimpl.h"
#include "dom_docimpl.h"

#include "css/cssstyleselector.h"
#include "rendering/render_object.h"
#include "misc/htmlhashes.h"

#include "kdebug.h"
#include "css_valueimpl.h"

using namespace DOM;
using namespace khtml;

/*
 * NOTE:
 * According to the DOM docs, an Attr stores the value directly in it's parsed
 * form, but for values containing entities, creates a subtree with nodes
 * containing the unexpanded form (for XML). On read, the value is always
 * returned entity-free, so we decided for HTML we could only store a
 * parsed DOMString and have no child-nodes.
 */

AttrImpl::AttrImpl()
    : NodeImpl(0),
      _name( 0 ),
      _value( 0 ),
      _element(0),
      attrId(0)
{
}


AttrImpl::AttrImpl(DocumentImpl *doc, const DOMString &name)
    : NodeImpl(doc),
      _name( 0 ),
      _value( 0 ),
      _element(0),
      attrId(0)
{
    setName(name);
}

AttrImpl::AttrImpl(DocumentImpl *doc, int id)
    : NodeImpl(doc),
      _name( 0 ),
      _value( 0 ),
      _element(0),
      attrId(id)
{
}

AttrImpl::AttrImpl(const AttrImpl &other) : NodeImpl(other.ownerDocument())
{
    m_specified = other.specified();
    _element = other._element;
    _name = other._name;
    if (_name) _name->ref();
    _value = other._value;
    if (_value) _value->ref();
    attrId = other.attrId;
}

AttrImpl &AttrImpl::operator = (const AttrImpl &other)
{
    m_specified = other.specified();
    document = other.ownerDocument();
    _element = other._element;

    if (_name) _name->deref();
    _name = other._name;
    if (_name) _name->ref();
    if (_value) _value->deref();
    _value = other._value;
    if (_value) _value->ref();
    attrId = other.attrId;

    return *this;
}

AttrImpl::~AttrImpl()
{
    if(_name) _name->deref();
    if(_value) _value->deref();
}

const DOMString AttrImpl::nodeName() const
{
    return name();
}

unsigned short AttrImpl::nodeType() const
{
    return Node::ATTRIBUTE_NODE;
}


DOMString AttrImpl::name() const
{
    if(attrId)
	return getAttrName(attrId);
    else if (_name)
	return _name;
    else
	return DOMString();
}

void AttrImpl::setName(const DOMString &n)
{
    if(_name) _name->deref();
    _name = n.implementation();
    attrId = khtml::getAttrID(QConstString(_name->s, _name->l).string().lower().ascii(), _name->l);
    if (attrId)
	_name = 0;
    else
	_name->ref();
}

DOMString AttrImpl::value() const {
    return _value;
}

void AttrImpl::setValue( const DOMString &v )
{
    // according to the DOM docs, we should create an unparsed Text child
    // node here; we decided this was not necessary for HTML

    // ### TODO: parse value string, interprete entities (not sure if we are supposed to do this)

    if (_element)
	_element->checkReadOnly();

    if (_value) _value->deref();
    _value = v.implementation();
    if (_value) _value->ref();
    m_specified = true;

    if (_element) {
	_element->parseAttribute(this);
	_element->setChanged(true);
    }
}

void AttrImpl::setNodeValue( const DOMString &v, int &exceptioncode )
{
    exceptioncode = 0;
    setValue(v);
}

#if 0
// ### is this still needed?
AttrImpl::AttrImpl(const DOMString &name, const DOMString &value,
		   DocumentImpl *doc, bool specified) : NodeImpl(doc)
{
    _element = 0;
    m_specified = specified;

    attrId = 0;
    _name = 0;
    setName(name);
    _value = value.implementation();
    if (_value) _value->ref();

}
#endif

AttrImpl::AttrImpl(const khtml::Attribute *attr, DocumentImpl *doc, ElementImpl *element) : NodeImpl(doc)
{
    _name = attr->n;
    if (_name) _name->ref();
    _value = attr->v;
    if (_value) _value->ref();
    attrId = attr->id;
    _element = element;
    m_specified = 1;

}

AttrImpl::AttrImpl(const DOMString &name, const DOMString &value, DocumentImpl *doc) : NodeImpl(doc)
{
    attrId = 0;
    _name = 0;
    setName(name);
    _value = value.implementation();
    if (_value) _value->ref();
    _element = 0;
    m_specified = 1;
}

AttrImpl::AttrImpl(int _id, const DOMString &value, DocumentImpl *doc) : NodeImpl(doc)
{
  attrId = _id;
  _name = 0;
  _value = value.implementation();
  if (_value) _value->ref();
  _element = 0;
  m_specified = false;
}

NodeImpl *AttrImpl::parentNode() const
{
    return 0;
}

NodeImpl *AttrImpl::previousSibling() const
{
    return 0;
}

NodeImpl *AttrImpl::nextSibling() const
{
    return 0;
}

NodeImpl *AttrImpl::cloneNode ( bool /*deep*/, int &/*exceptioncode*/ )
{
    AttrImpl *newImpl = new AttrImpl(*this);
    newImpl->_element = 0; // can't have two attributes with the same name/id attached to an element
    return newImpl;
}

bool AttrImpl::deleteMe()
{
    if(!_element && _ref <= 0) return true;
    return false;
}

// DOM Section 1.1.1
bool AttrImpl::childAllowed( NodeImpl *newChild )
{
    switch (newChild->nodeType()) {
	case Node::TEXT_NODE:
	case Node::ENTITY_REFERENCE_NODE:
	    return true;
	    break;
	default:
	    return false;
    }
}


// -------------------------------------------------------------------------

ElementImpl::ElementImpl(DocumentImpl *doc) : NodeBaseImpl(doc)
{
    namedAttrMap = new NamedAttrMapImpl(this);
    namedAttrMap->ref();

    has_tabindex=false;
    tabindex=0;
    m_styleDecls = 0;
}

ElementImpl::~ElementImpl()
{
    if (m_render)
        detach();

    namedAttrMap->detachFromElement();
    namedAttrMap->deref();

    if (m_styleDecls) {
	m_styleDecls->setNode(0);
        m_styleDecls->setParent(0);
	m_styleDecls->deref();
    }
}

bool ElementImpl::isInline() const
{
    if(!m_style) return false;
    return (m_style->display() == khtml::INLINE);
}

unsigned short ElementImpl::nodeType() const
{
    return Node::ELEMENT_NODE;
}

DOMString ElementImpl::tagName() const
{
    return nodeName();
}

DOMString ElementImpl::getAttribute( const DOMString &name ) const
{
  // search in already set attributes first
    int exceptioncode; // ### propogate
    AttrImpl *attr = static_cast<AttrImpl*>(namedAttrMap->getNamedItem(name,exceptioncode));
    if (attr) return attr->value();

    if(!defaultMap()) return 0;
    // then search in default attr in case it is not yet set
    int index = defaultMap()->find(name);
    if (index != -1) return defaultMap()->value(index);

    return 0;
}

DOMString ElementImpl::getAttribute( int id )
{
    // search in already set attributes first
    AttrImpl *attr = static_cast<AttrImpl*>(namedAttrMap->getIdItem(id));
    if (attr) return attr->value();

    if(!defaultMap()) return 0;
    // then search in default attr in case it is not yet set
    int index = defaultMap()->find(id);
    if (index != -1) return defaultMap()->value(index);

    return 0;
}

AttrImpl *ElementImpl::getAttributeNode ( int index ) const
{
    return namedAttrMap->getIdItem(index);
}

int ElementImpl::getAttributeCount() const
{
    int exceptioncode; // ### propogate
    return namedAttrMap->length(exceptioncode);
}

void ElementImpl::setAttribute( const DOMString &name, const DOMString &value)
{
    // ### check for invalid characters in value -> throw exception
    int exceptioncode; // ### propogate
    AttrImpl *oldAttr = 0;
    if (value.isNull())
	oldAttr = static_cast<AttrImpl*>(namedAttrMap->removeNamedItem(name,exceptioncode));
    else {
	AttrImpl *a = static_cast<AttrImpl*>(namedAttrMap->getNamedItem(name));
	if (a)
	    a->setValue(value);
	else
	    oldAttr = static_cast<AttrImpl*>(namedAttrMap->setNamedItem(new AttrImpl(name,value,document), exceptioncode));
    }
    if (oldAttr && oldAttr->deleteMe()) // ### deref instead?
	delete oldAttr;
}

void ElementImpl::setAttribute( int id, const DOMString &value )
{
    AttrImpl *oldAttr;
    if (value.isNull())
	oldAttr = namedAttrMap->removeIdItem(id);
    else {
	int exceptioncode;
	oldAttr = namedAttrMap->setIdItem(new AttrImpl(id,value,document), exceptioncode );
    }
    if (oldAttr && oldAttr->deleteMe())
	delete oldAttr;
}

void ElementImpl::setAttribute( const AttributeList &list )
{
    *namedAttrMap = list;
}

void ElementImpl::removeAttribute( const DOMString &name )
{
    int exceptioncode; // ### propogate
    AttrImpl *oldAttr = static_cast<AttrImpl*>(namedAttrMap->removeNamedItem(name,exceptioncode));
    if (oldAttr && oldAttr->deleteMe())
	delete oldAttr;
}

NodeImpl *ElementImpl::cloneNode ( bool deep, int &exceptioncode )
{
    ElementImpl *newImpl = document->createElement(tagName());
    if (!newImpl)
      return 0;

    // clone attributes
    *(newImpl->namedAttrMap) = *namedAttrMap;

    if (deep)
	cloneChildNodes(newImpl,exceptioncode);
    return newImpl;
}

NamedNodeMapImpl *ElementImpl::attributes() const
{
    return namedAttrMap;
}

AttrImpl *ElementImpl::getAttributeNode( const DOMString &name )
{
    int exceptioncode; // ### propogate
    // ### do we return attribute node if it is in the default map but not specified?
    return static_cast<AttrImpl*>(namedAttrMap->getNamedItem(name,exceptioncode));

}

AttrImpl *ElementImpl::setAttributeNode( AttrImpl *newAttr, int &exceptioncode )
{
    exceptioncode = 0;
    if (!newAttr) {
	exceptioncode = DOMException::NOT_FOUND_ERR;
	return 0;
    }
    if (newAttr->attrId)
	return static_cast<AttrImpl*>(namedAttrMap->setIdItem(newAttr, exceptioncode));
    else
	return static_cast<AttrImpl*>(namedAttrMap->setNamedItem(newAttr, exceptioncode));
}

AttrImpl *ElementImpl::removeAttributeNode( AttrImpl *oldAttr, int &exceptioncode )
{
    // ### should we replace with default in map? currently default attrs don't exist in map
    exceptioncode = 0;
    return namedAttrMap->removeAttr(oldAttr, exceptioncode);
}

NodeListImpl *ElementImpl::getElementsByTagName( const DOMString &name )
{
    return new TagNodeListImpl( this, name );
}

NodeListImpl *ElementImpl::getElementsByNameAttr( const DOMString &name )
{
    return new NameNodeListImpl( this, name);
}

short ElementImpl::tabIndex() const
{
  if (has_tabindex)
    return tabindex;
  else
    return -1;
}

void ElementImpl::setTabIndex( short _tabindex )
{
  has_tabindex=true;
  tabindex=_tabindex;
}

void ElementImpl::normalize( int &exceptioncode )
{
    // In DOM level 2, this gets moved to Node
    // ### normalize attributes? (when we store attributes using child nodes)
    exceptioncode = 0;
    NodeImpl *child = _first;
    while (child) {
	NodeImpl *nextChild = child->nextSibling();
	if (child->nodeType() == Node::ELEMENT_NODE) {
	    static_cast<ElementImpl*>(child)->normalize(exceptioncode);
	    if (exceptioncode)
		return;
	    child = nextChild;
	}
	else if (nextChild && child->nodeType() == Node::TEXT_NODE && nextChild->nodeType() == Node::TEXT_NODE) {
	    static_cast<TextImpl*>(child)->appendData(static_cast<TextImpl*>(nextChild)->data());
	    removeChild(nextChild,exceptioncode);
	    if (exceptioncode)
		return;
	}
	else
	    child = nextChild;
    }
}

AttributeList *ElementImpl::defaultMap() const
{
    return 0;
}

//#define SPEED_DEBUG

void ElementImpl::attach(KHTMLView *w)
{
    setStyle(document->styleSelector()->styleForElement(this));
#ifndef SPEED_DEBUG
    if(_parent && _parent->renderer())
    {
	m_render = khtml::RenderObject::createObject(this);
	if(m_render)
	{
	    _parent->renderer()->addChild(m_render, _next ? _next->renderer() : 0);
	}
    }
#endif
    NodeBaseImpl::attach(w);
}

void ElementImpl::detach()
{
    NodeBaseImpl::detach();
    delete m_render;
    m_render = 0;
}

void ElementImpl::applyChanges(bool top, bool force)
{
    // ### find a better way to handle non-css attributes
    setChanged(false);

    int ow = (m_style?m_style->outlineWidth():0);

    if (top)
	recalcStyle();   

    // a style change can influence the children, so we just go
    // through them and trigger an appplyChanges there too
    NodeImpl *n = _first;
    while(n) {
	n->applyChanges(false,force || changed());
	n = n->nextSibling();
    }

    if (!m_render)
	return;

    // calc min and max widths starting from leafs
    // might belong to renderer, but this is simple to do here
    if (force || changed())
	m_render->calcMinMaxWidth();

    if(top) {
	if (force)
	{
	    // force a relayout of this part of the document
	    m_render->updateSize();
	    // force a repaint of this part.
	    // ### if updateSize() changes any size, it will already force a
	    // repaint, so we might do double work here...
	    m_render->repaint();
	}
	else
	{
	    if (m_style)
		ow = QMAX(ow, m_style->outlineWidth());
	    RenderObject *cb = m_render->containingBlock();
	    if (cb && cb!=m_render)
		cb->repaintRectangle(-ow, -ow, cb->width()+2*ow, cb->height()+2*ow);
	    else
		m_render->repaint();
	}
    }
    setChanged(false);
}

void ElementImpl::recalcStyle()
{
    if(!m_style) return;
    bool faf = m_style->flowAroundFloats();
    EDisplay oldDisplay = m_style->display();

    int dynamicState = StyleSelector::None;
    if ( m_mouseInside )
	dynamicState |= StyleSelector::Hover;
    if ( m_focused )
	dynamicState |= StyleSelector::Focus;
    if ( m_active )
	dynamicState |= StyleSelector::Active;

    setStyle( document->styleSelector()->styleForElement(this, dynamicState) );

    if (oldDisplay != m_style->display()) {
        if (m_style->display()==NONE)
        {
            // ###FIXME hack
            // removing things from the rendering tree
            // is not yet reliable and leads to crashes.
            // disallowed changing to display:none after 
            // the initial construction           
            // fixes #19800
            
            // other option would be to reconstruct the entire
            // tree, but thats not realiable yet either
            m_style->setDisplay(oldDisplay);
        }
        else
        {
	    detach();
	    attach(document->view());
        }
    }

    m_style->setFlowAroundFloats(faf);
    if( m_render && m_style )
	m_render->setStyle(m_style);
    NodeImpl *n;
    for (n = _first; n; n = n->nextSibling())
	n->recalcStyle();
}

void ElementImpl::setOwnerDocument(DocumentImpl *_document)
{
    int exceptioncode; // ### propogate
    // also set for all our attributes
    uint i;
    for (i = 0; i < namedAttrMap->length(exceptioncode); i++)
	namedAttrMap->item(i,exceptioncode)->setOwnerDocument(_document);
    NodeBaseImpl::setOwnerDocument(_document);
}

bool ElementImpl::mouseEvent( int _x, int _y,
                              int _tx, int _ty,
                              MouseEvent *ev)
{
#ifdef EVENT_DEBUG
    kdDebug( 6030 ) << nodeName().string() << "::mouseEvent" << endl;
#endif
    bool inside = false;

    if(!m_render) return false;

    int origTx = _tx;
    int origTy = _ty;

    RenderObject *p = m_render->parent();
    while( p && p->isAnonymousBox() ) {
// 	kdDebug( 6030 ) << "parent is anonymous!" << endl;
	// we need to add the offset of the anonymous box
	_tx += p->xPos();
	_ty += p->yPos();
	p = p->parent();
    }

    bool positioned = m_render->style()
                      && (m_render->style()->position() == ABSOLUTE || m_render->style()->position() == FIXED);
    int oldZIndex = ev->currentZIndex;

    // Positioned element -> store current zIndex, for children to use
    if ( positioned )
    {
        ev->currentZIndex = m_render->style()->zIndex();
        //kdDebug() << "ElementImpl::mouseEvent storing currentZIndex=" << ev->currentZIndex << endl;
    }

    if(!m_render->isInline() || !m_render->firstChild() || m_render->isFloating() )
    {
        bool known = m_render->absolutePosition(_tx, _ty);

	if ( known && (_y >= _ty) && (_y < _ty + m_render->height())
                   && (_x >= _tx) && (_x < _tx + m_render->width()) )
        {
            if  ( ! (m_render->style() && m_render->style()->visiblity() == HIDDEN) )
            {
                //if ( positioned )
                //    kdDebug(6030) << " currentZIndex=" << ev->currentZIndex << " ev->zIndex=" << ev->zIndex << endl;
                if ( ! positioned || ev->currentZIndex > ev->zIndex )
                {
                    //kdDebug(6030) << nodeName().string() << " SETTING innerNode " << endl;
                    ev->innerNode = Node(this);
                    ev->nodeAbsX = origTx;
                    ev->nodeAbsY = origTy;
                    ev->zIndex = ev->currentZIndex;
                    inside = true;
                }
            }
        }
    }

    NodeImpl *child = firstChild();
    while(child != 0) {
	if(child->mouseEvent(_x, _y, _tx, _ty, ev))
	    inside = true;
	child = child->nextSibling();
    }

#ifdef EVENT_DEBUG
    if(inside) kdDebug( 6030 ) << nodeName().string() << "    --> inside" << endl;
#endif

    if(inside || mouseInside())
        if  ( ! (m_render->style() && m_render->style()->visiblity() == HIDDEN) )
        {
            // dynamic HTML...
            mouseEventHandler(ev, inside);
        }

    bool oldinside=mouseInside();

    setMouseInside(inside);

    if ( oldinside != inside && m_style->hasHover() )
        applyChanges(true,false);

    // reset previous z index
    if ( positioned )
        ev->currentZIndex = oldZIndex;

    return inside;
}

void ElementImpl::setFocus(bool received)
{
    NodeBaseImpl::setFocus(received);
    applyChanges(true,false);
}

void ElementImpl::setActive(bool down)
{
    NodeBaseImpl::setActive(down);
    applyChanges(true,false);
}

khtml::FindSelectionResult ElementImpl::findSelectionNode( int _x, int _y, int _tx, int _ty, DOM::Node & node, int & offset )
{
    //kdDebug(6030) << "ElementImpl::findSelectionNode " << this << " _x=" << _x << " _y=" << _y
    //           << " _tx=" << _tx << " _ty=" << _ty << endl;

    // ######### Duplicated code from mouseEvent
    // TODO put the code above (getting _tx,_ty) in a common place and call it from here

    if (!m_render) return SelectionPointAfter;

    RenderObject *p = m_render->parent();
    while( p && p->isAnonymousBox() ) {
// 	kdDebug( 6030 ) << "parent is anonymous!" << endl;
	// we need to add the offset of the anonymous box
	_tx += p->xPos();
	_ty += p->yPos();
	p = p->parent();
    }

    if(!m_render->isInline() || !m_render->firstChild() || m_render->isFloating() )
    {
        m_render->absolutePosition(_tx, _ty);
    }

    int off=0, lastOffset=0;
    DOM::Node nod;
    DOM::Node lastNode;
    NodeImpl *child = firstChild();
    while(child != 0)
    {
	khtml::FindSelectionResult pos = child->findSelectionNode(_x, _y, _tx, _ty, nod, off);
        //kdDebug(6030) << this << " child->findSelectionNode returned " << pos << endl;
        if ( pos == SelectionPointInside ) // perfect match
        {
            node = nod;
            offset = off;
            //kdDebug(6030) << "ElementImpl::findSelectionNode " << this << " match offset=" << offset << endl;
            return SelectionPointInside;
        } else if ( pos == SelectionPointBefore )
        {
            //x,y is before this element -> stop here
            if ( !lastNode.isNull() ) {
                node = lastNode;
                offset = lastOffset;
                //kdDebug(6030) << "ElementImpl::findSelectionNode " << this << " before this child -> returning offset=" << offset << endl;
                return SelectionPointInside;
            } else {
                //kdDebug(6030) << "ElementImpl::findSelectionNode " << this << " before us -> returning -2" << endl;
                return SelectionPointBefore;
            }
        }
        // SelectionPointAfter -> keep going
        if ( !nod.isNull() )
        {
            lastNode = nod;
            lastOffset = off;
        }
	child = child->nextSibling();
    }
    node = lastNode;
    offset = lastOffset;
    return SelectionPointAfter;
}

bool ElementImpl::isSelectable() const
{
    return false;
}

// DOM Section 1.1.1
bool ElementImpl::childAllowed( NodeImpl *newChild )
{
    switch (newChild->nodeType()) {
	case Node::ELEMENT_NODE:
	case Node::TEXT_NODE:
	case Node::COMMENT_NODE:
	case Node::PROCESSING_INSTRUCTION_NODE:
	case Node::CDATA_SECTION_NODE:
	case Node::ENTITY_REFERENCE_NODE:
	    // ### check xml element allowedness according to DTD
	    if (id() && newChild->id()) // if one if these is 0 then it is an xml element and we allow it anyway
		return checkChild(id(), newChild->id());
	    else
		return true;
	    break;
	default:
	    return false;
    }
}


// -------------------------------------------------------------------------

XMLElementImpl::XMLElementImpl(DocumentImpl *doc, DOMStringImpl *_name) : ElementImpl(doc)
{
    m_name = _name;
    if (m_name)
	m_name->ref();
    m_namespaceURI = 0;
    m_id = document->elementId(_name);
}

XMLElementImpl::XMLElementImpl(DocumentImpl *doc, DOMStringImpl *_name, DOMStringImpl *_namespaceURI) : ElementImpl(doc)
{
    m_name = _name;
    if (m_name)
	m_name->ref();
    m_namespaceURI = _namespaceURI;
    if (m_namespaceURI)
	m_namespaceURI->ref();
    m_id = document->elementId(_name);
}

XMLElementImpl::~XMLElementImpl()
{
    if (m_name)
	m_name->deref();
    if (m_namespaceURI)
	m_namespaceURI->deref();
}

const DOMString XMLElementImpl::nodeName() const
{
    return m_name;
}

DOMString XMLElementImpl::namespaceURI() const
{
    return m_namespaceURI;
}

// -------------------------------------------------------------------------

NamedAttrMapImpl::NamedAttrMapImpl(ElementImpl *e) : element(e)
{
    attrs = 0;
    len = 0;
}

NamedAttrMapImpl::~NamedAttrMapImpl()
{
    clearAttrs();
}

NamedAttrMapImpl &NamedAttrMapImpl::operator =(const AttributeList &list)
{
    uint i;
    clearAttrs(); // should be empty, but just in case...

    // now import the list
    len = list.length();
    attrs = new AttrImpl* [len];

    // first initialize attrs vector, then call parseAttribute on it
    // this allows parseAttribute to use getAttribute
    for (i = 0; i < len; i++)
	attrs[i] = new AttrImpl(list[i],element->ownerDocument(),element);
    for (i = 0; i < len; i++)
	element->parseAttribute(attrs[i]);

    // used only during parsing - we don't call applyChanges() here
    return *this;
}

NamedAttrMapImpl &NamedAttrMapImpl::operator =(const NamedAttrMapImpl &other)
{
    // clone all attributes in the other map, but attach to our element
    if (!element)
	return *this;

    clearAttrs();
    len = other.len;
    attrs = new AttrImpl* [len];
    uint i;

    // first initialize attrs vector, then call parseAttribute on it
    // this allows parseAttribute to use getAttribute
    for (i = 0; i < len; i++) {
	int exceptioncode; // ### propogate
	attrs[i] = static_cast<AttrImpl*>(other.attrs[i]->cloneNode(false,exceptioncode));
	attrs[i]->_element = element;
    }

    for(i = 0; i < len; i++)
	element->parseAttribute(attrs[i]);

    element->setChanged(true);
    return *this;
}

unsigned long NamedAttrMapImpl::length(int &/*exceptioncode*/) const
{
    return length();
}

unsigned long NamedAttrMapImpl::length() const
{
    return len;
}

NodeImpl *NamedAttrMapImpl::getNamedItem ( const DOMString &name, int &/*exceptioncode*/ ) const
{
    return getNamedItem(name);
}

NodeImpl *NamedAttrMapImpl::getNamedItem ( const DOMString &name ) const
{
    uint i;
    for (i = 0; i < len; i++) {
	if (!strcasecmp(attrs[i]->name(),name))
	    return attrs[i];
    }

    return 0;
}

AttrImpl *NamedAttrMapImpl::getIdItem ( int id ) const
{
    uint i;
    for (i = 0; i < len; i++) {
	if (attrs[i]->attrId == id)
	    return attrs[i];
    }

    return 0;
}


NodeImpl *NamedAttrMapImpl::setNamedItem ( const Node &arg, int &exceptioncode )
{
    // ### check for invalid chars in name ?
    // ### check same document
    exceptioncode = 0;
    if (!element) {
	exceptioncode = DOMException::NOT_FOUND_ERR;
	return 0;
    }
    element->checkReadOnly();

    if (arg.nodeType() != Node::ATTRIBUTE_NODE) {
	exceptioncode = DOMException::HIERARCHY_REQUEST_ERR;
	return 0;
    }

    AttrImpl *attr = static_cast<AttrImpl*>(arg.handle());

    if (attr->_element) {
	exceptioncode = DOMException::INUSE_ATTRIBUTE_ERR;
	return 0;
    }

    uint i;
    for (i = 0; i < len; i++) {
	// ### for XML attributes are case sensitive (?) - check this elsewhere also
	if (!strcasecmp(attrs[i]->name(),attr->name())) {
	    // attribute with this id already in list
	    AttrImpl *oldAttr = attrs[i];
	    attrs[i] = attr;
	    attr->_element = element;
	    oldAttr->_element = 0;
	    element->parseAttribute(attr);
	    element->setChanged(true);
	    return oldAttr; // ### check this gets deleted if ref = 0 and it's not assigned to anything
	}
    }

    // attribute with this name not yet in list
    AttrImpl **newAttrs = new AttrImpl* [len+1];
    if (attrs) {
      for (i = 0; i < len; i++)
        newAttrs[i] = attrs[i];
      delete [] attrs;
    }
    attrs = newAttrs;
    attrs[len] = attr;
    len++;
    attr->_element = element;
    element->parseAttribute(attr);
    element->setChanged(true);
    return 0;
}

AttrImpl *NamedAttrMapImpl::setIdItem ( AttrImpl *attr, int &exceptioncode )
{
    exceptioncode = 0;
    if (!element) {
	exceptioncode = DOMException::NOT_FOUND_ERR;
	return 0;
    }
    element->checkReadOnly();

    if (attr->_element) {
	exceptioncode = DOMException::INUSE_ATTRIBUTE_ERR;
	return 0;
    }

    uint i;
    for (i = 0; i < len; i++) {
	if (attrs[i]->attrId == attr->attrId) {
	    // attribute with this id already in list
	    AttrImpl *oldAttr = attrs[i];
	    attrs[i] = attr;
	    attr->_element = element;
	    oldAttr->_element = 0;
	    element->parseAttribute(attr);
	    element->setChanged(true);
	    return oldAttr; // ### check this gets deleted if ref = 0 and it's not assigned to anything
	}
    }

    // attribute with this id not yet in list
    AttrImpl **newAttrs = new AttrImpl* [len+1];
    if (attrs) {
      for (i = 0; i < len; i++)
        newAttrs[i] = attrs[i];
      delete [] attrs;
    }
    attrs = newAttrs;
    attrs[len] = attr;
    len++;
    attr->_element = element;
    element->parseAttribute(attr);
    element->setChanged(true);
    return 0;
}

NodeImpl *NamedAttrMapImpl::removeNamedItem ( const DOMString &name, int &exceptioncode )
{
    if (element)
	element->checkReadOnly();

    if (!attrs) {
 	exceptioncode = DOMException::NOT_FOUND_ERR;
	return 0;
    }

    uint i;
    int found = -1;
    for (i = 0; i < len && found < 0; i++) {
	if (!strcasecmp(attrs[i]->name(),name))
	    found = i;
    }
    if (found < 0) {
 	exceptioncode = DOMException::NOT_FOUND_ERR;
	return 0;
    }

    AttrImpl *ret = attrs[found];
    ret->_element = 0;
    if (len == 1) {
	delete [] attrs;
	attrs = 0;
	len = 0;
    } else {
	AttrImpl **newAttrs = new AttrImpl* [len-1];
	for (i = 0; i < uint(found); i++)
	    newAttrs[i] = attrs[i];
	len--;
	for (; i < len; i++)
	    newAttrs[i] = attrs[i+1];
	delete [] attrs;
	attrs = newAttrs;
    }
    DOMString nullStr;
    AttrImpl a(name,nullStr,element->ownerDocument());
    element->parseAttribute(&a);
    element->setChanged(true);

    return ret;
}

AttrImpl *NamedAttrMapImpl::removeIdItem ( int id )
{
    if (!attrs)
	return 0;

    uint i;
    int found = -1;
    for (i = 0; i < len && found < 0; i++) {
	if (attrs[i]->attrId == id)
	    found = i;
    }
    if (found < 0)
	return 0;

    AttrImpl *ret = attrs[found];
    ret->_element = 0;
    if (len == 1) {
	delete [] attrs;
	attrs = 0;
	len = 0;
    } else {
	AttrImpl **newAttrs = new AttrImpl* [len-1];
	for (i = 0; i < uint(found); i++)
	    newAttrs[i] = attrs[i];
	len--;
	for (; i < len; i++)
	    newAttrs[i] = attrs[i+1];
	delete [] attrs;
	attrs = newAttrs;
    }
    DOMString nullStr;
    AttrImpl a(id,nullStr,element->ownerDocument());
    element->parseAttribute(&a);
    element->setChanged(true);

    return ret;
}

NodeImpl *NamedAttrMapImpl::item ( unsigned long index, int &/*exceptioncode*/ ) const
{
    return item(index);
}

NodeImpl *NamedAttrMapImpl::item ( unsigned long index ) const
{
    if (index >= len)
	return 0;
    else
	return attrs[index];
}

void NamedAttrMapImpl::clearAttrs()
{
    if (attrs) {
	uint i;
	for (i = 0; i < len; i++) {
	    attrs[i]->_element = 0;
	    if (attrs[i]->deleteMe())
		delete attrs[i];
	}
	delete [] attrs;
	attrs = 0;
    }
    len = 0;
}

AttrImpl *NamedAttrMapImpl::removeAttr( AttrImpl *oldAttr, int &exceptioncode )
{
    exceptioncode = 0;
    uint i;
    for (i = 0; i < len; i++) {
	if (attrs[i] == oldAttr) {
	    AttrImpl *ret = attrs[i];

	    if (len == 1) {
		delete [] attrs;
		attrs = 0;
		len = 0;
	    }
	    else {
		AttrImpl **newAttrs = new AttrImpl* [len-1];
		uint ni;
		for (ni = 0; ni < i; ni++)
		    newAttrs[ni] = attrs[ni];
		len--;
		for (; ni < len; ni++)
		    newAttrs[ni] = attrs[ni+1];
		delete attrs;
		attrs = newAttrs;
	    }

	    ret->_element = 0;
	    AttrImpl a = oldAttr->attrId ? AttrImpl(oldAttr->attrId,"",element->ownerDocument()) :
	                               AttrImpl(oldAttr->name(),"",element->ownerDocument());
	    element->parseAttribute(&a);
	    element->setChanged(true);
	    return ret;
	}
    }
    exceptioncode = DOMException::NOT_FOUND_ERR;
    return 0;

}

void NamedAttrMapImpl::detachFromElement()
{
    // we allow a NamedAttrMapImpl w/o an element in case someone still has a reference
    // to if after the element gets deleted - but the map is now invalid
    element = 0;
    clearAttrs();
}
