/**
 * This file is part of the DOM implementation for KDE.
 *
 * (C) 1999 Lars Knoll (knoll@kde.org)
 * (C) 2000 Frederik Holljen (frederik.holljen@hig.no)
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

#include "dom2_traversal.h"
#include "dom_node.h"
#include "dom_exception.h"
#include "dom2_traversalimpl.h"

using namespace DOM;

NodeIteratorImpl::NodeIteratorImpl()
{
  filter = 0;
}

NodeIteratorImpl::NodeIteratorImpl(const NodeIteratorImpl &other)
    : DomShared(other)
{
  referenceNode = other.referenceNode;
  rootNode = other.rootNode;
  whatToShow = other.whatToShow;
  filter = other.filter;
  inFront = other.inFront;
  expandEntityReferences = other.expandEntityReferences;
}

NodeIteratorImpl::NodeIteratorImpl(Node n, NodeFilter *f)
{
  if( !n.isNull() )
    {
      rootNode = n;
      referenceNode = n;
      whatToShow = 0x0000FFFF;
      filter = f;
    }
//   else
//     throw DOMException(DOMException::NOT_FOUND_ERR); // ### should we go into an invalid state instead?
}

NodeIteratorImpl::NodeIteratorImpl(Node n, long _whatToShow , NodeFilter *f)
{
    filter = f;
    whatToShow = _whatToShow;
    referenceNode = n;
    rootNode = n;
}

NodeIteratorImpl &NodeIteratorImpl::operator = (const NodeIteratorImpl &other)
{
  referenceNode = other.referenceNode;
  rootNode = other.rootNode;
  whatToShow = other.whatToShow;
  filter = other.filter;
  inFront = other.inFront;
  expandEntityReferences = other.expandEntityReferences;
  return *this;
}

NodeIteratorImpl::~NodeIteratorImpl()
{
  if(filter)
    {
    delete filter;
    filter = 0;
    }
}

void NodeIteratorImpl::setWhatToShow(long _whatToShow)
{
  whatToShow = _whatToShow;
}

void NodeIteratorImpl::moveReferenceNode(Node n)
{
  if( !n.isNull() )
    {
      // cheching if they are in the same subtree??
      referenceNode = n;
      inFront = true;
    }
}

void NodeIteratorImpl::setReferenceNode(Node n)
{
  if( !n.isNull() )
    {
      rootNode = n;
      referenceNode = n;
      inFront = true;
    }
}

void NodeIteratorImpl::setFilter(NodeFilter *_filter)
{
  if(_filter != 0)
    {
      if( filter != 0 )
        delete filter;

      filter = _filter;
    }
}

void NodeIteratorImpl::setExpandEntityReferences(bool value)
{
  expandEntityReferences = value;
}

Node NodeIteratorImpl::getRoot()
{
    // ###
    return 0;
}

unsigned long NodeIteratorImpl::getWhatToShow()
{
    // ###
    return 0;
}

NodeFilter NodeIteratorImpl::getFilter()
{
    // ###
    return NodeFilter();
}

bool NodeIteratorImpl::getExpandEntityReferences()
{
    // ###
    return 0;
}

Node NodeIteratorImpl::nextNode(  )
{
  short _result;
  Node _tempCurrent = getNextNode(referenceNode);
  while( !_tempCurrent.isNull() )
    {
      _result =  isAccepted(_tempCurrent);

      if(_result == NodeFilter::FILTER_ACCEPT)
        {
          referenceNode = _tempCurrent;
          return referenceNode;
        }
      _tempCurrent = getNextNode(_tempCurrent);
    }

  return Node();
}

Node NodeIteratorImpl::getNextNode(Node n)
{
  /*  1. my first child
   *  2. my next sibling
   *  3. my parents sibling, or their parents sibling (loop)
   *  4. not found
   */

  if( n.isNull() )
    return n;

  inFront = true;

  if( n.hasChildNodes() )
    return n.firstChild();

  if( !n.nextSibling().isNull() )
    return n.nextSibling();

  if( rootNode == n)
     return Node();

  Node parent = n.parentNode();
  while( !parent.isNull() )
    {
      n = parent.nextSibling();
      if( !n.isNull() )
        return n;

      if( rootNode == parent )
        return Node();

      parent = parent.parentNode();
    }
  return Node();
}

Node NodeIteratorImpl::previousNode(  )
{
  short _result;
  Node _tempCurrent = getPreviousNode(referenceNode);
  while( !_tempCurrent.isNull() )
    {
      _result = isAccepted(_tempCurrent);
      if(_result == NodeFilter::FILTER_ACCEPT)
        {
          referenceNode = _tempCurrent;
          return referenceNode;
        }
      _tempCurrent = getPreviousNode(_tempCurrent);
    }

  return Node();
}

Node NodeIteratorImpl::getPreviousNode(Node n)
{
/* 1. my previous sibling.lastchild
 * 2. my previous sibling
 * 3. my parent
 */
  Node _tempCurrent;

  if( n.isNull() )
    return Node();

    inFront = false;

  _tempCurrent = n.previousSibling();
  if( !_tempCurrent.isNull() )
    {
      if( _tempCurrent.hasChildNodes() )
        {
          while( _tempCurrent.hasChildNodes() )
            _tempCurrent = _tempCurrent.lastChild();
          return _tempCurrent;
        }
      else
        return _tempCurrent;
    }


  if(n == rootNode)
    return Node();

  return n.parentNode();

}

void NodeIteratorImpl::detach()
{
}


void NodeIteratorImpl::deleteNode(Node n)
{
    if( n.isNull() )
        return;  // someone tried to delete a null node :)

    Node _tempDeleted = referenceNode;
    while( !_tempDeleted.isNull() && _tempDeleted != n) // did I get deleted, or one of my parents?
        _tempDeleted = _tempDeleted.parentNode();

    if( _tempDeleted.isNull() )  // someone that did consern me got deleted
        return;

    if( !inFront)
    {
        Node _next = getNextNode(_tempDeleted);
        if( !_next.isNull() )
            referenceNode = _next;
        else
        {
            inFront = false;
            referenceNode = getPreviousNode(_tempDeleted);
            return;
        }
    }
    referenceNode = getPreviousNode(_tempDeleted);
}

short NodeIteratorImpl::isAccepted(Node n)
{
  // if XML is implemented we have to check expandEntityRerefences in this function
  if( ( ( 1 << n.nodeType()-1) & whatToShow) != 0 )
    {
        if(filter)
            return filter->acceptNode(n);
        else
            return NodeFilter::FILTER_ACCEPT;
    }
    return NodeFilter::FILTER_SKIP;
}

// --------------------------------------------------------------


NodeFilterImpl::NodeFilterImpl()
{
}

NodeFilterImpl::~NodeFilterImpl()
{
}

short NodeFilterImpl::acceptNode(const Node &)
{
    return NodeFilter::FILTER_ACCEPT;
}

// --------------------------------------------------------------

TreeWalkerImpl::TreeWalkerImpl()
{
    filter = 0;
    whatToShow = 0x0000FFFF;
    expandEntityReferences = true;
}

TreeWalkerImpl::TreeWalkerImpl(const TreeWalkerImpl &other) : DomShared()
{
    expandEntityReferences = other.expandEntityReferences;
    filter = other.filter;
    whatToShow = other.whatToShow;
    currentNode = other.currentNode;
    rootNode = other.rootNode;
}

TreeWalkerImpl::TreeWalkerImpl(Node n, NodeFilter *f)
{
  currentNode = n;
  rootNode = n;
  whatToShow = 0x0000FFFF;
  filter = f;
}

TreeWalkerImpl::TreeWalkerImpl(Node n, long _whatToShow, NodeFilter *f)
{
  currentNode = n;
  rootNode = n;
  whatToShow = _whatToShow;
  filter = f;
}

TreeWalkerImpl &TreeWalkerImpl::operator = (const TreeWalkerImpl &other)
{
  expandEntityReferences = other.expandEntityReferences;
  filter = other.filter;
  whatToShow = other.whatToShow;
  currentNode = other.currentNode;
  return *this;
}

TreeWalkerImpl::~TreeWalkerImpl()
{
    if(filter)
      {
        delete filter;
        filter = 0;
      }
}





Node TreeWalkerImpl::getRoot()
{
    // ###
    return 0;
}

unsigned long TreeWalkerImpl::getWhatToShow()
{
    // ###
    return 0;
}

NodeFilter TreeWalkerImpl::getFilter()
{
    // ###
    return 0;
}

bool TreeWalkerImpl::getExpandEntityReferences()
{
    // ###
    return 0;
}

Node TreeWalkerImpl::getCurrentNode()
{
    return currentNode;
}

void TreeWalkerImpl::setWhatToShow(long _whatToShow)
{
  // do some testing wether this is an accepted value
  whatToShow = _whatToShow;
}

void TreeWalkerImpl::setFilter(NodeFilter *_filter)
{
  if(_filter)
    filter = _filter;
}

void TreeWalkerImpl::setExpandEntityReferences(bool value)
{
  expandEntityReferences = value;
}

void TreeWalkerImpl::setCurrentNode( const Node n )
{
    if( !n.isNull() )
    {
        rootNode = n;
        currentNode = n;
    }
//     else
//         throw( DOMException::NOT_SUPPORTED_ERR );
}

Node TreeWalkerImpl::parentNode(  )
{
    Node n = getParentNode(currentNode);
    if( !n.isNull() )
        currentNode = n;
    return n;
}


Node TreeWalkerImpl::firstChild(  )
{
    Node n = getFirstChild(currentNode);
    if( !n.isNull() )
        currentNode = n;
    return n;
}


Node TreeWalkerImpl::lastChild(  )
{
    Node n = getLastChild(currentNode);
    if( !n.isNull() )
        currentNode = n;
    return n;
}

Node TreeWalkerImpl::previousSibling(  )
{
    Node n = getPreviousSibling(currentNode);
    if( !n.isNull() )
        currentNode = n;
    return n;
}

Node TreeWalkerImpl::nextSibling(  )
{
    Node n = getNextSibling(currentNode);
    if( !n.isNull() )
        currentNode = n;
    return n;
}

Node TreeWalkerImpl::previousNode(  )
{
/* 1. my previous sibling.lastchild
 * 2. my previous sibling
 * 3. my parent
 */

    Node n = getPreviousSibling(currentNode);
    if( n.isNull() )
    {
        n = getParentNode(currentNode);
        if( !n.isNull() )      //parent
        {
            currentNode = n;
            return currentNode;
        }
        else                  // parent failed.. no previous node
            return Node();
    }

    Node child = getLastChild(n);
    if( !child.isNull() )     // previous siblings last child
    {
        currentNode = child;
        return currentNode;
    }
    else                      // previous sibling
    {
        currentNode = n;
        return currentNode;
    }
    return Node();            // should never get here!
}

Node TreeWalkerImpl::nextNode(  )
{
/*  1. my first child
 *  2. my next sibling
 *  3. my parents sibling, or their parents sibling (loop)
 *  4. not found
 */

    Node n = getFirstChild(currentNode);
    if( !n.isNull()  ) // my first child
    {
        currentNode = n;
        return n;
    }

    n = getNextSibling(currentNode); // my next sibling
    if( !n.isNull() )
    {
        currentNode = n;
        return currentNode;
    }
    Node parent = getParentNode(currentNode);
    while( !parent.isNull() ) // parents sibling
    {
        n = getNextSibling(parent);
        if( !n.isNull() )
        {
          currentNode = n;
          return currentNode;
        }
        else
            parent = getParentNode(parent);
    }
    return Node();
}

short TreeWalkerImpl::isAccepted(Node n)
{
    // if XML is implemented we have to check expandEntityRerefences in this function
  if( ( ( 1 << n.nodeType()-1 ) & whatToShow) != 0 )
    {
      if(filter)
        return filter->acceptNode(n);
      else
        return NodeFilter::FILTER_ACCEPT;
    }
  return NodeFilter::FILTER_SKIP;
}

Node TreeWalkerImpl::getParentNode(Node n)
{
     short _result = NodeFilter::FILTER_ACCEPT;

    if( n == rootNode /*|| n.isNull()*/ )
      return Node();

    Node _tempCurrent = n.parentNode();

    if( _tempCurrent.isNull() )
      return Node();

    _result = isAccepted(_tempCurrent );
    if(_result == NodeFilter::FILTER_ACCEPT)
      return _tempCurrent;       // match found

    return getParentNode(_tempCurrent);
}

Node TreeWalkerImpl::getFirstChild(Node n)
{
    short _result;

    if( n.isNull() || n.firstChild().isNull() )
        return Node();
    n = n.firstChild();

    _result = isAccepted(n);

    switch(_result)
    {
         case NodeFilter::FILTER_ACCEPT:
           return n;
           break;
        case NodeFilter::FILTER_SKIP:
          if( n.hasChildNodes() )
                return getFirstChild(n);
            else
                return getNextSibling(n);
            break;

        case NodeFilter::FILTER_REJECT:
            return getNextSibling(n);
            break;
    }
    return Node();      // should never get here!
}

Node TreeWalkerImpl::getLastChild(Node n)
{
    short _result;

    if( n.isNull() || n.lastChild().isNull() )
        return Node();
    n = n.lastChild();
    _result = isAccepted(n);
    switch(_result)
    {
        case NodeFilter::FILTER_ACCEPT:
            return n;
            break;

        case NodeFilter::FILTER_SKIP:
            if( n.hasChildNodes() )
                return getLastChild(n);
            else
                return getPreviousSibling(n);
            break;

        case NodeFilter::FILTER_REJECT:
            return getPreviousSibling(n);
            break;
    }
    return Node();
}

Node TreeWalkerImpl::getPreviousSibling(Node n)
{
    short _result;
    Node _tempCurrent;

    if( n.isNull() )
        return Node();
    //first the cases if we have a previousSibling
    _tempCurrent = n.previousSibling();
    if( !_tempCurrent.isNull() )
    {
        _result = isAccepted(_tempCurrent);
        switch(_result)
        {
            case NodeFilter::FILTER_ACCEPT:
                return _tempCurrent;
                break;

            case NodeFilter::FILTER_SKIP:
            {
                Node nskip = getLastChild(_tempCurrent);
                if( !nskip.isNull() )
                    return nskip;
                return getPreviousSibling(_tempCurrent);
                break;
            }

            case NodeFilter::FILTER_REJECT:
                return getPreviousSibling(_tempCurrent);
                break;
        }
    }
    // now the case if we don't have previous sibling
    else
    {
        _tempCurrent = _tempCurrent.parentNode();
        if(_tempCurrent.isNull() || _tempCurrent == rootNode)
            return Node();
        _result = isAccepted(_tempCurrent);
        if(_result == NodeFilter::FILTER_SKIP)
            return getPreviousSibling(_tempCurrent);

        return Node();

    }
    return Node();  // should never get here!
}

Node TreeWalkerImpl::getNextSibling(Node n)
{
 short _result;
    Node _tempCurrent;

    if( n.isNull() || _tempCurrent == rootNode)
        return Node();

    _tempCurrent = n.nextSibling();
    if( !_tempCurrent.isNull() )
    {
        _result = isAccepted(_tempCurrent);
        switch(_result)
        {
            case NodeFilter::FILTER_ACCEPT:
                return _tempCurrent;
                break;

            case NodeFilter::FILTER_SKIP:
            {
                Node nskip = getFirstChild(_tempCurrent);
                if( !nskip.isNull() )
                    return nskip;
                return getNextSibling(_tempCurrent);
                break;
            }

            case NodeFilter::FILTER_REJECT:
                return getNextSibling(_tempCurrent);
                break;
        }
    }
    else
    {
        _tempCurrent = _tempCurrent.parentNode();
        if(_tempCurrent.isNull() || _tempCurrent == rootNode)
            return Node();
        _result = isAccepted(_tempCurrent);
        if(_result == NodeFilter::FILTER_SKIP)
            return getNextSibling(_tempCurrent);

        return Node();
    }
    return Node();
}

