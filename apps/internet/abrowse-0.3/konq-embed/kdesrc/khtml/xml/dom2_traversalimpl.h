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

#ifndef _DOM2_TraversalImpl_h_
#define _DOM2_TraversalImpl_h_

#include "dom_nodeimpl.h"
#include "dom/dom_node.h"
#include "dom/dom_misc.h"
#include "dom2_traversal.h"

namespace DOM {

class NodeIteratorImpl : public DomShared
{
public:
    NodeIteratorImpl();
    NodeIteratorImpl(const NodeIteratorImpl &other);
    NodeIteratorImpl(Node n, NodeFilter *f=0);
    NodeIteratorImpl(Node n, long _whatToShow, NodeFilter *f=0);

    NodeIteratorImpl & operator = (const NodeIteratorImpl &other);

    ~NodeIteratorImpl();


    Node getRoot();
    unsigned long getWhatToShow();
    NodeFilter getFilter();
    bool getExpandEntityReferences();

    Node nextNode();
    Node previousNode();

    void detach();


    // ### remove "get" from attribute read methods


    void setWhatToShow(long _whatToShow);
    void setFilter(NodeFilter *_filter);
    void setExpandEntityReferences(bool value);

    /**
     * This function has to be called if you delete a node from the
     * document tree and you want the Iterator to react if there
     * are any changes concerning it.
     */
    void deleteNode(Node n);

    /**
     *  Move the Iterators referenceNode within the subtree. Does not set a new root node.
     */
    void moveReferenceNode(Node n);
    /**
     * Sets the iterators referenceNode and sets a new root node
     */
    void setReferenceNode(Node n);
    short isAccepted(Node n);
    Node getNextNode(Node n);
    Node getPreviousNode(Node n);
 protected:

    /**
     * This attribute determines which node types are presented via
     * the Iterator.
     *
     */
    long whatToShow;

    /**
     * The filter used to screen nodes.
     *
     */
    NodeFilter *filter;

    /**
     * The value of this flag determines whether entity reference
     * nodes are expanded. To produce a view of the document that has
     * entity references expanded and does not expose the entity
     * reference node itself, use the whatToShow flags to hide the
     * entity reference node and set expandEntityReferences to true
     * when creating the iterator. To produce a view of the document
     * that has entity reference nodes but no entity expansion, use
     * the whatToShow flags to show the entity reference node and set
     * expandEntityReferences to true.
     *
     */
    bool expandEntityReferences;

    /**
     * internal, used to determine if the iterator is in front or to the back
     * of the referenceNode
     */
    bool inFront;
    /**
     * Internal, not specified by the dom
     * The current referenceNode
     */
    Node referenceNode;

    /**
     * Internal, not specified by the dom
     * The initial referenceNode for this Iterator
     */
    Node rootNode;
};

class NodeFilterImpl : public DomShared
{
public:
    NodeFilterImpl();
    ~NodeFilterImpl();

    virtual short acceptNode(const Node &n);
};

class TreeWalkerImpl : public DomShared
{
public:
    TreeWalkerImpl();
    TreeWalkerImpl(const TreeWalkerImpl &other);
    TreeWalkerImpl(Node n, NodeFilter *f=0);
    TreeWalkerImpl(Node n, long _whatToShow, NodeFilter *f=0);
    TreeWalkerImpl & operator = (const TreeWalkerImpl &other);


    ~TreeWalkerImpl();

    Node getRoot();

    unsigned long getWhatToShow();

    NodeFilter getFilter();

    bool getExpandEntityReferences();

    Node getCurrentNode();

    void setCurrentNode(const Node _currentNode);

    Node parentNode();

    Node firstChild();

    Node lastChild ();

    Node previousSibling ();

    Node nextSibling();

    Node previousNode();

    Node nextNode();


    /**
     * Sets which node types are to be presented via the TreeWalker
     */
    void setWhatToShow(long _whatToShow);
    void setFilter(NodeFilter *_filter);
    void setExpandEntityReferences(bool value);

    Node getParentNode(Node n);
    Node getFirstChild(Node n);
    Node getLastChild(Node n);
    Node getPreviousSibling(Node n);
    Node getNextSibling(Node n);

    short isAccepted(Node n);

protected:
    /**
     * This attribute determines which node types are presented via
     * the TreeWalker.
     *
     */
    long whatToShow;

    /**
     * The filter used to screen nodes.
     *
     */
    NodeFilter *filter;

    /**
     * The value of this flag determines whether entity reference
     * nodes are expanded. To produce a view of the document that has
     * entity references expanded and does not expose the entity
     * reference node itself, use the whatToShow flags to hide the
     * entity reference node and set expandEntityReferences to true
     * when creating the iterator. To produce a view of the document
     * that has entity reference nodes but no entity expansion, use
     * the whatToShow flags to show the entity reference node and set
     * expandEntityReferences to true.
     *
     * This is not implemented (allways true)
     */
    bool expandEntityReferences;

    /**
     * The current node.
     *
     *  The value must not be null. Attempting to set it to null will
     * raise a NOT_SUPPORTED_ERR exception. When setting a node, the
     * whatToShow flags and any Filter associated with the TreeWalker
     * are not checked. The currentNode may be set to any Node of any
     * type.
     *
     */
    Node currentNode;

    Node rootNode;
};


}; // namespace

#endif

