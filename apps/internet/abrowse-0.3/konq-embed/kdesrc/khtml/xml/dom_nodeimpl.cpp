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
 */
#include "dom_nodeimpl.h"

#include "dom_node.h"
#include "dom_exception.h"
#include "htmlattrs.h"

#include "dom_elementimpl.h"
#include "dom_docimpl.h"

#include <kdebug.h>

#include "rendering/render_object.h"
#include "rendering/render_text.h"
#include <qrect.h>

using namespace DOM;
using namespace khtml;

NodeImpl::NodeImpl(DocumentImpl *doc)
    : document(doc),
      m_render(0),
      m_complexText( false ),
      m_hasEvents( false ),
      m_hasId( false ),
      m_hasClass( false ),
      m_hasStyle( false ),
      m_hasTooltip( false ),
      m_pressed( false ),
      m_mouseInside( false ),
      m_attached( false ),
      m_changed( false ),
      m_specified( false ),
      m_focused( false ),
      m_active( false )
{
}

NodeImpl::~NodeImpl()
{
    setOwnerDocument(0);
}

DOMString NodeImpl::nodeValue() const
{
  return 0;
}

void NodeImpl::setNodeValue( const DOMString &, int &exceptioncode )
{
  exceptioncode = DOMException::NO_MODIFICATION_ALLOWED_ERR;
}

const DOMString NodeImpl::nodeName() const
{
  return 0;
}

unsigned short NodeImpl::nodeType() const
{
  return 0;
}

NodeImpl *NodeImpl::parentNode() const
{
  return 0;
}

NodeListImpl *NodeImpl::childNodes()
{
  return new ChildNodeListImpl(this);
}

NodeImpl *NodeImpl::firstChild() const
{
  return 0;
}

NodeImpl *NodeImpl::lastChild() const
{
  return 0;
}

NodeImpl *NodeImpl::previousSibling() const
{
  return 0;
}

NodeImpl *NodeImpl::nextSibling() const
{
  return 0;
}

NamedNodeMapImpl *NodeImpl::attributes() const
{
  return 0;
}

void NodeImpl::setPreviousSibling(NodeImpl *)
{
}

void NodeImpl::setNextSibling(NodeImpl *)
{
}

void NodeImpl::setOwnerDocument(DocumentImpl *_document)
{
    if (document)
	document->changedNodes.remove(this);

    document = _document;
}


NodeImpl *NodeImpl::insertBefore( NodeImpl *, NodeImpl *, int &exceptioncode )
{
    exceptioncode = DOMException::HIERARCHY_REQUEST_ERR;
    return 0;
}

NodeImpl *NodeImpl::replaceChild( NodeImpl *, NodeImpl *, int &exceptioncode )
{
  exceptioncode = DOMException::HIERARCHY_REQUEST_ERR;
  return 0;
}

NodeImpl *NodeImpl::removeChild( NodeImpl *, int &exceptioncode )
{
  exceptioncode = DOMException::NOT_FOUND_ERR;
  return 0;
}

NodeImpl *NodeImpl::appendChild( NodeImpl *, int &exceptioncode )
{
  exceptioncode = DOMException::HIERARCHY_REQUEST_ERR;
  return 0;
}

bool NodeImpl::hasChildNodes(  ) const
{
  return false;
}

// helper functions not being part of the DOM
void NodeImpl::setParent(NodeImpl *)
{
}

void NodeImpl::setFirstChild(NodeImpl *)
{
}

void NodeImpl::setLastChild(NodeImpl *)
{
}

NodeImpl *NodeImpl::addChild(NodeImpl *)
{
  return 0;
}

QString NodeImpl::toHTML() const
{
    return recursive_toHTML( );
}

QString NodeImpl::recursive_toHTML( ) const
{
    QString me;

    // Copy who I am into the htmlText string
    if ( nodeType() == Node::TEXT_NODE )
        me = nodeValue().string();
    else
    {   // If I am an element, not a text
        me = QChar('<') + nodeName().string();

        // print attributes
        if( nodeType() == Node::ELEMENT_NODE )
        {
            ElementImpl *el = const_cast<ElementImpl*>(static_cast<const ElementImpl *>(this));
            AttrImpl *attr;
            int exceptioncode;
            NamedNodeMapImpl *attrs = el->attributes();
            unsigned long lmap = attrs->length(exceptioncode);
            for( unsigned int j=0; j<lmap; j++ )
            {
                attr = static_cast<AttrImpl*>(attrs->item(j,exceptioncode));
                me += " " + attr->name().string() + "=\"" + attr->value().string() + "\"";
            }
        }

        // print ending bracket of start tag
        if( firstChild() == 0 )     // if element has no endtag
            me += " />";
        else                        // if element has endtag
            me += ">";
    }

    NodeImpl* n;

    if( (n = firstChild()) )
    {
        // print firstChild
        me += n->recursive_toHTML( );

        // Print my ending tag
        if ( nodeType() != Node::TEXT_NODE )
            me += "</" + nodeName().string() + ">";
    }
    // print next sibling
    if( (n = nextSibling()) )
        me += n->recursive_toHTML( );

    return me;
}

void NodeImpl::applyChanges(bool, bool)
{
    setChanged(false);
}

void NodeImpl::getCursor(int offset, int &_x, int &_y, int &height)
{
    if(m_render) m_render->cursorPos(offset, _x, _y, height);
    else _x = _y = height = -1;
}

QRect NodeImpl::getRect() const
{
    int _x, _y;
    if(m_render && m_render->absolutePosition(_x, _y))
        return QRect( _x, _y, m_render->width(), m_render->height() );
    return QRect();
}

void NodeImpl::setChanged(bool b)
{
    if (b && !changed() && document)
	document->changedNodes.append(this);
    m_changed = b;
}

DOMString NodeImpl::namespaceURI() const
{
    return 0;
}


void NodeImpl::printTree(int indent)
{
    QString ind;
    QString s;
    ind.fill(' ', indent);

    // ### find out why this isn't working
    if(isElementNode())
    {
        s = ind + "<" + nodeName().string();

        ElementImpl *el = const_cast<ElementImpl*>(static_cast<const ElementImpl *>(this));
        AttrImpl *attr;
        int exceptioncode;
        NamedNodeMapImpl *attrs = el->attributes();
        unsigned long lmap = attrs->length(exceptioncode);
        for( unsigned int j=0; j<lmap; j++ )
        {
            attr = static_cast<AttrImpl*>(attrs->item(j,exceptioncode));
            s += " " + attr->name().string() + "=\"" + attr->value().string() + "\"";
        }
        if(!firstChild())
            s += " />";
        else
            s += ">";
    }
    else
        s = ind + "'" + nodeValue().string() + "'";

    kdDebug() << s << endl;

    NodeImpl *child = firstChild();
    while( child )
    {
        child->printTree(indent+2);
        child = child->nextSibling();
    }
    if(isElementNode() && firstChild())
        kdDebug() << ind << "</" << nodeName().string() << ">" << endl;
}

//--------------------------------------------------------------------

NodeWParentImpl::NodeWParentImpl(DocumentImpl *doc) : NodeImpl(doc)
{
    _parent = 0;
    _previous = 0;
    _next = 0;
}

NodeWParentImpl::~NodeWParentImpl()
{
    // previous and next node may still reference this!!!
    // hope this fix is fine...
    if(_previous) _previous->setNextSibling(0);
    if(_next) _next->setPreviousSibling(0);
}

NodeImpl *NodeWParentImpl::parentNode() const
{
    return _parent;
}

NodeImpl *NodeWParentImpl::previousSibling() const
{
    return _previous;
}

NodeImpl *NodeWParentImpl::nextSibling() const
{
    return _next;
}

// not part of the DOM
void NodeWParentImpl::setParent(NodeImpl *n)
{
    _parent = n;
}

bool NodeWParentImpl::deleteMe()
{
    if(!_parent && _ref <= 0) return true;
    return false;
}

void NodeWParentImpl::setPreviousSibling(NodeImpl *n)
{
    _previous = n;
}

void NodeWParentImpl::setNextSibling(NodeImpl *n)
{
    _next = n;
}

bool NodeWParentImpl::checkReadOnly() const
{
    // ####
    return false;
}

//-------------------------------------------------------------------------

NodeBaseImpl::NodeBaseImpl(DocumentImpl *doc) : NodeWParentImpl(doc)
{
    _first = _last = 0;
    m_style = 0;
}


NodeBaseImpl::~NodeBaseImpl()
{
    //kdDebug( 6020 ) << "NodeBaseImpl destructor" << endl;
    // we have to tell all children, that the parent has died...
    NodeImpl *n;
    NodeImpl *next;

    for( n = _first; n != 0; n = next )
    {
	next = n->nextSibling();
        n->setPreviousSibling(0);
        n->setNextSibling(0);
	n->setParent(0);
	if(n->deleteMe())
	    delete n;
	else
	    n->setOwnerDocument(0);
    }
    if (m_style)
	m_style->deref();
}


NodeImpl *NodeBaseImpl::firstChild() const
{
    return _first;
}

NodeImpl *NodeBaseImpl::lastChild() const
{
    return _last;
}

NodeImpl *NodeBaseImpl::insertBefore ( NodeImpl *newChild, NodeImpl *refChild, int &exceptioncode )
{
    exceptioncode = 0;
    if (checkReadOnly()) {
	exceptioncode = DOMException::NO_MODIFICATION_ALLOWED_ERR;
	return 0;
    }
    if (!newChild || (newChild->nodeType() == Node::DOCUMENT_FRAGMENT_NODE && !newChild->firstChild())) {
	exceptioncode = DOMException::NOT_FOUND_ERR;
	return 0;
    }

    if(!refChild)
	return appendChild(newChild, exceptioncode);

    if( checkSameDocument(newChild, exceptioncode) )
	return 0;
    if( checkNoOwner(newChild, exceptioncode) )
	return 0;
    if( checkIsChild(refChild, exceptioncode) )
	return 0;

    if(newChild->parentNode() == this)
	removeChild(newChild, exceptioncode);
    if( exceptioncode )
	return 0;

    bool isFragment = newChild->nodeType() == Node::DOCUMENT_FRAGMENT_NODE;
    NodeImpl *nextChild;
    NodeImpl *child = isFragment ? newChild->firstChild() : newChild;

    NodeImpl *prev = refChild->previousSibling();
    while (child) {
	nextChild = isFragment ? child->nextSibling() : 0;

	if( checkNoOwner(child, exceptioncode) )
	    return 0;
	if(!childAllowed(child)) {
	    exceptioncode = DOMException::HIERARCHY_REQUEST_ERR;
	    return 0;
	}
	// if already in the tree, remove it first!
	NodeImpl *newParent = child->parentNode();
	if(newParent)
	    newParent->removeChild( child, exceptioncode );
	if ( exceptioncode )
	    return 0;

	// seems ok, lets's insert it.
	if (prev)
	    prev->setNextSibling(child);
	else
	    _first = child;
	refChild->setPreviousSibling(child);
	child->setParent(this);
	child->setPreviousSibling(prev);
	child->setNextSibling(refChild);
	if (attached() && !child->attached())
	    child->attach(document ? document->view() : static_cast<DocumentImpl*>(this)->view());

	prev = child;
	child = nextChild;
    }

    // ### set style in case it's attached
    setChanged(true);

    return newChild;
}

NodeImpl *NodeBaseImpl::replaceChild ( NodeImpl *newChild, NodeImpl *oldChild, int &exceptioncode )
{
    exceptioncode = 0;
    if (checkReadOnly()) {
	exceptioncode = DOMException::NO_MODIFICATION_ALLOWED_ERR;
	return 0;
    }
    if (!newChild || (newChild->nodeType() == Node::DOCUMENT_FRAGMENT_NODE && !newChild->firstChild())) {
	exceptioncode = DOMException::NOT_FOUND_ERR;
	return 0;
    }
    if( checkSameDocument(newChild, exceptioncode) )
	return 0;
    if( checkIsChild(oldChild, exceptioncode) )
	return 0;
    if( checkNoOwner(newChild, exceptioncode) )
	return 0;

    bool isFragment = newChild->nodeType() == Node::DOCUMENT_FRAGMENT_NODE;
    NodeImpl *nextChild;
    NodeImpl *child = isFragment ? newChild->firstChild() : newChild;

    // make sure we will be able to insert the first node before we go removing the old one
    if( checkNoOwner(isFragment ? newChild->firstChild() : newChild, exceptioncode) )
	return 0;
    if(!childAllowed(isFragment ? newChild->firstChild() : newChild)) {
	exceptioncode = DOMException::HIERARCHY_REQUEST_ERR;
	return 0;
    }

    NodeImpl *prev = oldChild->previousSibling();
    NodeImpl *next = oldChild->nextSibling();
    oldChild->setPreviousSibling(0);
    oldChild->setNextSibling(0);
    oldChild->setParent(0);
    if (m_render && oldChild->renderer())
	m_render->removeChild(oldChild->renderer());

    while (child) {
	nextChild = isFragment ? child->nextSibling() : 0;

	if( checkNoOwner(child, exceptioncode ) )
	    return 0;
	if(!childAllowed(child)) {
	    exceptioncode = DOMException::HIERARCHY_REQUEST_ERR;
	    return 0;
	}

	// if already in the tree, remove it first!
	NodeImpl *newParent = child->parentNode();
	if(newParent)
	    newParent->removeChild( child, exceptioncode );
	if ( exceptioncode )
	    return 0;

	// seems ok, lets's insert it.
	if (prev) prev->setNextSibling(child);
	if (next) next->setPreviousSibling(child);
	if(!prev) _first = child;
	if(!next) _last = child;

	child->setParent(this);
	child->setPreviousSibling(prev);
	child->setNextSibling(next);
	if (attached() && !child->attached())
	    child->attach(document ? document->view() : static_cast<DocumentImpl*>(this)->view());
	prev = child;
	child = nextChild;
    }

    // ### set style in case it's attached
    setChanged(true);

    return oldChild;
}

NodeImpl *NodeBaseImpl::removeChild ( NodeImpl *oldChild, int &exceptioncode )
{
    exceptioncode = 0;
    if( checkReadOnly() )
	return 0;
    if( checkIsChild(oldChild, exceptioncode) )
	return 0;

    NodeImpl *prev, *next;
    prev = oldChild->previousSibling();
    next = oldChild->nextSibling();

    if(next) next->setPreviousSibling(prev);
    if(prev) prev->setNextSibling(next);
    if(_first == oldChild) _first = next;
    if(_last == oldChild) _last = prev;

    oldChild->setPreviousSibling(0);
    oldChild->setNextSibling(0);
    oldChild->setParent(0);
    if (oldChild->attached())
	oldChild->detach();

    setChanged(true);

    return oldChild;
}

void NodeBaseImpl::removeChildren()
{
    NodeImpl *n, *next;
    for( n = _first; n != 0; n = next )
    {
	next = n->nextSibling();
        n->setPreviousSibling(0);
        n->setNextSibling(0);
	n->setParent(0);
	if (n->renderer() && n->renderer()->parent())
	    n->renderer()->parent()->removeChild(n->renderer());
	n->setRenderer( 0 );
	n->setStyle( 0 );
	if(n->deleteMe())
	    delete n;
	else
	    n->setOwnerDocument(0);
    }
    _first = _last = 0;
}


NodeImpl *NodeBaseImpl::appendChild ( NodeImpl *newChild, int &exceptioncode )
{
//    kdDebug(6010) << "NodeBaseImpl::appendChild( " << newChild << " );" <<endl;
    checkReadOnly();
    if (!newChild || (newChild->nodeType() == Node::DOCUMENT_FRAGMENT_NODE && !newChild->firstChild())) {
	exceptioncode = DOMException::NOT_FOUND_ERR;
	return 0;
    }
    if( checkSameDocument(newChild, exceptioncode) )
	return 0;
    if( checkNoOwner(newChild, exceptioncode) )
	return 0;

    if(newChild->parentNode() == this)
	removeChild(newChild, exceptioncode);
    if ( exceptioncode )
	return 0;

    bool isFragment = newChild->nodeType() == Node::DOCUMENT_FRAGMENT_NODE;
    NodeImpl *nextChild;
    NodeImpl *child = isFragment ? newChild->firstChild() : newChild;

    while (child) {
	nextChild = isFragment ? child->nextSibling() : 0;

	if (checkNoOwner(child, exceptioncode) )
	    return 0;
	if(!childAllowed(child)) {
	    exceptioncode = DOMException::HIERARCHY_REQUEST_ERR;
	    return 0;
	}

	// if already in the tree, remove it first!
	NodeImpl *oldParent = child->parentNode();
	if(oldParent)
	    oldParent->removeChild( child, exceptioncode );
	if ( exceptioncode )
	    return 0;

	// lets append it
	child->setParent(this);

	if(_last)
	{
	    child->setPreviousSibling(_last);
	    _last->setNextSibling(child);
	    _last = child;
	}
	else
	{
	    _first = _last = child;
	}
	if (attached() && !child->attached())
	    child->attach(document ? document->view() : static_cast<DocumentImpl*>(this)->view());

	child = nextChild;
    }

    setChanged(true);
    // ### set style in case it's attached
    return newChild;
}

bool NodeBaseImpl::hasChildNodes (  ) const
{
    return _first != 0;
}

// not part of the DOM
void NodeBaseImpl::setFirstChild(NodeImpl *child)
{
    _first = child;
}

void NodeBaseImpl::setLastChild(NodeImpl *child)
{
    _last = child;
}

// check for same source document:
bool NodeBaseImpl::checkSameDocument( NodeImpl *newChild, int &exceptioncode )
{
    exceptioncode = 0;
    DocumentImpl *ownerDocThis = static_cast<DocumentImpl*>(nodeType() == Node::DOCUMENT_NODE ? this : ownerDocument());
    DocumentImpl *ownerDocNew = static_cast<DocumentImpl*>(newChild->nodeType() == Node::DOCUMENT_NODE ? newChild : newChild->ownerDocument());
    if(ownerDocThis != ownerDocNew) {
	kdDebug(6010)<< "not same document, newChild = " << newChild << "document = " << document << endl;
	exceptioncode = DOMException::WRONG_DOCUMENT_ERR;
	return true;
    }
    return false;
}

// check for being (grand-..)father:
bool NodeBaseImpl::checkNoOwner( NodeImpl *newChild, int &exceptioncode )
{
  //check if newChild is parent of this...
  NodeImpl *n;
  for( n = this; n != (NodeImpl *)document && n!= 0; n = n->parentNode() )
      if(n == newChild) {
	  exceptioncode = DOMException::HIERARCHY_REQUEST_ERR;
	  return true;
      }
  return false;
}

// check for being child:
bool NodeBaseImpl::checkIsChild( NodeImpl *oldChild, int &exceptioncode )
{
    if(!oldChild || oldChild->parentNode() != this) {
	exceptioncode = DOMException::NOT_FOUND_ERR;
	return true;
    }
    return false;
}

bool NodeBaseImpl::childAllowed( NodeImpl */*newChild*/ )
{
    return false;
}

NodeImpl *NodeBaseImpl::addChild(NodeImpl *newChild)
{
    // do not add applyChanges here! This function is only used during parsing

    // short check for consistency with DTD
    if(!childAllowed(newChild))
    {
        //kdDebug( 6020 ) << "AddChild failed! id=" << id() << ", child->id=" << newChild->id() << endl;
	return 0;
    }

    // just add it...
    newChild->setParent(this);

    if(_last)
    {
	newChild->setPreviousSibling(_last);
	_last->setNextSibling(newChild);
	_last = newChild;
    }
    else
    {
	_first = _last = newChild;
    }
    if(newChild->nodeType() == Node::ELEMENT_NODE)
	return newChild;
    return this;
}

void NodeBaseImpl::attach(KHTMLView *w)
{
    NodeImpl *child = _first;
    while(child != 0)
    {
	child->attach(w);
	child = child->nextSibling();
    }
    NodeWParentImpl::attach(w);
}

void NodeBaseImpl::detach()
{
    NodeImpl *child = _first;
    while(child != 0)
    {
	child->detach();
	child = child->nextSibling();
    }
    NodeWParentImpl::detach();
}

void NodeBaseImpl::setOwnerDocument(DocumentImpl *_document)
{
    NodeImpl *n;
    for(n = _first; n != 0; n = n->nextSibling())
	n->setOwnerDocument(_document);
    NodeWParentImpl::setOwnerDocument(_document);
}

void NodeBaseImpl::cloneChildNodes(NodeImpl *clone, int &exceptioncode)
{
    NodeImpl *n;
//    for(n = firstChild(); n != lastChild() && !exceptioncode; n = n->nextSibling())
    for(n = firstChild(); n && !exceptioncode; n = n->nextSibling())
    {
	clone->appendChild(n->cloneNode(true,exceptioncode),exceptioncode);
    }
}

// I don't like this way of implementing the method, but I didn't find any
// other way. Lars
bool NodeBaseImpl::getUpperLeftCorner(int &xPos, int &yPos) const
{
    if (!m_render)
	return false;
    RenderObject *o = m_render;
    o->absolutePosition( xPos, yPos );
    if ( !isInline() )
	return true;

    // find the next text/image child, to get a position
    while(o) {
	if(o->firstChild())
	    o = o->firstChild();
	else if(o->nextSibling())
	    o = o->nextSibling();
	else {
	    RenderObject *next = 0;
	    while(!next) {
		o = o->parent();
		if(!o) return false;
		next = o->nextSibling();
	    }
	    o = next;
	}
	if(o->isText() || o->isReplaced()) {
	    if (o->isText())
		xPos += static_cast<RenderText *>(o)->minXPos();
	    else
		xPos += o->xPos();
	    yPos += o->yPos();
	    return true;
	}
    }
    return true;
}

bool NodeBaseImpl::getLowerRightCorner(int &xPos, int &yPos) const
{
    if (!m_render)
	return false;

    RenderObject *o = m_render;
    o->absolutePosition( xPos, yPos );
    if (!isInline())
    {
	xPos += o->width();
	yPos += o->height();
	return true;
    }
    // find the last text/image child, to get a position
    while(o) {
	if(o->lastChild())
	    o = o->lastChild();
	else if(o->previousSibling())
	    o = o->previousSibling();
	else {
	    RenderObject *prev = 0;
	    while(!prev) {
		o = o->parent();
		if(!o) return false;
		prev = o->previousSibling();
	    }
	    o = prev;
	}
	if(o->isText() || o->isReplaced()) {
	    if (o->isText())
		xPos += static_cast<RenderText *>(o)->minXPos() + o->width();
	    else
		xPos += o->xPos()+o->intrinsicWidth();
	    yPos += o->yPos()+o->height();
	    return true;
	}
    }
    return true;
}

QRect NodeBaseImpl::getRect() const
{
    int xPos, yPos;
    if (!getUpperLeftCorner(xPos,yPos))
    {
	xPos=0;
	yPos=0;
    }
    int xEnd, yEnd;
    if (!getLowerRightCorner(xEnd,yEnd))
    {
	if (xPos)
	    xEnd = xPos;
	if (yPos)
	    yEnd = yPos;
    }
    else
    {
	if (xPos==0)
	    xPos = xEnd;
	if (yPos==0)
	    yPos = yEnd;
    }
    return QRect(xPos, yPos, xEnd - xPos, yEnd - yPos);
}

void NodeBaseImpl::setStyle(khtml::RenderStyle *style)
{
    RenderStyle *oldStyle = m_style;
    m_style = style;
    if (m_style)
	m_style->ref();
    if (oldStyle)
	oldStyle->deref();
}

void NodeBaseImpl::setFocus(bool received)
{
    NodeImpl::setFocus(received);
    for(NodeImpl *it=_first;it;it=it->nextSibling())
        it->setFocus(received);
}

void NodeBaseImpl::setActive(bool down)
{
    NodeImpl::setActive(down);
    for(NodeImpl *it=_first;it;it=it->nextSibling())
        it->setActive(down);
}

// ---------------------------------------------------------------------------

NodeImpl *NodeListImpl::item( unsigned long /*index*/ ) const
{
    return 0;
}

unsigned long NodeListImpl::length() const
{
    return 0;
}

unsigned long NodeListImpl::recursiveLength(NodeImpl *start) const
{
    unsigned long len = 0;

    for(NodeImpl *n = start->firstChild(); n != 0; n = n->nextSibling()) {
	if ( n->nodeType() == Node::ELEMENT_NODE ) {
	    if (nodeMatches(n))
		len++;
	    len+= recursiveLength(n);
	}
    }

    return len;
}

NodeImpl *NodeListImpl::recursiveItem ( NodeImpl *start, unsigned long &offset ) const
{
    for(NodeImpl *n = start->firstChild(); n != 0; n = n->nextSibling()) {
	if ( n->nodeType() == Node::ELEMENT_NODE ) {
	    if (nodeMatches(n))
		if (!offset--)
		    return n;

	    NodeImpl *depthSearch= recursiveItem(n, offset);
	    if (depthSearch)
		return depthSearch;
	}
    }

    return 0; // no matching node in this subtree
}

bool NodeListImpl::nodeMatches( NodeImpl */*testNode*/ ) const
{
  // ###
    return false;
}


ChildNodeListImpl::ChildNodeListImpl( NodeImpl *n )
{
    refNode = n;
    refNode->ref();
}

ChildNodeListImpl::~ChildNodeListImpl()
{
    refNode->deref();
}

unsigned long ChildNodeListImpl::length() const
{
    unsigned long len = 0;
    NodeImpl *n;
    for(n = refNode->firstChild(); n != 0; n = n->nextSibling())
	len++;

    return len;
}

NodeImpl *ChildNodeListImpl::item ( unsigned long index ) const
{
    unsigned int pos = 0;
    NodeImpl *n = refNode->firstChild();

    while( n != 0 && pos < index )
    {
	n = n->nextSibling();
	pos++;
    }

    return n;
}



TagNodeListImpl::TagNodeListImpl(NodeImpl *n, const DOMString &t )
  : tagName(t)
{
    refNode = n;
    refNode->ref();
    allElements = (t == "*");
}

TagNodeListImpl::~TagNodeListImpl()
{
    refNode->deref();
}

unsigned long TagNodeListImpl::length() const
{
    return recursiveLength( refNode );
}

NodeImpl *TagNodeListImpl::item ( unsigned long index ) const
{
    return recursiveItem( refNode, index );
}

bool TagNodeListImpl::nodeMatches( NodeImpl *testNode ) const
{
    return ((allElements && testNode->nodeType() == Node::ELEMENT_NODE) ||
            !strcasecmp(testNode->nodeName(),tagName));
}



NameNodeListImpl::NameNodeListImpl(NodeImpl *n, const DOMString &t )
  : nodeName(t)
{
    refNode= n;
    refNode->ref();
}

NameNodeListImpl::~NameNodeListImpl()
{
    refNode->deref();
}

unsigned long NameNodeListImpl::length() const
{
    return recursiveLength( refNode );
}

NodeImpl *NameNodeListImpl::item ( unsigned long index ) const
{
    return recursiveItem( refNode, index );
}

bool NameNodeListImpl::nodeMatches( NodeImpl *testNode ) const
{
    return static_cast<ElementImpl *>(testNode)->getAttribute(ATTR_NAME) == nodeName;
}

// ---------------------------------------------------------------------------

NamedNodeMapImpl::NamedNodeMapImpl()
{
}

NamedNodeMapImpl::~NamedNodeMapImpl()
{
}

// ----------------------------------------------------------------------------

GenericRONamedNodeMapImpl::GenericRONamedNodeMapImpl() : NamedNodeMapImpl()
{
    // not sure why this doesn't work as a normal object
    m_contents = new QList<NodeImpl>;
}

GenericRONamedNodeMapImpl::~GenericRONamedNodeMapImpl()
{
    while (m_contents->count() > 0)
	m_contents->take(0)->deref();

    delete m_contents;
}

unsigned long GenericRONamedNodeMapImpl::length(int &/*exceptioncode*/) const
{
    return m_contents->count();
}

NodeImpl *GenericRONamedNodeMapImpl::getNamedItem ( const DOMString &name, int &/*exceptioncode*/ ) const
{
    QListIterator<NodeImpl> it(*m_contents);
    for (; it.current(); ++it)
	if (it.current()->nodeName() == name)
	    return it.current();
    return 0;
}

NodeImpl *GenericRONamedNodeMapImpl::setNamedItem ( const Node &/*arg*/, int &exceptioncode )
{
    // can't modify this list through standard DOM functions
    exceptioncode = DOMException::NO_MODIFICATION_ALLOWED_ERR;
    return 0;
}

NodeImpl *GenericRONamedNodeMapImpl::removeNamedItem ( const DOMString &/*name*/, int &exceptioncode )
{
    // can't modify this list through standard DOM functions
    exceptioncode = DOMException::NO_MODIFICATION_ALLOWED_ERR;
    return 0;
}

NodeImpl *GenericRONamedNodeMapImpl::item ( unsigned long index, int &/*exceptioncode*/ ) const
{
    // ### check this when calling from javascript using -1 = 2^sizeof(int)-1
    // (also for other similar methods)
    if (index >= m_contents->count())
	return 0;

    return m_contents->at(index);
}

void GenericRONamedNodeMapImpl::addNode(NodeImpl *n)
{
    // The spec says that in the case of duplicates we only keep the first one
    int exceptioncode;
    if (getNamedItem(n->nodeName(),exceptioncode))
	return;

    n->ref();
    m_contents->append(n);
}

