/**
 * This file is part of the DOM implementation for KDE.
 *
 * (C) 1999 Lars Knoll (knoll@kde.org)
 * (C) 2000 Gunnstein Lye (gunnstein@netcom.no)
 * (C) 2000 Frederik Holljen (frederik.holljen@hig.no)
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
#include "dom2_range.h"
#include "dom2_traversal.h"
#include "dom_node.h"
#include "dom_doc.h"
#include "dom_string.h"
#include "dom_text.h"
#include "dom_exception.h"
#include "dom_docimpl.h"
#include "dom2_rangeimpl.h"
#include <qstring.h>
#include <stdio.h>               // for printf

using namespace DOM;


Range::Range()
{
    impl = 0;
}

Range::Range(const Document rootContainer)
{
    impl = new RangeImpl(rootContainer);
    impl->ref();
}

Range::Range(const Range &other)
{
    impl = other.impl;
    if (impl) impl->ref();
}

Range::Range(const Node startContainer, const long startOffset, const Node endContainer, const long endOffset)
{
    impl = new RangeImpl(startContainer,startOffset,endContainer,endOffset);
    impl->ref();
}

Range::Range(RangeImpl *i)
{
    impl = i;
    if (impl) impl->ref();
}

Range &Range::operator = (const Range &other)
{
    if (impl) impl->deref();
    impl = other.impl;
    if (impl) impl->ref();

    return *this;
}

Range::~Range()
{
    if (impl) impl->deref();
}

Node Range::startContainer() const
{
    checkCommon();
    return impl->getStartContainer();
}

long Range::startOffset() const
{
    checkCommon();
    return impl->getStartOffset();
}

Node Range::endContainer() const
{
    checkCommon();
    return impl->getEndContainer();
}

long Range::endOffset() const
{
    checkCommon();
    return impl->getEndOffset();
}

bool Range::collapsed() const
{
    checkCommon();
    return impl->getCollapsed();
    return false;
}

Node Range::commonAncestorContainer()
{
    checkCommon();
    return impl->getCommonAncestorContainer();
}

void Range::setStart( const Node &refNode, long offset )
{
    checkNodeWOffset( refNode, offset );
    impl->setStart(refNode,offset);
}

void Range::setEnd( const Node &refNode, long offset )
{
    checkNodeWOffset( refNode, offset );
    impl->setEnd(refNode,offset);
}

void Range::setStartBefore( const Node &refNode )
{
    checkNodeBA( refNode );
    
    try
    {
        setStart( refNode.parentNode(), refNode.index() );
    }
    catch( RangeException r )
    {
        if( r.code == RangeException::INVALID_NODE_TYPE_ERR )
            fprintf( stderr, "Exception: Invalid Node type\n" );
        return;
    }
    catch( DOMException d )
    {
        if( d.code == DOMException::NOT_FOUND_ERR )
            fprintf( stderr, "Exception: Null Nodes not accepted\n" );
        if( d.code == DOMException::INDEX_SIZE_ERR )
            fprintf( stderr, "Exception: offset has wrong size\n" );
        if( d.code == DOMException::INVALID_STATE_ERR )
            fprintf( stderr, "Exception: detach() has been invoked\n" );
        return;
    }
}

void Range::setStartAfter( const Node &refNode )
{
    checkNodeBA( refNode );

    try
    {
        setStart( refNode.parentNode(), refNode.index()+1 );
    }
    catch( RangeException r )
    {
        if( r.code == RangeException::INVALID_NODE_TYPE_ERR )
            fprintf( stderr, "Exception: Invalid Node type\n" );
        return;
    }
    catch( DOMException d )
    {
        if( d.code == DOMException::NOT_FOUND_ERR )
            fprintf( stderr, "Exception: Null Nodes not accepted\n" );
        if( d.code == DOMException::INDEX_SIZE_ERR )
            fprintf( stderr, "Exception: offset has wrong size\n" );
        if( d.code == DOMException::INVALID_STATE_ERR )
            fprintf( stderr, "Exception: detach() has been invoked\n" );
        return;
    }
}

void Range::setEndBefore( const Node &refNode )
{
    checkNodeBA( refNode );

    try
    {
        setEnd( refNode.parentNode(), refNode.index() );
    }
    catch( RangeException r )
    {
        if( r.code == RangeException::INVALID_NODE_TYPE_ERR )
            fprintf( stderr, "Exception: Invalid Node type\n" );
        return;
    }
    catch( DOMException d )
    {
        if( d.code == DOMException::NOT_FOUND_ERR )
            fprintf( stderr, "Exception: Null Nodes not accepted\n" );
        if( d.code == DOMException::INDEX_SIZE_ERR )
            fprintf( stderr, "Exception: offset has wrong size\n" );
        if( d.code == DOMException::INVALID_STATE_ERR )
            fprintf( stderr, "Exception: detach() has been invoked\n" );
        return;
    }
}

void Range::setEndAfter( const Node &refNode )
{
    checkNodeBA( refNode );

    try
    {
        setEnd( refNode.parentNode(), refNode.index()+1 );
    }
    catch( RangeException r )
    {
        if( r.code == RangeException::INVALID_NODE_TYPE_ERR )
            fprintf( stderr, "Exception: Invalid Node type\n" );
        return;
    }
    catch( DOMException d )
    {
        if( d.code == DOMException::NOT_FOUND_ERR )
            fprintf( stderr, "Exception: Null Nodes not accepted\n" );
        if( d.code == DOMException::INDEX_SIZE_ERR )
            fprintf( stderr, "Exception: offset has wrong size\n" );
        if( d.code == DOMException::INVALID_STATE_ERR )
            fprintf( stderr, "Exception: detach() has been invoked\n" );
        return;
    }
}

void Range::collapse( bool toStart )
{
    checkCommon();
    impl->collapse(toStart);
}

void Range::selectNode( const Node &refNode )
{
    checkNodeBA( refNode );
    try
    {
        setStartBefore( refNode );
        setEndAfter( refNode );
    }
    catch( RangeException r )
    {
        if( r.code == RangeException::INVALID_NODE_TYPE_ERR )
            fprintf( stderr, "Exception: Invalid Node type\n" );
        return;
    }
    catch( DOMException d )
    {
        if( d.code == DOMException::NOT_FOUND_ERR )
            fprintf( stderr, "Exception: Null Nodes not accepted\n" );
        if( d.code == DOMException::INVALID_STATE_ERR )
            fprintf( stderr, "Exception: detach() has been invoked\n" );
        return;
    }
}

void Range::selectNodeContents( const Node &refNode )
{
    checkNode( refNode );

    try
    {
        setStartBefore( refNode.firstChild() );
        setEndAfter( refNode.lastChild() );
    }
    catch( RangeException r )
    {
        if( r.code == RangeException::INVALID_NODE_TYPE_ERR )
            fprintf( stderr, "Exception: Invalid Node type\n" );
        return;
    }
    catch( DOMException d )
    {
        if( d.code == DOMException::NOT_FOUND_ERR )
            fprintf( stderr, "Exception: Null Nodes not accepted\n" );
        if( d.code == DOMException::INVALID_STATE_ERR )
            fprintf( stderr, "Exception: detach() has been invoked\n" );
        return;
    }
}

short Range::compareBoundaryPoints( CompareHow how, const Range &sourceRange )
{
    checkCommon();
    
    if( commonAncestorContainer().ownerDocument() != sourceRange.handle()->getCommonAncestorContainer().ownerDocument() )
        throw DOMException( DOMException::WRONG_DOCUMENT_ERR );

    return impl->compareBoundaryPoints(how,sourceRange);
}

bool Range::boundaryPointsValid(  )
{
    checkCommon();
    return impl->boundaryPointsValid();
}

void Range::deleteContents(  )
{
    if (impl) impl->deleteContents();
}

DocumentFragment Range::extractContents(  )
{
    checkCommon();
    return impl->extractContents();
}

DocumentFragment Range::cloneContents(  )
{
    checkCommon();
    return impl->cloneContents();
}

void Range::insertNode( const Node &newNode )
{
    checkCommon();

    if( newNode.nodeType() == Node::ATTRIBUTE_NODE ||
        newNode.nodeType() == Node::ENTITY_NODE ||
        newNode.nodeType() == Node::NOTATION_NODE ||
        newNode.nodeType() == Node::DOCUMENT_NODE ||
        newNode.nodeType() == Node::DOCUMENT_FRAGMENT_NODE)
        throw RangeException( RangeException::INVALID_NODE_TYPE_ERR);

    if( newNode.ownerDocument() != startContainer().ownerDocument() )
        throw DOMException( DOMException::WRONG_DOCUMENT_ERR );

    impl->insertNode(newNode);
}

void Range::surroundContents( const Node &newParent )
{
    checkCommon();

    if( newParent.isNull() )
        return;

    Node start = startContainer();
    if( newParent.ownerDocument() != start.ownerDocument() )
        throw DOMException( DOMException::WRONG_DOCUMENT_ERR );
    
    if( newParent.nodeType() == Node::ATTRIBUTE_NODE ||
        newParent.nodeType() == Node::ENTITY_NODE ||
        newParent.nodeType() == Node::NOTATION_NODE ||
        newParent.nodeType() == Node::DOCUMENT_TYPE_NODE ||
        newParent.nodeType() == Node::DOCUMENT_NODE ||
        newParent.nodeType() == Node::DOCUMENT_FRAGMENT_NODE)
        throw RangeException( RangeException::INVALID_NODE_TYPE_ERR );

    // revisit: if you set a range without optimizing it (trimming) the following exception might be
    // thrown incorrectly
    Node realStart = (start.nodeType() == Node::TEXT_NODE)? start.parentNode() : start;
    Node end = endContainer();
    Node realEnd = (end.nodeType() == Node::TEXT_NODE)? end.parentNode() : end;
    if( realStart != realEnd )
        throw RangeException( RangeException::BAD_BOUNDARYPOINTS_ERR );

    DocumentFragment fragment = extractContents();
    insertNode( newParent );
    // BIC: to avoid this const_cast newParent shouldn't be const
    //(const_cast<Node>(newParent)).appendChild( fragment );
    ((Node)(newParent)).appendChild( fragment );
    selectNode( newParent );
}

Range Range::cloneRange(  )
{
    if( isDetached() )
        throw DOMException( DOMException::INVALID_STATE_ERR );

    return Range( impl );
}

DOMString Range::toString(  )
{
    checkCommon();
    return impl->toString();
}

DOMString Range::toHTML(  )
{
    checkCommon();
    return impl->toHTML();
}

void Range::detach(  )
{
    if (impl) impl->detach(); // ### get rid of impl instead? (thereby becoming null)
}

bool Range::isDetached() const
{
    if (impl) return impl->isDetached();
    return true;
}

RangeImpl *Range::handle() const
{
    return impl;
}

bool Range::isNull() const
{
    return (impl == 0);
}

void Range::checkCommon() const
{
    if( !impl )
	throw DOMException::NO_MODIFICATION_ALLOWED_ERR;
    
    if( isDetached() )
        throw DOMException( DOMException::INVALID_STATE_ERR );
}

void Range::checkNode( DOM::Node n ) const
{
    checkCommon();
    
    Node _tempNode = n;
    if( _tempNode.isNull() )
        throw DOMException( DOMException::NOT_FOUND_ERR );

    while( !_tempNode.isNull() )
    {
        if( _tempNode.nodeType() == Node::ATTRIBUTE_NODE ||
            _tempNode.nodeType() == Node::ENTITY_NODE ||
            _tempNode.nodeType() == Node::NOTATION_NODE ||
            _tempNode.nodeType() == Node::DOCUMENT_TYPE_NODE )
            throw RangeException( RangeException::INVALID_NODE_TYPE_ERR );

        _tempNode = _tempNode.parentNode();
    }
}

void Range::checkNodeWOffset( DOM::Node n, int offset) const
{
    checkNode( n );
    
    if( offset < 0 )
        throw DOMException( DOMException::INDEX_SIZE_ERR );

    if( n.nodeType() != Node::TEXT_NODE )
    {
	if( (unsigned int)offset > n.childNodes().length() )
            throw DOMException( DOMException::INDEX_SIZE_ERR );
    }
    else
    {
	Text t;
	t = n;
        if( t.isNull() || (unsigned)offset > t.length() )
            throw DOMException( DOMException::INDEX_SIZE_ERR );
    }
}

void Range::checkNodeBA( DOM::Node n ) const
{
    checkNode( n );

    if( n.nodeType() == Node::DOCUMENT_NODE ||
        n.nodeType() == Node::DOCUMENT_FRAGMENT_NODE ||
        n.nodeType() == Node::ATTRIBUTE_NODE ||
        n.nodeType() == Node::ENTITY_NODE ||
        n.nodeType() == Node::NOTATION_NODE )
        throw RangeException( RangeException::INVALID_NODE_TYPE_ERR );

}
// ---------------------------------------------------------------
/*
DocumentRange::DocumentRange()
    : Document()
{
}

DocumentRange::DocumentRange(const DocumentRange &other)
    : Document(other)
{
}

DocumentRange &DocumentRange::operator = (const DocumentRange &other)
{
    Document::operator = (other);
    return *this;
}

DocumentRange::~DocumentRange()
{
}

//Range DocumentRange::createRange(  )
//{
//}

*/

