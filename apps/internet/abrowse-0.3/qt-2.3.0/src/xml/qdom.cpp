/****************************************************************************
** $Id$
**
** Implementation of QDomDocument and related classes.
**
** Created : 000518
**
** Copyright (C) 1992-2000 Trolltech AS.  All rights reserved.
**
** This file is part of the XML module of the Qt GUI Toolkit.
**
** This file may be distributed under the terms of the Q Public License
** as defined by Trolltech AS of Norway and appearing in the file
** LICENSE.QPL included in the packaging of this file.
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** Licensees holding valid Qt Enterprise Edition licenses may use this
** file in accordance with the Qt Commercial License Agreement provided
** with the Software.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.trolltech.com/pricing.html or email sales@trolltech.com for
**   information about Qt Commercial License Agreements.
** See http://www.trolltech.com/qpl/ for QPL licensing information.
** See http://www.trolltech.com/gpl/ for GPL licensing information.
**
** Contact info@trolltech.com if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

#include "qdom.h"

#ifndef QT_NO_DOM

#include "qxml.h"
#include "qmap.h"
#include "qtextstream.h"
#include "qiodevice.h"
#include "qpixmap.h"

#include <string.h>
#include <stdlib.h>

#if defined(_OS_LINUX_)
#  if defined(__alpha__) || defined(__alpha)
#    define Q_BROKEN_ALPHA
#  endif
#endif

// template class QDict<QDOM_NodePrivate>;

// NOT REVISED

/**
 * TODO:
 * If the document dies, remove all pointers to it from children
 * which can not be deleted at this time.
 *
 * If a node dies and has direct children which can not be deleted,
 * then remove the pointer to the parent.
 *
 * Handle QDomDocumentFragment on insertion correctly.
 *
 * createElement and friends create double reference counts.
 */

/**
 * Reference counting:
 *
 * Some simple rules:
 * 1) If an intern object returns a pointer to another intern object
 *    then the reference count of the returned object is not increased.
 * 2) If an extern object is created and gets a pointer to some intern
 *    object, then the extern object increases the intern objects reference count.
 * 3) If an extern object is deleted, then it decreases the reference count
 *    on its associated intern object and deletes it if nobody else hold references
 *    on the intern object.
 */

/**************************************************************
 *
 * QDOMHandler
 *
 **************************************************************/

class QDomHandler : public QXmlDefaultHandler
{
public:
    QDomHandler( QDOM_DocumentPrivate* d );
    ~QDomHandler();

    // content handler
    void setDocumentLocator( QXmlLocator* locator );
    bool endDocument();
    bool startElement( const QString& namespaceURI, const QString& localName, const QString& qName, const QXmlAttributes& atts );
    bool endElement( const QString& namespaceURI, const QString& localName, const QString& qName );
    bool characters( const QString& ch );
    bool processingInstruction( const QString& target, const QString& data );

    // error handler
    bool fatalError( const QXmlParseException& exception );

    // lexical handler
    bool startCDATA();
    bool endCDATA();
    bool startDTD( const QString& name, const QString&, const QString& );
    bool comment( const QString& ch );

    // decl handler
    bool externalEntityDecl( const QString &name, const QString &publicId, const QString &systemId ) ;

    // DTD handler
    bool notationDecl( const QString & name, const QString & publicId, const QString & systemId );
    bool unparsedEntityDecl( const QString &name, const QString &publicId, const QString &systemId, const QString &notationName ) ;

private:
    QXmlLocator* loc;
    QDOM_DocumentPrivate* doc;
    QDOM_NodePrivate* node;
    bool cdata;
};

/*==============================================================*/
/*                Implementation                                */
/*==============================================================*/

/**************************************************************
 *
 * QDOM_ImplementationPrivate
 *
 **************************************************************/

class QDOM_ImplementationPrivate : public QShared
{
public:
    QDOM_ImplementationPrivate();
    ~QDOM_ImplementationPrivate();

    QDOM_ImplementationPrivate* clone();

};

QDOM_ImplementationPrivate::QDOM_ImplementationPrivate()
{
}

QDOM_ImplementationPrivate::~QDOM_ImplementationPrivate()
{
}

QDOM_ImplementationPrivate* QDOM_ImplementationPrivate::clone()
{
    QDOM_ImplementationPrivate* p = new QDOM_ImplementationPrivate;
    // We are not interested in this node
    p->deref();
    return p;
}

/**************************************************************
 *
 * QDomImplementation
 *
 **************************************************************/

/*!
  \class QDomImplementation qdom.h
  \brief The QDomImplementation class provides information about the features
  of the DOM implementation.

  \module XML

  This class describes the features that are supported by the DOM
  implementation. Currently only the XML subset of DOM Level 1 is supported.

  Normally you will use the function QDomDocument::implementation() to get the
  implementation object.

  For further information about the Document Objct Model see
  <a href="http://www.w3.org/TR/REC-DOM-Level-1/">http://www.w3.org/TR/REC-DOM-Level-1/</a>.
  For a more general introduction of the DOM implementation see the
  QDomDocument documentation.

  \sa hasFeature()
*/

/*!
  Constructs a QDomImplementation object.
*/
QDomImplementation::QDomImplementation()
{
    impl = 0;
}

/*!
  Copy constructor.
*/
QDomImplementation::QDomImplementation( const QDomImplementation& x )
{
    impl = x.impl;
    if ( impl )
	impl->ref();
}

/*!
  \internal
*/
QDomImplementation::QDomImplementation( QDOM_ImplementationPrivate* p )
{
    // We want to be co-owners, so increase the reference count
    impl = p;
}

/*!
  Assignment operator.
*/
QDomImplementation& QDomImplementation::operator= ( const QDomImplementation& x )
{
    if ( x.impl )
	x.impl->ref();		//avoid x=x
    if ( impl && impl->deref() )
	delete impl;
    impl = x.impl;

    return *this;
}

/*!
  Returns TRUE if both objects were created from the same QDomDocument.
*/
bool QDomImplementation::operator==( const QDomImplementation& x ) const
{
    return ( impl == x.impl );
}

/*!
  Returns TRUE if both objects were created from different QDomDocuments.
*/
bool QDomImplementation::operator!=( const QDomImplementation& x ) const
{
    return ( impl != x.impl );
}

/*!
  Destructor.
*/
QDomImplementation::~QDomImplementation()
{
    if ( impl && impl->deref() )
	delete impl;
}

/*!
  The function returns TRUE if QDom implements the requested \a version of a \a
  feature.

  Currently only the feature "XML" in version "1.0" is supported.
*/
bool QDomImplementation::hasFeature( const QString& feature, const QString& version )
{
    if ( feature == "XML" )
	if ( version.isEmpty() || version == "1.0" )
	    return TRUE;

    return FALSE;
}

/*!
  Returns TRUE if the object was not created by QDomDocument::implementation().
*/
bool QDomImplementation::isNull()
{
    return ( impl == 0 );
}

/*==============================================================*/
/*                       NodeList                               */
/*==============================================================*/

/**************************************************************
 *
 * QDOM_NodeListPrivate
 *
 **************************************************************/

class QDOM_NodePrivate : public QShared
{
public:
    QDOM_NodePrivate( QDOM_DocumentPrivate*, QDOM_NodePrivate* parent = 0 );
    QDOM_NodePrivate( QDOM_NodePrivate* n, bool deep );
    virtual ~QDOM_NodePrivate();

    QString nodeName() const { return name; }
    QString nodeValue() const { return value; }
    void setNodeValue( const QString& v ) { value = v; }

    QDOM_DocumentPrivate*     ownerDocument();

    virtual QDOM_NamedNodeMapPrivate* attributes();
    virtual QDOM_NodePrivate* insertBefore( QDOM_NodePrivate* newChild, QDOM_NodePrivate* refChild );
    virtual QDOM_NodePrivate* insertAfter( QDOM_NodePrivate* newChild, QDOM_NodePrivate* refChild );
    virtual QDOM_NodePrivate* replaceChild( QDOM_NodePrivate* newChild, QDOM_NodePrivate* oldChild );
    virtual QDOM_NodePrivate* removeChild( QDOM_NodePrivate* oldChild );
    virtual QDOM_NodePrivate* appendChild( QDOM_NodePrivate* newChild );

    QDOM_NodePrivate* namedItem( const QString& name );

    virtual QDOM_NodePrivate* cloneNode( bool deep = TRUE );
    virtual void clear();

    void setParent( QDOM_NodePrivate* );

    // Dynamic cast
    virtual bool isAttr() { return FALSE; }
    virtual bool isCDATASection() { return FALSE; }
    virtual bool isDocumentFragment() { return FALSE; }
    virtual bool isDocument() { return FALSE; }
    virtual bool isDocumentType() { return FALSE; }
    virtual bool isElement() { return FALSE; }
    virtual bool isEntityReference() { return FALSE; }
    virtual bool isText() { return FALSE; }
    virtual bool isEntity() { return FALSE; }
    virtual bool isNotation() { return FALSE; }
    virtual bool isProcessingInstruction() { return FALSE; }
    virtual bool isCharacterData() { return FALSE; }
    virtual bool isComment() { return FALSE; }

    virtual void save( QTextStream&, int ) const;

    // Variables
    QDOM_NodePrivate* prev;
    QDOM_NodePrivate* next;
    QDOM_NodePrivate* parent;
    QDOM_NodePrivate* first;
    QDOM_NodePrivate* last;

    QString name;
    QString value;
};

class QDOM_NodeListPrivate : public QShared
{
public:
    QDOM_NodeListPrivate( QDOM_NodePrivate* );
    QDOM_NodeListPrivate( QDOM_NodePrivate*, const QString&  );
    virtual ~QDOM_NodeListPrivate();

    virtual bool operator== ( const QDOM_NodeListPrivate& ) const;
    virtual bool operator!= ( const QDOM_NodeListPrivate& ) const;

    virtual QDOM_NodePrivate* item( int index );
    virtual uint length() const;

    QDOM_NodePrivate* node_impl;
    QString tagname;
};

QDOM_NodeListPrivate::QDOM_NodeListPrivate( QDOM_NodePrivate* n_impl )
{
    node_impl = n_impl;
    if ( node_impl )
	node_impl->ref();
}

QDOM_NodeListPrivate::QDOM_NodeListPrivate( QDOM_NodePrivate* n_impl, const QString& name )
{
    node_impl = n_impl;
    if ( node_impl )
	node_impl->ref();
    tagname = name;
}

QDOM_NodeListPrivate::~QDOM_NodeListPrivate()
{
    if ( node_impl && node_impl->deref() )
	delete node_impl;
}

bool QDOM_NodeListPrivate::operator== ( const QDOM_NodeListPrivate& other ) const
{
    return ( node_impl == other.node_impl ) && ( tagname == other.tagname ) ;
}

bool QDOM_NodeListPrivate::operator!= ( const QDOM_NodeListPrivate& other ) const
{
    return ( node_impl != other.node_impl ) || ( tagname != other.tagname ) ;
}

QDOM_NodePrivate* QDOM_NodeListPrivate::item( int index )
{
    QDOM_NodePrivate* p = node_impl->first;
    int i = 0;
    if ( tagname.isNull() ) {
	while ( i < index && p ) {
	    p = p->next;
	    ++i;
	}
    } else {
	while ( p && p != node_impl ) {
	    if ( p->isElement() && p->nodeName() == tagname ) {
		if ( i == index )
		    break;
		++i;
	    }
	    if ( p->first )
		p = p->first;
	    else if ( p->next )
		p = p->next;
	    else {
		p = p->parent;
		while ( p && p != node_impl && !p->next )
		    p = p->parent;
		if ( p && p != node_impl )
		    p = p->next;
	    }
	}
    }

    return p;
}

uint QDOM_NodeListPrivate::length() const
{
    uint i = 0;
    QDOM_NodePrivate* p = node_impl->first;
    if ( tagname.isNull() ) {
	while ( p ) {
	    p = p->next;
	    ++i;
	}
    } else {
	while ( p && p != node_impl ) {
	    if ( p->isElement() && p->nodeName() == tagname )
		++i;

	    if ( p->first )
		p = p->first;
	    else if ( p->next )
		p = p->next;
	    else {
		p = p->parent;
		while ( p && p != node_impl && !p->next )
		    p = p->parent;
		if ( p && p != node_impl )
		    p = p->next;
	    }
	}
    }
    return i;
}

/**************************************************************
 *
 * QDomNodeList
 *
 **************************************************************/

/*!
  \class QDomNodeList qdom.h
  \brief The QDomNodeList class is a list of QDomNode objects.

  \module XML

  Lists can be obtained by QDomDocument::elementsByTagName() and
  QDomNode::childNodes(). The Document Object Model (DOM) requires these lists
  to be "live": whenever you change the underlying document, the contents of
  the list will get updated.

  For further information about the Document Objct Model see
  <a href="http://www.w3.org/TR/REC-DOM-Level-1/">http://www.w3.org/TR/REC-DOM-Level-1/</a>.
  For a more general introduction of the DOM implementation see the
  QDomDocument documentation.

  \sa QDomNode::childNode() QDomDocument::elementsByTagName()
*/

/*!
  Creates an empty node list.
*/
QDomNodeList::QDomNodeList()
{
    impl = 0;
}

/*! \internal
*/
QDomNodeList::QDomNodeList( QDOM_NodeListPrivate* p )
{
    impl = p;
}

/*!
  Copy constructor.
*/
QDomNodeList::QDomNodeList( const QDomNodeList& n )
{
    impl = n.impl;
    if ( impl )
	impl->ref();
}

/*!
  Assigns another node list to this object.
*/
QDomNodeList& QDomNodeList::operator= ( const QDomNodeList& n )
{
    if ( n.impl )
	n.impl->ref();
    if ( impl && impl->deref() )
	delete impl;
    impl = n.impl;

    return *this;
}

/*!
  Returns TRUE if both lists are equal, otherwise FALSE.
*/
bool QDomNodeList::operator== ( const QDomNodeList& n ) const
{
    if ( impl == n.impl )
	return TRUE;
    if ( !impl || !n.impl )
	return FALSE;
    return (*impl == *n.impl);
}

/*!
  Returns TRUE if both lists are not equal, otherwise FALSE.
*/
bool QDomNodeList::operator!= ( const QDomNodeList& n ) const
{
    return !operator==(n);
}

/*!
  Destructor.
*/
QDomNodeList::~QDomNodeList()
{
    if ( impl && impl->deref() )
	delete impl;
}

/*!
  Returns the node at position \a index.

  If \a index is negative or if \a index >= length() then a null node is
  returned (i.e. a node for which QDomNode::isNull() returns TRUE).
*/
QDomNode QDomNodeList::item( int index ) const
{
    if ( !impl )
	return QDomNode();

    return QDomNode( impl->item( index ) );
}

/*!
  Returns the number of nodes in the list.

  This function is the same as count().
*/
uint QDomNodeList::length() const
{
    if ( !impl )
	return 0;
    return impl->length();
}

/*!
  \fn uint QDomNodeList::count() const

  Returns the number of nodes in the list.

  This function is the same as length().
*/


/*==============================================================*/
/*==============================================================*/

/**************************************************************
 *
 * QDOM_NodePrivate
 *
 **************************************************************/

QDOM_NodePrivate::QDOM_NodePrivate( QDOM_DocumentPrivate* /* qd */, QDOM_NodePrivate *par )
{
    parent = par;
    prev = 0;
    next = 0;
    first = 0;
    last = 0;
}

QDOM_NodePrivate::QDOM_NodePrivate( QDOM_NodePrivate* n, bool deep )
{
    parent = 0;
    prev = 0;
    next = 0;
    first = 0;
    last = 0;

    name = n->name;
    value = n->value;

    if ( !deep )
	return;

    for ( QDOM_NodePrivate* x = n->first; x; x = x->next )
	appendChild( x->cloneNode( TRUE ) );
}

QDOM_NodePrivate::~QDOM_NodePrivate()
{
    QDOM_NodePrivate* p = first;
    QDOM_NodePrivate* n;

    while ( p ) {
	n = p->next;
	if ( p->deref() )
	    delete p;
	else
	    p->parent = 0;
	p = n;
    }

    first = 0;
    last = 0;
}

void QDOM_NodePrivate::clear()
{
    QDOM_NodePrivate* p = first;
    QDOM_NodePrivate* n;

    while ( p ) {
	n = p->next;
	if ( p->deref() )
	    delete p;
	p = n;
    }

    first = 0;
    last = 0;
}

QDOM_NodePrivate* QDOM_NodePrivate::namedItem( const QString& n )
{
    QDOM_NodePrivate* p = first;
    while ( p ) {
	if ( p->nodeName() == n )
	    return p;
	p = p->next;
    }

    return 0;
}

QDOM_NamedNodeMapPrivate* QDOM_NodePrivate::attributes()
{
    return 0;
}

QDOM_NodePrivate* QDOM_NodePrivate::insertBefore( QDOM_NodePrivate* newChild, QDOM_NodePrivate* refChild )
{
    // Error check
    if ( !newChild )
	return 0;

  // Error check
    if ( newChild == refChild )
	return 0;

  // Error check
    if ( refChild && refChild->parent != this )
	return 0;

  // Special handling for inserting a fragment. We just insert
  // all elements of the fragment instead of the fragment itself.
    if ( newChild->isDocumentFragment() ) {
	// Fragment is empty ?
	if ( newChild->first == 0 )
	    return newChild;

	// New parent
	QDOM_NodePrivate* n = newChild->first;
	while ( n )  {
	    n->parent = this;
	    n = n->next;
	}

	// Insert at the beginning ?
	if ( !refChild || refChild->prev == 0 ) {
	    if ( first )
		first->prev = newChild->last;
	    newChild->last->next = first;
	    if ( !last )
		last = newChild->last;
	    first = newChild->first;
	} else {// Insert in the middle
	    newChild->last->next = refChild;
	    newChild->first->prev = refChild->prev;
	    refChild->prev->next = newChild->first;
	    refChild->prev = newChild->last;
	}

	// No need to increase the reference since QDomDocumentFragment
	// does not decrease the reference.

	// Remove the nodes from the fragment
	newChild->first = 0;
	newChild->last = 0;
	return newChild;
    }

    // No more errors can occure now, so we take
    // ownership of the node.
    newChild->ref();

    if ( newChild->parent )
	newChild->parent->removeChild( newChild );

    newChild->parent = this;

    if ( !refChild ) {
	if ( first )
	    first->prev = newChild;
	newChild->next = first;
	if ( !last )
	    last = newChild;
	first = newChild;
	return newChild;
    }

    if ( refChild->prev == 0 ) {
	if ( first )
	    first->prev = newChild;
	newChild->next = first;
	if ( !last )
	    last = newChild;
	first = newChild;
	return newChild;
    }

    newChild->next = refChild;
    newChild->prev = refChild->prev;
    refChild->prev->next = newChild;
    refChild->prev = newChild;

    return newChild;
}

QDOM_NodePrivate* QDOM_NodePrivate::insertAfter( QDOM_NodePrivate* newChild, QDOM_NodePrivate* refChild )
{
    // Error check
    if ( !newChild )
	return 0;

  // Error check
    if ( newChild == refChild )
	return 0;

  // Error check
    if ( refChild && refChild->parent != this )
	return 0;

  // Special handling for inserting a fragment. We just insert
  // all elements of the fragment instead of the fragment itself.
    if ( newChild->isDocumentFragment() ) {
	// Fragment is empty ?
	if ( newChild->first == 0 )
	    return newChild;

	// New parent
	QDOM_NodePrivate* n = newChild->first;
	while ( n ) {
	    n->parent = this;
	    n = n->next;
	}

	// Insert at the end
	if ( !refChild || refChild->next == 0 ) {
	    if ( last )
		last->next = newChild->first;
	    newChild->first->prev = last;
	    if ( !first )
		first = newChild->first;
	    last = newChild->last;
	} else { // Insert in the middle
	    newChild->first->prev = refChild;
	    newChild->last->next = refChild->next;
	    refChild->next->prev = newChild->last;
	    refChild->next = newChild->first;
	}

	// No need to increase the reference since QDomDocumentFragment
	// does not decrease the reference.

	// Remove the nodes from the fragment
	newChild->first = 0;
	newChild->last = 0;
	return newChild;
    }

    // Release new node from its current parent
    if ( newChild->parent )
	newChild->parent->removeChild( newChild );

  // No more errors can occure now, so we take
  // ownership of the node
    newChild->ref();

    newChild->parent = this;

  // Insert at the end
    if ( !refChild ) {
	if ( last )
	    last->next = newChild;
	newChild->prev = last;
	if ( !first )
	    first = newChild;
	last = newChild;
	return newChild;
    }

    if ( refChild->next == 0 ) {
	if ( last )
	    last->next = newChild;
	newChild->prev = last;
	if ( !first )
	    first = newChild;
	last = newChild;
	return newChild;
    }

    newChild->prev = refChild;
    newChild->next = refChild->next;
    refChild->next->prev = newChild;
    refChild->next = newChild;

    return newChild;
}

QDOM_NodePrivate* QDOM_NodePrivate::replaceChild( QDOM_NodePrivate* newChild, QDOM_NodePrivate* oldChild )
{
    // Error check
    if ( oldChild->parent != this )
	return 0;

  // Error check
    if ( !newChild || !oldChild )
	return 0;

  // Error check
    if ( newChild == oldChild )
	return 0;

    // Special handling for inserting a fragment. We just insert
    // all elements of the fragment instead of the fragment itself.
    if ( newChild->isDocumentFragment() ) {
	// Fragment is empty ?
	if ( newChild->first == 0 )
	    return newChild;

	// New parent
	QDOM_NodePrivate* n = newChild->first;
	while ( n ) {
	    n->parent = this;
	    n = n->next;
	}


	if ( oldChild->next )
	    oldChild->next->prev = newChild->last;
	if ( oldChild->prev )
	    oldChild->prev->next = newChild->first;

	newChild->last->next = oldChild->next;
	newChild->first->prev = oldChild->prev;

	if ( first == oldChild )
	    first = newChild->first;
	if ( last == oldChild )
	    last = newChild->last;

	oldChild->parent = 0;
	oldChild->next = 0;
	oldChild->prev = 0;

	// No need to increase the reference since QDomDocumentFragment
	// does not decrease the reference.

	// Remove the nodes from the fragment
	newChild->first = 0;
	newChild->last = 0;

	// We are no longer interested in the old node
	if ( oldChild ) oldChild->deref();

	return oldChild;
    }

    // No more errors can occure now, so we take
    // ownership of the node
    newChild->ref();

  // Release new node from its current parent
    if ( newChild->parent )
	newChild->parent->removeChild( newChild );

    newChild->parent = this;

    if ( oldChild->next )
	oldChild->next->prev = newChild;
    if ( oldChild->prev )
	oldChild->prev->next = newChild;

    newChild->next = oldChild->next;
    newChild->prev = oldChild->prev;

    if ( first == oldChild )
	first = newChild;
    if ( last == oldChild )
	last = newChild;

    oldChild->parent = 0;
    oldChild->next = 0;
    oldChild->prev = 0;

    // We are no longer interested in the old node
    if ( oldChild ) oldChild->deref();

    return oldChild;
}

QDOM_NodePrivate* QDOM_NodePrivate::removeChild( QDOM_NodePrivate* oldChild )
{
    // Error check
    if ( oldChild->parent != this )
	return 0;

  // Perhaps oldChild was just created with "createElement" or that. In this case
  // its parent is QDomDocument but it is not part of the documents child list.
    if ( oldChild->next == 0 && oldChild->prev == 0 && first != oldChild )
	return 0;

    if ( oldChild->next )
	oldChild->next->prev = oldChild->prev;
    if ( oldChild->prev )
	oldChild->prev->next = oldChild->next;

    if ( last == oldChild )
	last = oldChild->prev;
    if ( first == oldChild )
	first = oldChild->next;

    oldChild->parent = 0;
    oldChild->next = 0;
    oldChild->prev = 0;

    // We are no longer interested in the old node
    if ( oldChild ) oldChild->deref();

    return oldChild;
}

QDOM_NodePrivate* QDOM_NodePrivate::appendChild( QDOM_NodePrivate* newChild )
{
    // No reference manipulation needed. Done in insertAfter.
    return insertAfter( newChild, 0 );
}

void QDOM_NodePrivate::setParent( QDOM_NodePrivate* n )
{
    // Dont take over ownership of our parent :-)
    parent = n;
}

QDOM_DocumentPrivate* QDOM_NodePrivate::ownerDocument()
{
    QDOM_NodePrivate* p = this;
    while ( p && p->isDocumentType() )
	p = p->parent;

    return (QDOM_DocumentPrivate*)p;
}

QDOM_NodePrivate* QDOM_NodePrivate::cloneNode( bool deep )
{
    QDOM_NodePrivate* p = new QDOM_NodePrivate( this, deep );
    // We are not interested in this node
    p->deref();
    return p;
}

void QDOM_NodePrivate::save( QTextStream& s, int indent ) const
{
    const QDOM_NodePrivate* n = first;
    while ( n ) {
	n->save( s, indent );
	n = n->next;
    }
}

/**************************************************************
 *
 * QDomNode
 *
 **************************************************************/

#define IMPL ((QDOM_NodePrivate*)impl)

/*!
  \class QDomNode qdom.h
  \brief The QDomNode class is the base class for all nodes of the DOM tree.

  \module XML

  This class is the base class for almost all other classes in the DOM. Many
  functions in the DOM return a QDomNode. The various isXxx() functions are
  useful to find out the type of the node. A QDomNode can be converted to a
  subclass by using the toXxx() function family.

  Copies of the QDomNode class share their data; this means modifying one will
  change all copies. This is especially useful in combination with functions
  which return a QDomNode, e.g. firstChild(). You can make an independent copy
  of the node with cloneNode().

  The following example looks for the first element in an XML document and
  prints its name:
  \code
  QDomDocument d;
  d.setContent( someXML );
  QDomNode n = d.firstChild();
  while ( !n.isNull() ) {
      if ( n.isElement ) {
          QDomElement e = n.toElement();
          cout << "The name of the element is " << e.tagName() << endl;
	  return;
      }
      n = n.nextSibling();
  }
  cout << "no element in the Document" << endl;
  \endcode

  For further information about the Document Objct Model see
  <a href="http://www.w3.org/TR/REC-DOM-Level-1/">http://www.w3.org/TR/REC-DOM-Level-1/</a>.
  For a more general introduction of the DOM implementation see the
  QDomDocument documentation.
*/

/*!
  Constructs an empty node.
*/
QDomNode::QDomNode()
{
    impl = 0;
}

/*!
  Copy constructor.

  The data of the copy is shared: modifying one will also change the other. If
  you want to make a real copy, use cloneNode() instead.
*/
QDomNode::QDomNode( const QDomNode& n )
{
    impl = n.impl;
    if ( impl ) impl->ref();
}

/*!
  \internal
*/
QDomNode::QDomNode( QDOM_NodePrivate* n )
{
    impl = n;
    if ( impl ) impl->ref();
}

/*!
  Assignment operator.

  The data of the copy is shared: modifying one will also change the other. If
  you want to make a real copy, use cloneNode() instead.
*/
QDomNode& QDomNode::operator= ( const QDomNode& n )
{
    if ( n.impl ) n.impl->ref();
    if ( impl && impl->deref() ) delete impl;
    impl = n.impl;

    return *this;
}

/*!
  Returns TRUE if the two nodes are equal, otherwise FALSE.
*/
bool QDomNode::operator== ( const QDomNode& n ) const
{
    return ( impl == n.impl );
}

/*!
  Returns TRUE if the two nodes are not equal, otherwise FALSE.
*/
bool QDomNode::operator!= ( const QDomNode& n ) const
{
    return ( impl != n.impl );
}

/*!
  Destructor.
*/
QDomNode::~QDomNode()
{
    if ( impl && impl->deref() ) delete impl;
}

/*!
  Returns the name of the node.

  The meaning of the name depends on the subclass:

  <ul>
  <li> QDomElement - the tag name
  <li> QDomAttr - the name of the attribute
  <li> QDomText - the string "#text"
  <li> QDomCDATASection - the string "#cdata-section"
  <li> QDomEntityReference - the name of the referenced entity
  <li> QDomEntity - the name of the entity
  <li> QDomProcessingInstruction - the target of the processing instruction
  <li> QDomDocument - the string "#document"
  <li> QDomComment - the string "#comment"
  <li> QDomDocumentType - the name of the document type
  <li> QDomDocumentFragment - the string "#document-fragment"
  <li> QDomNotation - the name of the notation
  </ul>

  \sa nodeValue()
*/
QString QDomNode::nodeName() const
{
    if ( !impl )
	return QString::null;
    return IMPL->name;
}

/*!
  Returns the value of the node.

  The meaning of the value depends on the subclass:

  <ul>
  <li> QDomAttr - the attribute value
  <li> QDomText - the text
  <li> QDomCDATASection - the content of the CDATA section
  <li> QDomProcessingInstruction - the data of the processing intruction
  <li> QDomComment - the comment
  </ul>

  All other subclasses not listed above do not have a node value. These classes
  will return a null string.

  \sa setNodeValue() nodeName()
*/
QString QDomNode::nodeValue() const
{
    if ( !impl )
	return QString::null;
    return IMPL->value;
}

/*!
  Sets the value of the node to \a v.

  \sa nodeValue()
*/
void QDomNode::setNodeValue( const QString& v )
{
    if ( !impl )
	return;
    IMPL->setNodeValue( v );
}

/*!
  Returns the type of the node.

  \sa toAttr() toCDATASection() toDocumentFragment() toDocument()
  toDocumentType() toElement() toEntityReference() toText() toEntity()
  toNotation() toProcessingInstruction() toCharacterData() toComment() 
*/
QDomNode::NodeType QDomNode::nodeType() const
{
    // not very efficient, but will do for the moment.
    if( isCDATASection() )
	return CDATASectionNode;
    if ( isText() )
	return TextNode;
    if ( isComment() )
	return CommentNode;
    if ( isCharacterData() )
	return CharacterDataNode;
    if( isAttr() )
	return AttributeNode;
    if( isElement() )
	return ElementNode;
    if (isEntityReference() )
	return EntityReferenceNode;
    if ( isEntity() )
	return EntityNode;
    if (isNotation() )
	return NotationNode;
    if ( isProcessingInstruction() )
	return ProcessingInstructionNode;
    if( isDocumentFragment() )
	return DocumentFragmentNode;
    if( isDocument() )
	return DocumentNode;
    if( isDocumentType() )
	return DocumentTypeNode;

    return QDomNode::BaseNode;
}

/*!
  Returns the parent node, If this node has no parent, then a null node is
  returned (i.e. a node for which isNull() returns TRUE).
*/
QDomNode QDomNode::parentNode() const
{
    if ( !impl )
	return QDomNode();
    return QDomNode( IMPL->parent );
}

/*!
  Returns a list of all child nodes.

  Most often you will call this function on a QDomElement object.
  If the XML document looks like this:

  \code
  <body>
   <h1>Heading</h1>
   <p>Hallo <b>you</b></p>
  </body>
  \endcode

  Then the list of child nodes for the "body"-element will contain the node
  created by the &lt;h1&gt; tag and the node created by the &lt;p&gt; tag.

  The nodes in the list are not copied; so changing the nodes in the list will
  also change the children of this node.

  \sa firstChild() lastChild()
*/
QDomNodeList QDomNode::childNodes() const
{
    if ( !impl )
	return QDomNodeList();
    return QDomNodeList( new QDOM_NodeListPrivate( impl ) );
}

/*!
  Returns the first child of the node. If there is no child node, a null node
  is returned.

  \sa lastChild() childNodes()
*/
QDomNode QDomNode::firstChild() const
{
    if ( !impl )
	return QDomNode();
    return QDomNode( IMPL->first );
}

/*!
  Returns the last child of the node. If there is no child node then a null
  node is returned.

  \sa firstChild() childNodes()
*/
QDomNode QDomNode::lastChild() const
{
    if ( !impl )
	return QDomNode();
    return QDomNode( IMPL->last );
}

/*!
  Returns the previous sibling in the document tree. Changing the returned node
  will also change the node in the document tree.

  If you have XML like this:
  \code
  <h1>Heading</h1>
  <p>The text...</p>
  <h2>Next heading</h2>
  \endcode

  and this QDomNode represents the &lt;p&gt; tag, the previousSibling
  will return the node representing the &lt;h1&gt; tag.

  \sa nextSibling()
*/
QDomNode QDomNode::previousSibling() const
{
    if ( !impl )
	return QDomNode();
    return QDomNode( IMPL->prev );
}

/*!
  Returns the next sibling in the document tree. Changing the returned node
  will also change the node in the document tree.

  If you have XML like this:
  \code
  <h1>Heading</h1>
  <p>The text...</p>
  <h2>Next heading</h2>
  \endcode

  and this QDomNode represents the &lt;p&gt; tag, the nextSibling
  will return the node representing the &lt;h2&gt; tag.

  \sa previousSibling()
*/
QDomNode QDomNode::nextSibling() const
{
    if ( !impl )
	return QDomNode();
    return QDomNode( IMPL->next );
}

/*!
  Returns a map of all attributes. Attributes are only provided for
  QDomElement.

  Changing the attributes in the map will also change the attributes of this
  QDomNode.
*/
QDomNamedNodeMap QDomNode::attributes() const
{
    if ( !impl )
	return QDomNamedNodeMap();

    return QDomNamedNodeMap( impl->attributes() );
}

/*!
  Returns the document to which this node belongs.
*/
QDomDocument QDomNode::ownerDocument() const
{
    if ( !impl )
	return QDomDocument();
    return QDomDocument( IMPL->ownerDocument() );
}

/*!
  Creates a real copy of the QDomNode.

  If \a deep is TRUE, then the cloning is done recursive.
  That means all children are copied, too. Otherwise the cloned
  node does not contain child nodes.
*/
QDomNode QDomNode::cloneNode( bool deep ) const
{
    if ( !impl )
	return QDomNode();
    return QDomNode( IMPL->cloneNode( deep ) );
}

/*!
  Inserts the node \a newChild before the child node \a refChild.  \a refChild
  has to be a direct child of this node. If \a refChild is null then \a
  newChild is inserted as first child.

  If \a newChild is currently child of another parent, then it is reparented.
  If \a newChild is currently a child of this QDomNode, then its position in
  the list of children is changed.

  If \a newChild is a QDomDocumentFragment, then the children of the fragment
  are removed from the fragment and inserted after \a refChild.

  Returns a new reference to \a newChild on success or an empty node on
  failure.

  \sa insertAfter() replaceChild() removeChild() appendChild()
*/
QDomNode QDomNode::insertBefore( const QDomNode& newChild, const QDomNode& refChild )
{
    if ( !impl )
	return QDomNode();
    return QDomNode( IMPL->insertBefore( newChild.impl, refChild.impl ) );
}

/*!
  Inserts the node \a newChild after the child node \a refChild.  \a refChild
  has to be a direct child of this node. If \a refChild is null then \a
  newChild is appended as last child.

  If \a newChild is currently child of another parent, then it is reparented.
  If \a newChild is currently a child of this QDomNode, then its position in
  the list of children is changed.

  If \a newChild is a QDomDocumentFragment, then the children of the fragment
  are removed from the fragment and inserted after \a refChild.

  Returns a new reference to \a newChild on success or an empty node on failure.

  \sa insertBefore() replaceChild() removeChild() appendChild()
*/
QDomNode QDomNode::insertAfter( const QDomNode& newChild, const QDomNode& refChild )
{
    if ( !impl )
	return QDomNode();
    return QDomNode( IMPL->insertAfter( newChild.impl, refChild.impl ) );
}

/*!
  Replaces \a oldChild with \a newChild. \a oldChild has to be a direct child
  of this node.

  If \a newChild is currently child of another parent, then it is reparented.
  If \a newChild is currently a child of this QDomNode, then its position in
  the list of children is changed.

  If \a newChild is a QDomDocumentFragment, then the children of the fragment
  are removed from the fragment and inserted after \a refChild.

  Returns a new reference to \a oldChild on success or a null node an failure.

  \sa insertBefore() insertAfter() removeChild() appendChild()
*/
QDomNode QDomNode::replaceChild( const QDomNode& newChild, const QDomNode& oldChild )
{
    if ( !impl )
	return QDomNode();
    return QDomNode( IMPL->replaceChild( newChild.impl, oldChild.impl ) );
}

/*!
  Removes \a oldChild from the list of children.
  \a oldChild has to be a direct child of this node.

  Returns a new reference to \a oldChild on success or a null node on failure.

  \sa insertBefore() insertAfter() replaceChild() appendChild()
*/
QDomNode QDomNode::removeChild( const QDomNode& oldChild )
{
    if ( !impl )
	return QDomNode();

    if ( oldChild.isNull() )
	return QDomNode();

    return QDomNode( IMPL->removeChild( oldChild.impl ) );
}

/*!
  Appends \a newChild to the end of the children list.

  If \a newChild is currently child of another parent, then it is reparented.
  If \a newChild is currently a child of this QDomNode, then its position in
  the list of children is changed.

  Returns a new reference to \a newChild.

  \sa insertBefore() insertAfter() replaceChild() removeChild()
*/
QDomNode QDomNode::appendChild( const QDomNode& newChild )
{
    if ( !impl )
	return QDomNode();
    return QDomNode( IMPL->appendChild( newChild.impl ) );
}

/*!
  Returns TRUE if this node does not reference any internal object, otherwise
  FALSE.
*/
bool QDomNode::isNull() const
{
    return ( impl == 0 );
}

/*!
  Dereferences the internal object. The node is then a null node.

  \sa isNull()
*/
void QDomNode::clear()
{
    if ( impl && impl->deref() ) delete impl;
    impl = 0;
}

/*!
  Returns the first child node for which nodeName() equals \a name.

  If no such direct child exists, a null node is returned.

  \sa nodeName()
*/
QDomNode QDomNode::namedItem( const QString& name ) const
{
    if ( !impl )
	return QDomNode();
    return QDomNode( impl->namedItem( name ) );
}

/*!
  Writes the XML representation of the node including all its children
  on the stream.
*/
void QDomNode::save( QTextStream& str, int indent ) const
{
    if ( impl )
	IMPL->save( str, indent );
}

/*!
  Writes the XML representation of the node including all its children
  on the stream.
*/
QTextStream& operator<<( QTextStream& str, const QDomNode& node )
{
    node.save( str, 0 );

    return str;
}

/*!
  Returns TRUE if the node is an attribute, otherwise FALSE.

  If this function returns TRUE, this does not imply that this object is
  a QDomAttribute; you can get the QDomAttribute with toAttribute().

  \sa toAttribute()
*/
bool QDomNode::isAttr() const
{
    if(impl)
	return impl->isAttr();
    return FALSE;
}

/*!
  Returns TRUE if the node is a CDATA section, otherwise FALSE.

  If this function returns TRUE, this does not imply that this object is
  a QDomCDATASection; you can get the QDomCDATASection with toCDATASection().

  \sa toCDATASection()
*/
bool QDomNode::isCDATASection() const
{
    if(impl)
	return impl->isCDATASection();
    return FALSE;
}

/*!
  Returns TRUE if the node is a document fragment, otherwise FALSE.

  If this function returns TRUE, this does not imply that this object is
  a QDomDocumentFragment; you can get the QDomDocumentFragment with
  toDocumentFragment().

  \sa toDocumentFragment()
*/
bool QDomNode::isDocumentFragment() const
{
    if(impl)
	return impl->isDocumentFragment();
    return FALSE;
}

/*!
  Returns TRUE if the node is a document, otherwise FALSE.

  If this function returns TRUE, this does not imply that this object is
  a QDomDocument; you can get the QDomDocument with toDocument().

  \sa toDocument()
*/
bool QDomNode::isDocument() const
{
    if(impl)
	return impl->isDocument();
    return FALSE;
}

/*!
  Returns TRUE if the node is a document type, otherwise FALSE.

  If this function returns TRUE, this does not imply that this object is
  a QDomDocumentType; you can get the QDomDocumentType with toDocumentType().

  \sa toDocumentType()
*/
bool QDomNode::isDocumentType() const
{
    if(impl)
	return impl->isDocumentType();
    return FALSE;
}

/*!
  Returns TRUE if the node is an element, otherwise FALSE.

  If this function returns TRUE, this does not imply that this object is
  a QDomElement; you can get the QDomElement with toElement().

  \sa toElement()
*/
bool QDomNode::isElement() const
{
    if(impl)
	return impl->isElement();
    return FALSE;
}

/*!
  Returns TRUE if the node is an entity reference, otherwise FALSE.

  If this function returns TRUE, this does not imply that this object is
  a QDomEntityReference; you can get the QDomEntityReference with
  toEntityReference().

  \sa toEntityReference()
*/
bool QDomNode::isEntityReference() const
{
    if(impl)
	return impl->isEntityReference();
    return FALSE;
}

/*!
  Returns TRUE if the node is a text, otherwise FALSE.

  If this function returns TRUE, this does not imply that this object is
  a QDomText; you can get the QDomText with toText().

  \sa toText()
*/
bool QDomNode::isText() const
{
    if(impl)
	return impl->isText();
    return FALSE;
}

/*!
  Returns TRUE if the node is an entity, otherwise FALSE.

  If this function returns TRUE, this does not imply that this object is
  a QDomEntity; you can get the QDomEntity with toEntity().

  \sa toEntity()
*/
bool QDomNode::isEntity() const
{
    if(impl)
	return impl->isEntity();
    return FALSE;
}

/*!
  Returns TRUE if the node is a notation, otherwise FALSE.

  If this function returns TRUE, this does not imply that this object is
  a QDomNotation; you can get the QDomNotation with toNotation().

  \sa toNotation()
*/
bool QDomNode::isNotation() const
{
    if(impl)
	return impl->isNotation();
    return FALSE;
}

/*!
  Returns TRUE if the node is a processing instruction, otherwise FALSE.

  If this function returns TRUE, this does not imply that this object is
  a QDomProcessingInstruction; you can get the QProcessingInstruction with
  toProcessingInstruction().

  \sa toProcessingInstruction()
*/
bool QDomNode::isProcessingInstruction() const
{
    if(impl)
	return impl->isProcessingInstruction();
    return FALSE;
}

/*!
  Returns TRUE if the node is a character data node, otherwise FALSE.

  If this function returns TRUE, this does not imply that this object is
  a QDomCharacterData; you can get the QDomCharacterData with
  toCharacterData().

  \sa toCharacterData()
*/
bool QDomNode::isCharacterData() const
{
    if(impl)
	return impl->isCharacterData();
    return FALSE;
}

/*!
  Returns TRUE if the node is a comment, otherwise FALSE.

  If this function returns TRUE, this does not imply that this object is
  a QDomComment; you can get the QDomComment with toComment().

  \sa toComment()
*/
bool QDomNode::isComment() const
{
     if(impl)
	return impl->isComment();
    return FALSE;
}

#undef IMPL

/*==============================================================*/
/*                      NamedNodeMap                            */
/*==============================================================*/

/**************************************************************
 *
 * QDOM_NamedNodeMapPrivate
 *
 **************************************************************/

class QDOM_NamedNodeMapPrivate : public QShared
{
public:
    QDOM_NamedNodeMapPrivate( QDOM_NodePrivate* );
    ~QDOM_NamedNodeMapPrivate();

    QDOM_NodePrivate* namedItem( const QString& name ) const;
    QDOM_NodePrivate* setNamedItem( QDOM_NodePrivate* arg );
    QDOM_NodePrivate* removeNamedItem( const QString& name );
    QDOM_NodePrivate* item( int index ) const;
    uint length() const;
    bool contains( const QString& name ) const;

    /**
     * Remove all children from the map.
     */
    void clearMap();
    bool isReadOnly() { return readonly; }
    void setReadOnly( bool r ) { readonly = r; }
    bool isAppendToParent() { return appendToParent; }
    /**
     * If TRUE, then the node will redirect insert/remove calls
     * to its parent by calling QDOM_NodePrivate::appendChild or removeChild.
     * In addition the map wont increase or decrease the reference count
     * of the nodes it contains.
     *
     * By default this value is FALSE and the map will handle reference counting
     * by itself.
     */
    void setAppendToParent( bool b ) { appendToParent = b; }

    /**
     * Creates a copy of the map. It is a deep copy
     * that means that all children are cloned.
     */
    QDOM_NamedNodeMapPrivate* clone( QDOM_NodePrivate* parent );

    // Variables
    QDict<QDOM_NodePrivate> map;
    QDOM_NodePrivate* parent;
    bool readonly;
    bool appendToParent;
};

QDOM_NamedNodeMapPrivate::QDOM_NamedNodeMapPrivate( QDOM_NodePrivate* n )
{
    readonly = FALSE;
    parent = n;
    appendToParent = FALSE;
}

QDOM_NamedNodeMapPrivate::~QDOM_NamedNodeMapPrivate()
{
    clearMap();
}

QDOM_NamedNodeMapPrivate* QDOM_NamedNodeMapPrivate::clone( QDOM_NodePrivate* p )
{
    QDOM_NamedNodeMapPrivate* m = new QDOM_NamedNodeMapPrivate( p );
    m->readonly = readonly;
    m->appendToParent = appendToParent;

    QDictIterator<QDOM_NodePrivate> it ( map );
    for ( ; it.current(); ++it )
	m->setNamedItem( it.current()->cloneNode() );

    // we are no longer interested in ownership
    m->deref();
    return m;
}

void QDOM_NamedNodeMapPrivate::clearMap()
{
    // Dereference all of our children if we took references
    if ( !appendToParent ) {
	QDictIterator<QDOM_NodePrivate> it( map );
	for ( ; it.current(); ++it )
	    if ( it.current()->deref() )
		delete it.current();
    }

    map.clear();
}

QDOM_NodePrivate* QDOM_NamedNodeMapPrivate::namedItem( const QString& name ) const
{
    QDOM_NodePrivate* p = map[ name ];
    return p;
}

QDOM_NodePrivate* QDOM_NamedNodeMapPrivate::setNamedItem( QDOM_NodePrivate* arg )
{
    if ( readonly || !arg )
	return 0;

    if ( appendToParent )
	return parent->appendChild( arg );

    // We take a reference
    arg->ref();
    map.insert( arg->nodeName(), arg );
    return arg;
}

QDOM_NodePrivate* QDOM_NamedNodeMapPrivate::removeNamedItem( const QString& name )
{
    if ( readonly )
	return 0;

    QDOM_NodePrivate* p = namedItem( name );
    if ( p == 0 )
	return 0;
    if ( appendToParent )
	return parent->removeChild( p );

    map.remove( p->nodeName() );
    // We took a reference, so we have to free one here
    p->deref();
    return p;
}

QDOM_NodePrivate* QDOM_NamedNodeMapPrivate::item( int index ) const
{
    if ( (uint)index >= length() )
	return 0;

    QDictIterator<QDOM_NodePrivate> it( map );
    for ( int i = 0; i < index; ++i, ++it )
	;
    return it.current();
}

uint QDOM_NamedNodeMapPrivate::length() const
{
    return map.count();
}

bool QDOM_NamedNodeMapPrivate::contains( const QString& name ) const
{
    return ( map[ name ] != 0 );
}

/**************************************************************
 *
 * QDomNamedNodeMap
 *
 **************************************************************/

#define IMPL ((QDOM_NamedNodeMapPrivate*)impl)

/*!
  \class QDomNamedNodeMap qdom.h
  \brief The QDomNamedNodeMap class contains a collection of nodes that can be
  accessed by name.

  \module XML

  Note that QDomNamedNodeMap does not inherit from QDomNodeList;
  QDomNamedNodeMaps does not provide any specific order of the nodes. Nodes
  contained in a QDomNamedNodeMap may also be accessed by an ordinal index, but
  this is simply to allow a convenient enumeration of the contents of a
  QDomNamedNodeMap and does not imply that the DOM specifies an order on the
  nodes.

  The QDomNamedNodeMap is used in three places:

  <ul>
  <li> QDomDocumentType::entities() returns a map of all entities
       described in the DTD.
  <li> QDomDocumentType::notations() returns a map of all notations
       described in the DTD.
  <li> QDomElement::attributes() returns a map of all attributes of the
       element.
  </ul>

  Items in the map are identified by the name which QDomNode::name() returns.
  They can be queried using the namedItem() function and set using
  setNamedItem().

  \sa namedItem() setNamedItem()
*/

/*!
  Constructs an empty map.
*/
QDomNamedNodeMap::QDomNamedNodeMap()
{
    impl = 0;
}

/*!
  Copy constructor.
*/
QDomNamedNodeMap::QDomNamedNodeMap( const QDomNamedNodeMap& n )
{
    impl = n.impl;
    if ( impl )
	impl->ref();
}

/*!
  \internal
*/
QDomNamedNodeMap::QDomNamedNodeMap( QDOM_NamedNodeMapPrivate* n )
{
    impl = n;
    if ( impl )
	impl->ref();
}

/*!
  Assignement operator.
*/
QDomNamedNodeMap& QDomNamedNodeMap::operator= ( const QDomNamedNodeMap& n )
{
    if ( impl && impl->deref() )
	delete impl;
    impl = n.impl;
    if ( impl )
	impl->ref();

    return *this;
}

/*!
  Returns TRUE if the maps are equal, FALSE otherwise.
*/
bool QDomNamedNodeMap::operator== ( const QDomNamedNodeMap& n ) const
{
    return ( impl == n.impl );
}

/*!
  Returns TRUE if the maps are not equal, FALSE otherwise.
*/
bool QDomNamedNodeMap::operator!= ( const QDomNamedNodeMap& n ) const
{
    return ( impl != n.impl );
}

/*!
  Destructor.
*/
QDomNamedNodeMap::~QDomNamedNodeMap()
{
    if ( impl && impl->deref() )
	delete impl;
}

/*!
  Returns the node associated with they key \a name.

  If the map does not contain such a node, then a null node is returned.

  \sa setNamedItem()
*/
QDomNode QDomNamedNodeMap::namedItem( const QString& name ) const
{
    if ( !impl )
	return QDomNode();
    return QDomNode( IMPL->namedItem( name ) );
}

/*!
  Inserts the node \a newNode in the map. The kye for the map is the name of \a
  newNode as returned by QDomNode::nodeName().

  The function returns the newly inserted node.

  \sa removeNamedItem()
*/
QDomNode QDomNamedNodeMap::setNamedItem( const QDomNode& newNode )
{
    if ( !impl )
	return QDomNode();
    return QDomNode( IMPL->setNamedItem( (QDOM_NodePrivate*)newNode.impl ) );
}

/*!
  Removes the node with the name \a name from the map.

  The function returns the removed node or a null node
  if the map did not contain a node with the name \a name.

  \sa setNamedItem()
*/
QDomNode QDomNamedNodeMap::removeNamedItem( const QString& name )
{
    if ( !impl )
	return QDomNode();
    return QDomNode( IMPL->removeNamedItem( name ) );
}

/*!
  Retrieves the node at position \a index.

  This can be used to iterate over the map.

  \sa length()
*/
QDomNode QDomNamedNodeMap::item( int index ) const
{
    if ( !impl )
	return QDomNode();
    return QDomNode( IMPL->item( index ) );
}

/*!
  Returns the number of nodes in the map.

  \sa item()
*/
uint QDomNamedNodeMap::length() const
{
    if ( !impl )
	return 0;
    return IMPL->length();
}

/*!
  Returns TRUE if the map contains a node with the name \a name, otherwise
  FALSE.
*/
bool QDomNamedNodeMap::contains( const QString& name ) const
{
    if ( !impl )
	return FALSE;
    return IMPL->contains( name );
}

#undef IMPL

/*==============================================================*/
/*==============================================================*/

/**************************************************************
 *
 * QDOM_DocumentTypePrivate
 *
 **************************************************************/

class QDOM_DocumentTypePrivate : public QDOM_NodePrivate
{
public:
    QDOM_DocumentTypePrivate( QDOM_DocumentPrivate*, QDOM_NodePrivate* parent = 0 );
    QDOM_DocumentTypePrivate( QDOM_DocumentTypePrivate* n, bool deep );
    ~QDOM_DocumentTypePrivate();

    //     virtual QDOM_NamedNodeMapPrivate* entities();
    //     virtual QDOM_NamedNodeMapPrivate* notations();

    // Overloaded from QDOM_NodePrivate
    virtual QDOM_NodePrivate* cloneNode( bool deep = TRUE );
    virtual QDOM_NodePrivate* insertBefore( QDOM_NodePrivate* newChild, QDOM_NodePrivate* refChild );
    virtual QDOM_NodePrivate* insertAfter( QDOM_NodePrivate* newChild, QDOM_NodePrivate* refChild );
    virtual QDOM_NodePrivate* replaceChild( QDOM_NodePrivate* newChild, QDOM_NodePrivate* oldChild );
    virtual QDOM_NodePrivate* removeChild( QDOM_NodePrivate* oldChild );
    virtual QDOM_NodePrivate* appendChild( QDOM_NodePrivate* newChild );

    // Overloaded from QDOM_DocumentTypePrivate
    virtual bool isDocumentType() { return TRUE; }
    virtual void save( QTextStream& s, int ) const;

    // Variables
    QDOM_NamedNodeMapPrivate* entities;
    QDOM_NamedNodeMapPrivate* notations;
};

QDOM_DocumentTypePrivate::QDOM_DocumentTypePrivate( QDOM_DocumentPrivate* doc, QDOM_NodePrivate* parent )
    : QDOM_NodePrivate( doc, parent )
{
    entities = new QDOM_NamedNodeMapPrivate( this );
    notations = new QDOM_NamedNodeMapPrivate( this );

    entities->setAppendToParent( TRUE );
    notations->setAppendToParent( TRUE );
}

QDOM_DocumentTypePrivate::QDOM_DocumentTypePrivate( QDOM_DocumentTypePrivate* n, bool deep )
    : QDOM_NodePrivate( n, deep )
{
    entities = new QDOM_NamedNodeMapPrivate( this );
    notations = new QDOM_NamedNodeMapPrivate( this );

    entities->setAppendToParent( TRUE );
    notations->setAppendToParent( TRUE );

    // Refill the maps with our new children
    QDOM_NodePrivate* p = first;
    while ( p ) {
	if ( p->isEntity() )
	    // Dont use normal insert function since we would create infinite recursion
	    entities->map.insert( p->nodeName(), p );
	if ( p->isNotation() )
	    // Dont use normal insert function since we would create infinite recursion
	    notations->map.insert( p->nodeName(), p );
    }
}

QDOM_DocumentTypePrivate::~QDOM_DocumentTypePrivate()
{
    if ( entities->deref() )
	delete entities;
    if ( notations->deref() )
	delete notations;
}

QDOM_NodePrivate* QDOM_DocumentTypePrivate::cloneNode( bool deep)
{
    QDOM_NodePrivate* p = new QDOM_DocumentTypePrivate( this, deep );
    // We are not interested in this node
    p->deref();
    return p;
}

QDOM_NodePrivate* QDOM_DocumentTypePrivate::insertBefore( QDOM_NodePrivate* newChild, QDOM_NodePrivate* refChild )
{
    // Call the origianl implementation
    QDOM_NodePrivate* p = QDOM_NodePrivate::insertBefore( newChild, refChild );
    // Update the maps
    if ( p && p->isEntity() )
	entities->map.insert( p->nodeName(), p );
    else if ( p && p->isNotation() )
	notations->map.insert( p->nodeName(), p );

    return p;
}

QDOM_NodePrivate* QDOM_DocumentTypePrivate::insertAfter( QDOM_NodePrivate* newChild, QDOM_NodePrivate* refChild )
{
    // Call the origianl implementation
    QDOM_NodePrivate* p = QDOM_NodePrivate::insertAfter( newChild, refChild );
    // Update the maps
    if ( p && p->isEntity() )
	entities->map.insert( p->nodeName(), p );
    else if ( p && p->isNotation() )
	notations->map.insert( p->nodeName(), p );

    return p;
}

QDOM_NodePrivate* QDOM_DocumentTypePrivate::replaceChild( QDOM_NodePrivate* newChild, QDOM_NodePrivate* oldChild )
{
    // Call the origianl implementation
    QDOM_NodePrivate* p = QDOM_NodePrivate::replaceChild( newChild, oldChild );
    // Update the maps
    if ( p ) {
	if ( oldChild && oldChild->isEntity() )
	    entities->map.remove( oldChild->nodeName() );
	else if ( oldChild && oldChild->isNotation() )
	    notations->map.remove( oldChild->nodeName() );

	if ( p->isEntity() )
	    entities->map.insert( p->nodeName(), p );
	else if ( p->isNotation() )
	    notations->map.insert( p->nodeName(), p );
    }

    return p;
}

QDOM_NodePrivate* QDOM_DocumentTypePrivate::removeChild( QDOM_NodePrivate* oldChild )
{
    // Call the origianl implementation
    QDOM_NodePrivate* p = QDOM_NodePrivate::removeChild(  oldChild );
    // Update the maps
    if ( p && p->isEntity() )
	entities->map.remove( p->nodeName() );
    else if ( p && p->isNotation() )
	notations->map.remove( p ->nodeName() );

    return p;
}

QDOM_NodePrivate* QDOM_DocumentTypePrivate::appendChild( QDOM_NodePrivate* newChild )
{
    return insertAfter( newChild, 0 );
}

void QDOM_DocumentTypePrivate::save( QTextStream& s, int ) const
{
    if ( name.isEmpty() )
	return;
    s << "<!DOCTYPE " << name << " ";

    // qDebug("--------- 3 DocType %i %i", entities->map.count(), notations->map.count() );

    if ( entities->length() > 0 || notations->length() > 0 ) {
	s << "[ ";

	QDictIterator<QDOM_NodePrivate> it2( notations->map );
	for ( ; it2.current(); ++it2 )
	    it2.current()->save( s, 0 );

	QDictIterator<QDOM_NodePrivate> it( entities->map );
	for ( ; it.current(); ++it )
	    it.current()->save( s, 0 );

	s << " ]";
    }

    s << ">";
}

/**************************************************************
 *
 * QDomDocumentType
 *
 **************************************************************/

#define IMPL ((QDOM_DocumentTypePrivate*)impl)

/*!
  \class QDomDocumentType qdom.h
  \brief The QDomDocumentType class is the representation of the DTD in the
  document tree.

  \module XML

  The QDomDocumentType class allows readonly access to some of the data
  structures in the DTD: it can return a map of all entities() and notations().

  In addition the function name() returns the name of the document type as
  specified in the &lt;!DOCTYPE name&gt; tag.

  \sa QDomDocument
*/

/*!
  Creates an empty QDomDocumentType object.
*/
QDomDocumentType::QDomDocumentType() : QDomNode()
{
}

/*!
  Copy constructor.

  The data of the copy is shared: modifying one will also change the other. If
  you want to make a real copy, use cloneNode() instead.
*/
QDomDocumentType::QDomDocumentType( const QDomDocumentType& n )
    : QDomNode( n )
{
}

/*!
  \internal
*/
QDomDocumentType::QDomDocumentType( QDOM_DocumentTypePrivate* n )
    : QDomNode( n )
{
}

/*!
  Assignement operator.

  The data of the copy is shared: modifying one will also change the other. If
  you want to make a real copy, use cloneNode() instead.
*/
QDomDocumentType& QDomDocumentType::operator= ( const QDomDocumentType& n )
{
    return (QDomDocumentType&) QDomNode::operator=( n );
}

/*!
  Destructor.
*/
QDomDocumentType::~QDomDocumentType()
{
}

/*!
  Returns the name of the document type as specified in
  the &lt;!DOCTYPE name&gt; tag.

  \sa nodeName()
*/
QString QDomDocumentType::name() const
{
    if ( !impl )
	return QString::null;

    return IMPL->nodeName();
}

/*!
  Returns a map of all entities described in the DTD.
*/
QDomNamedNodeMap QDomDocumentType::entities() const
{
    if ( !impl )
	return QDomNamedNodeMap();
    return QDomNamedNodeMap( IMPL->entities );
}

/*!
  Returns a map of all notations described in the DTD.
*/
QDomNamedNodeMap QDomDocumentType::notations() const
{
    if ( !impl )
	return QDomNamedNodeMap();
    return QDomNamedNodeMap( IMPL->notations );
}

/*!
  Returns \c DocumentTypeNode.

  \sa isDocumentType() QDomNode::toDocumentType()
*/
QDomNode::NodeType QDomDocumentType::nodeType() const
{
    return DocumentTypeNode;
}

/*!
  This function overloads QDomNode::isDocumentType().

  \sa nodeType() QDomNode::toDocumentType()
*/
bool QDomDocumentType::isDocumentType() const
{
    return TRUE;
}

#undef IMPL

/*==============================================================*/
/*                     DocumentFragment                         */
/*==============================================================*/

/**************************************************************
 *
 * QDOM_DocumentFragmentPrivate
 *
 **************************************************************/

class QDOM_DocumentFragmentPrivate : public QDOM_NodePrivate
{
public:
    QDOM_DocumentFragmentPrivate( QDOM_DocumentPrivate*, QDOM_NodePrivate* parent = 0 );
    QDOM_DocumentFragmentPrivate( QDOM_NodePrivate* n, bool deep );
    ~QDOM_DocumentFragmentPrivate();

    // Overloaded from QDOM_NodePrivate
    virtual QDOM_NodePrivate* cloneNode( bool deep = TRUE );
    virtual bool isDocumentFragment() { return TRUE; }

    static QString* dfName;
};

QString* QDOM_DocumentFragmentPrivate::dfName = 0;

QDOM_DocumentFragmentPrivate::QDOM_DocumentFragmentPrivate( QDOM_DocumentPrivate* doc, QDOM_NodePrivate* parent )
    : QDOM_NodePrivate( doc, parent )
{
    if ( !dfName )
	dfName = new QString( "#document-fragment" );
    name = *dfName;
}

QDOM_DocumentFragmentPrivate::QDOM_DocumentFragmentPrivate( QDOM_NodePrivate* n, bool deep )
    : QDOM_NodePrivate( n, deep )
{
}

QDOM_DocumentFragmentPrivate::~QDOM_DocumentFragmentPrivate()
{
}

QDOM_NodePrivate* QDOM_DocumentFragmentPrivate::cloneNode( bool deep)
{
    QDOM_NodePrivate* p = new QDOM_DocumentFragmentPrivate( this, deep );
    // We are not interested in this node
    p->deref();
    return p;
}

/**************************************************************
 *
 * QDomDocumentFragment
 *
 **************************************************************/

#define IMPL ((QDOM_DocumentFragmentPrivate*)impl)

/*!
  \class QDomDocumentFragment qdom.h
  \brief The QDomDocumentFragment class is a tree of QDomNodes which is usually
  not a complete QDomDocument.

  \module XML

  If you want to do complex tree operations it is useful to have a lightweight
  class to store nodes and their relations. QDomDocumentFragment stores a
  subtree of a document which does not necessarily represent a well-formed XML
  document.

  QDomDocumentFragment is also useful if you want to group several nodes in a
  list and insert them all together as children of some
  node. In these cases QDomDocumentFragment can be used as a temporary
  container for this list of children.

  The most important feature of QDomDocumentFragment is, that it is treated in
  a special way by QDomNode::insertAfter(), QDomNode::insertBefore() and
  QDomNode::replaceChild(): instead of inserting the fragment itself, all
  children of the fragment are inserted.
*/

/*!
  Constructs an empty DocumentFragment.
*/
QDomDocumentFragment::QDomDocumentFragment()
{
}

/*!
  \internal
*/
QDomDocumentFragment::QDomDocumentFragment( QDOM_DocumentFragmentPrivate* n )
    : QDomNode( n )
{
}

/*!
  Copy constructor.

  The data of the copy is shared: modifying one will also change the other. If
  you want to make a real copy, use cloneNode() instead.
*/
QDomDocumentFragment::QDomDocumentFragment( const QDomDocumentFragment& x )
    : QDomNode( x )
{
}

/*!
  Assignment operator.

  The data of the copy is shared: modifying one will also change the other. If
  you want to make a real copy, use cloneNode() instead.
*/
QDomDocumentFragment& QDomDocumentFragment::operator= ( const QDomDocumentFragment& x )
{
    return (QDomDocumentFragment&) QDomNode::operator=( x );
}

/*!
  Destructor.
*/
QDomDocumentFragment::~QDomDocumentFragment()
{
}

/*!
  Returns \c DocumentFragment.

  \sa isDocumentFragment() QDomNode::toDocumentFragment()
*/
QDomNode::NodeType QDomDocumentFragment::nodeType() const
{
    return QDomNode::DocumentFragmentNode;
}

/*!
  This function reimplements QDomNode::isDocumentFragment().

  \sa nodeType() QDomNode::toDocumentFragment()
*/
bool QDomDocumentFragment::isDocumentFragment() const
{
    return TRUE;
}

#undef IMPL

/*==============================================================*/
/*                     CharacterData                            */
/*==============================================================*/

/**************************************************************
 *
 * QDOM_CharacterDataPrivate
 *
 **************************************************************/

class QDOM_CharacterDataPrivate : public QDOM_NodePrivate
{
public:
    QDOM_CharacterDataPrivate( QDOM_DocumentPrivate*, QDOM_NodePrivate* parent, const QString& data );
    QDOM_CharacterDataPrivate( QDOM_CharacterDataPrivate* n, bool deep );
    ~QDOM_CharacterDataPrivate();

    uint dataLength() const;
    QString substringData( unsigned long offset, unsigned long count ) const;
    void    appendData( const QString& arg );
    void    insertData( unsigned long offset, const QString& arg );
    void    deleteData( unsigned long offset, unsigned long count );
    void    replaceData( unsigned long offset, unsigned long count, const QString& arg );

    // Overloaded from QDOM_NodePrivate
    virtual bool isCharacterData() { return TRUE; }
    virtual QDOM_NodePrivate* cloneNode( bool deep = TRUE );

    static QString* cdName;
};

QString* QDOM_CharacterDataPrivate::cdName = 0;

QDOM_CharacterDataPrivate::QDOM_CharacterDataPrivate( QDOM_DocumentPrivate* d, QDOM_NodePrivate* p,
						      const QString& data )
    : QDOM_NodePrivate( d, p )
{
    value = data;

    if ( !cdName )
	cdName = new QString( "#character-data" );
    name = *cdName;
}

QDOM_CharacterDataPrivate::QDOM_CharacterDataPrivate( QDOM_CharacterDataPrivate* n, bool deep )
    : QDOM_NodePrivate( n, deep )
{
}

QDOM_CharacterDataPrivate::~QDOM_CharacterDataPrivate()
{
}

QDOM_NodePrivate* QDOM_CharacterDataPrivate::cloneNode( bool deep )
{
    QDOM_NodePrivate* p = new QDOM_CharacterDataPrivate( this, deep );
    // We are not interested in this node
    p->deref();
    return p;
}

uint QDOM_CharacterDataPrivate::dataLength() const
{
    return value.length();
}

QString QDOM_CharacterDataPrivate::substringData( unsigned long offset, unsigned long n ) const
{
    return value.mid( offset, n );
}

void QDOM_CharacterDataPrivate::insertData( unsigned long offset, const QString& arg )
{
    value.insert( offset, arg );
}

void QDOM_CharacterDataPrivate::deleteData( unsigned long offset, unsigned long n )
{
    value.remove( offset, n );
}

void QDOM_CharacterDataPrivate::replaceData( unsigned long offset, unsigned long n, const QString& arg )
{
    value.replace( offset, n, arg );
}

void QDOM_CharacterDataPrivate::appendData( const QString& arg )
{
    value += arg;
}

/**************************************************************
 *
 * QDomCharacterData
 *
 **************************************************************/

#define IMPL ((QDOM_CharacterDataPrivate*)impl)

/*!
  \class QDomCharacterData qdom.h
  \brief The QDomCharacterData class represents a generic string in the DOM.

  \module XML

  Character data as used in XML specifies a generic data string.
  More specialized versions of this class are QDomText, QDomComment
  and QDomCDATASection.

  \sa QDomText QDomComment QDomCDATASection
*/

/*!
  Constructs an empty character data object.
*/
QDomCharacterData::QDomCharacterData()
{
}

/*!
  Copy constructor.

  The data of the copy is shared: modifying one will also change the other. If
  you want to make a real copy, use cloneNode() instead.
*/
QDomCharacterData::QDomCharacterData( const QDomCharacterData& x )
    : QDomNode( x )
{
}

/*!
  \internal
*/
QDomCharacterData::QDomCharacterData( QDOM_CharacterDataPrivate* n )
    : QDomNode( n )
{
}

/*!
  Assignment operator.

  The data of the copy is shared: modifying one will also change the other. If
  you want to make a real copy, use cloneNode() instead.
*/
QDomCharacterData& QDomCharacterData::operator= ( const QDomCharacterData& x )
{
    return (QDomCharacterData&) QDomNode::operator=( x );
}

/*!
  Destructor.
*/
QDomCharacterData::~QDomCharacterData()
{
}

/*!
  Returns the string stored in this object.

  If the node is a null node, it will return a null string.
*/
QString QDomCharacterData::data() const
{
    if ( !impl )
	return QString::null;
    return impl->nodeValue();
}

/*!
  Sets the string of this object to \a v.
*/
void QDomCharacterData::setData( const QString& v )
{
    if ( impl )
	impl->setNodeValue( v );
}

/*!
  Returns the length of the stored string.
*/
uint QDomCharacterData::length() const
{
    if ( impl )
	return IMPL->dataLength();
    return 0;
}

/*!
  Returns the substring from position \a offset with length \a count.
*/
QString QDomCharacterData::substringData( unsigned long offset, unsigned long count )
{
    if ( !impl )
	return QString::null;
    return IMPL->substringData( offset, count );
}

/*!
  Appends \a arg to the stored string.
*/
void QDomCharacterData::appendData( const QString& arg )
{
    if ( impl )
	IMPL->appendData( arg );
}

/*!
  Inserts the string \a arg at position \a offset into the stored string.
*/
void QDomCharacterData::insertData( unsigned long offset, const QString& arg )
{
    if ( impl )
	IMPL->insertData( offset, arg );
}

/*!
  Deletes the substring starting at position \a offset with length \a count.
*/
void QDomCharacterData::deleteData( unsigned long offset, unsigned long count )
{
    if ( impl )
	IMPL->deleteData( offset, count );
}

/*!
  Replaces the substring starting at \a offset with length \a count with the
  string \a arg.
*/
void QDomCharacterData::replaceData( unsigned long offset, unsigned long count, const QString& arg )
{
    if ( impl )
	IMPL->replaceData( offset, count, arg );
}

/*!
  Returns the type of node this object refers to (i.e. \c TextNode,
  \c CDATASectionNode, \c CommentNode or \c CharacterDataNode). For a null node
  \c CharacterDataNode is returned.
*/
QDomNode::NodeType QDomCharacterData::nodeType() const
{
    if( !impl )
        return CharacterDataNode;
    return QDomNode::nodeType();
}

/*!
  Returns TRUE.
*/
bool QDomCharacterData::isCharacterData() const
{
    return TRUE;
}

#undef IMPL

/**************************************************************
 *
 * QDOM_TextPrivate
 *
 **************************************************************/

class QDOM_TextPrivate : public QDOM_CharacterDataPrivate
{
public:
    QDOM_TextPrivate( QDOM_DocumentPrivate*, QDOM_NodePrivate* parent, const QString& value );
    QDOM_TextPrivate( QDOM_TextPrivate* n, bool deep );
    ~QDOM_TextPrivate();

    QDOM_TextPrivate* splitText( int offset );

    // Overloaded from QDOM_NodePrivate
    virtual QDOM_NodePrivate* cloneNode( bool deep = TRUE );
    virtual bool isText() { return TRUE; }
    virtual void save( QTextStream& s, int ) const;

    static QString* textName;
};

/*==============================================================*/
/*                        Attr                                  */
/*==============================================================*/

/**************************************************************
 *
 * QDOM_AttrPrivate
 *
 **************************************************************/

class QDOM_AttrPrivate : public QDOM_NodePrivate
{
public:
    QDOM_AttrPrivate( QDOM_DocumentPrivate*, QDOM_NodePrivate* parent, const QString& name );
    QDOM_AttrPrivate( QDOM_AttrPrivate* n, bool deep );
    ~QDOM_AttrPrivate();

    bool specified() const;

    // Overloaded from QDOM_NodePrivate
    virtual QDOM_NodePrivate* cloneNode( bool deep = TRUE );
    virtual bool isAttr() { return TRUE; }
    virtual void save( QTextStream& s, int ) const;

    // Variables
    bool m_specified;
};

QDOM_AttrPrivate::QDOM_AttrPrivate( QDOM_DocumentPrivate* d, QDOM_NodePrivate* parent, const QString& name_ )
    : QDOM_NodePrivate( d, parent )
{
    name = name_;
    m_specified = FALSE;
    // qDebug("ATTR");
}

QDOM_AttrPrivate::QDOM_AttrPrivate( QDOM_AttrPrivate* n, bool deep )
    : QDOM_NodePrivate( n, deep )
{
    m_specified = n->specified();
    // qDebug("ATTR");
}

QDOM_AttrPrivate::~QDOM_AttrPrivate()
{
    // qDebug("~ATTR %s=%s", nodeName().latin1(), nodeValue().latin1() );
}

QDOM_NodePrivate* QDOM_AttrPrivate::cloneNode( bool deep )
{
    QDOM_NodePrivate* p = new QDOM_AttrPrivate( this, deep );
    // We are not interested in this node
    p->deref();
    return p;
}

bool QDOM_AttrPrivate::specified() const
{
    return m_specified;
}

/*
  Encode an attribute value upon saving.
*/
static QString encodeAttr( const QString& str )
{
    QString tmp( str );
    uint len = tmp.length();
    uint i = 0;
    while ( i < len ) {
	if ( tmp[(int)i] == '<' ) {
	    tmp.replace( i, 1, "&lt;" );
	    len += 3;
	    i += 4;
	} else if ( tmp[(int)i] == '"' ) {
	    tmp.replace( i, 1, "&quot;" );
	    len += 5;
	    i += 6;
	} else if ( tmp[(int)i] == '&' ) {
	    tmp.replace( i, 1, "&amp;" );
	    len += 4;
	    i += 5;
	} else {
	    ++i;
	}
    }

    return tmp;
}

void QDOM_AttrPrivate::save( QTextStream& s, int ) const
{
    s << name << "=\"" << encodeAttr( value ) << "\"";
}

/**************************************************************
 *
 * QDomAttr
 *
 **************************************************************/

#define IMPL ((QDOM_AttrPrivate*)impl)

/*!
  \class QDomAttr qdom.h
  \brief The QDomAttr class represents one attribute of a QDomElement

  \module XML

  For example, the following piece of XML gives an element with no children,
  but two attributes:

  \code
  <link href="http://www.trolltech.com" color="red" />
  \endcode

  One can use the attributes of an element with code similar to:

  \code
  QDomElement e = ....;
  QDomAttr a = e.attributeNode( "href" );
  cout << a.value() << endl // gives "http://www.trolltech.com"
  a.setValue( "http://doc.trolltech.com" );
  QDomAttr a2 = e.attributeNode( "href" );
  cout << a2.value() << endl // gives "http://doc.trolltech.com"
  \endcode

  This example also shows that changing an attribute received from an element
  changes the attribute of the element. If you do not want to change the
  value of the element's attribute you have to use cloneNode() to get an
  independent copy of the attribute.

  For further information about the Document Objct Model see
  <a href="http://www.w3.org/TR/REC-DOM-Level-1/">http://www.w3.org/TR/REC-DOM-Level-1/</a>.
  For a more general introduction of the DOM implementation see the
  QDomDocument documentation.
*/


/*!
  Constructs an empty attribute.
*/
QDomAttr::QDomAttr()
{
}

/*!
  Copy constructor.

  The data of the copy is shared: modifying one will also change the other. If
  you want to make a real copy, use cloneNode() instead.
*/
QDomAttr::QDomAttr( const QDomAttr& x )
    : QDomNode( x )
{
}

/*!
  \internal
*/
QDomAttr::QDomAttr( QDOM_AttrPrivate* n )
    : QDomNode( n )
{
}

/*!
  Assignment operator.

  The data of the copy is shared: modifying one will also change the other. If
  you want to make a real copy, use cloneNode() instead.
*/
QDomAttr& QDomAttr::operator= ( const QDomAttr& x )
{
    return (QDomAttr&) QDomNode::operator=( x );
}

/*!
  Destructor.
*/
QDomAttr::~QDomAttr()
{
}

/*!
  Returns the name of the attribute.
*/
QString QDomAttr::name() const
{
    if ( !impl )
	return QString::null;
    return impl->nodeName();
}

/*!
  Returns TRUE if the attribute has been expicitly specified in the XML
  document or was set by the user with setValue(), otherwise FALSE.

  \sa setValue()
*/
bool QDomAttr::specified() const
{
    if ( !impl )
	return FALSE;
    return IMPL->specified();
}

/*!
  Returns the current value of the attribute. Returns a null string
  when the attribute has not been specified.

  \sa specified() setValue()
*/
QString QDomAttr::value() const
{
    if ( !impl )
	return QString::null;
    return impl->nodeValue();
}

/*!
  Sets the value of the attribute to \a v.

  \sa value()
*/
void QDomAttr::setValue( const QString& v )
{
    if ( !impl )
	return;
    impl->setNodeValue( v );
    IMPL->m_specified = TRUE;
}

/*!
  Returns \c AttributeNode.
*/
QDomNode::NodeType QDomAttr::nodeType() const
{
    return AttributeNode;
}

/*!
  Returns TRUE.
*/
bool QDomAttr::isAttr() const
{
    return TRUE;
}

#undef IMPL

/*==============================================================*/
/*                        Element                               */
/*==============================================================*/

/**************************************************************
 *
 * QDOM_ElementPrivate
 *
 **************************************************************/

static void qNormalizeElement( QDOM_NodePrivate* n )
{
    QDOM_NodePrivate* p = n->first;
    QDOM_TextPrivate* t = 0;

    while ( p ) {
	if ( p->isText() ) {
	    if ( t ) {
		QDOM_NodePrivate* tmp = p->next;
		t->appendData( p->nodeValue() );
		n->removeChild( p );
		p = tmp;
	    } else {
		t = (QDOM_TextPrivate*)p;
		p = p->next;
	    }
	} else {
	    p = p->next;
	    t = 0;
	}
    }
}


class QDOM_ElementPrivate : public QDOM_NodePrivate
{
public:
    QDOM_ElementPrivate( QDOM_DocumentPrivate*, QDOM_NodePrivate* parent, const QString& name );
    QDOM_ElementPrivate( QDOM_ElementPrivate* n, bool deep );
    ~QDOM_ElementPrivate();

    virtual QString   attribute( const QString& name,  const QString& defValue ) const;
    virtual void      setAttribute( const QString& name, const QString& value );
    virtual void      removeAttribute( const QString& name );
    virtual QDOM_AttrPrivate* attributeNode( const QString& name);
    virtual QDOM_AttrPrivate* setAttributeNode( QDOM_AttrPrivate* newAttr );
    virtual QDOM_AttrPrivate* removeAttributeNode( QDOM_AttrPrivate* oldAttr );
    virtual bool hasAttribute( const QString& name );
    virtual void normalize();

    QString text();

    // Overloaded from QDOM_NodePrivate
    virtual QDOM_NamedNodeMapPrivate* attributes() { return m_attr; }
    virtual bool isElement() { return TRUE; }
    virtual QDOM_NodePrivate* cloneNode( bool deep = TRUE );
    virtual void save( QTextStream& s, int ) const;

    // Variables
    QDOM_NamedNodeMapPrivate* m_attr;
};

QDOM_ElementPrivate::QDOM_ElementPrivate( QDOM_DocumentPrivate* d, QDOM_NodePrivate* p,
					  const QString& tagname )
    : QDOM_NodePrivate( d, p )
{
    name = tagname;
    m_attr = new QDOM_NamedNodeMapPrivate( this );
}

QDOM_ElementPrivate::QDOM_ElementPrivate( QDOM_ElementPrivate* n, bool deep ) :
    QDOM_NodePrivate( n, deep )
{
    m_attr = n->m_attr->clone( this );
    // Reference is down to 0, so we set it to 1 here.
    m_attr->ref();
}

QDOM_ElementPrivate::~QDOM_ElementPrivate()
{
    // qDebug("~Element=%s", nodeName().latin1() );
    if ( m_attr->deref() )
	delete m_attr;
}

QDOM_NodePrivate* QDOM_ElementPrivate::cloneNode( bool deep)
{
    QDOM_NodePrivate* p = new QDOM_ElementPrivate( this, deep );
    // We are not interested in this node
    p->deref();
    return p;
}

QString QDOM_ElementPrivate::attribute( const QString& name_, const QString& defValue ) const
{
    QDOM_NodePrivate* n = m_attr->namedItem( name_ );
    if ( !n )
	return defValue;

    return n->nodeValue();
}

void QDOM_ElementPrivate::setAttribute( const QString& aname, const QString& newValue )
{
    removeAttribute( aname );

    QDOM_NodePrivate* n = new QDOM_AttrPrivate( ownerDocument(), this, aname );
    n->setNodeValue( newValue );

    // Referencing is done by the map, so we set the reference
    // counter back to 0 here. This is ok since we created the QDOM_AttrPrivate.
    n->deref();
    m_attr->setNamedItem( n );
}

void QDOM_ElementPrivate::removeAttribute( const QString& aname )
{
    QDOM_NodePrivate* p = m_attr->removeNamedItem( aname );
    if ( p && p->count == 0 )
	delete p;
}

QDOM_AttrPrivate* QDOM_ElementPrivate::attributeNode( const QString& aname )
{
    return (QDOM_AttrPrivate*)m_attr->namedItem( aname );
}

QDOM_AttrPrivate* QDOM_ElementPrivate::setAttributeNode( QDOM_AttrPrivate* newAttr )
{
    QDOM_NodePrivate* n = m_attr->namedItem( newAttr->nodeName() );

    // Referencing is done by the maps
    m_attr->setNamedItem( newAttr );

    return (QDOM_AttrPrivate*)n;
}

QDOM_AttrPrivate* QDOM_ElementPrivate::removeAttributeNode( QDOM_AttrPrivate* oldAttr )
{
    return (QDOM_AttrPrivate*)m_attr->removeNamedItem( oldAttr->nodeName() );
}

bool QDOM_ElementPrivate::hasAttribute( const QString& aname )
{
    return m_attr->contains( aname );
}

void QDOM_ElementPrivate::normalize()
{
    qNormalizeElement( this );
}

QString QDOM_ElementPrivate::text()
{
    QString t( "" );

    QDOM_NodePrivate* p = first;
    while ( p ) {
	if ( p->isText() || p->isCDATASection() )
	    t += p->nodeValue();
	else if ( p->isElement() )
	    t += ((QDOM_ElementPrivate*)p)->text();
	p = p->next;
    }

    return t;
}

void QDOM_ElementPrivate::save( QTextStream& s, int indent ) const
{
    for ( int i = 0; i < indent; ++i )
	s << " ";

    s << "<" << name;

    if ( !m_attr->map.isEmpty() ) {
	s << " ";
	QDictIterator<QDOM_NodePrivate> it( m_attr->map );
	for ( ; it.current(); ++it ) {
	    it.current()->save( s, 0 );
	    s << " ";
	}
    }

    if ( last ) { // has child nodes
	if ( first->isText() )
	    s << ">";
	else
	    s << ">" << endl;
	QDOM_NodePrivate::save( s, indent + 1 );
 	if ( !last->isText() )
 	    for( int i = 0; i < indent; ++i )
 		s << " ";
	s << "</" << name << ">" << endl;
    } else {
	s << "/>" << endl;
    }
}

/**************************************************************
 *
 * QDomElement
 *
 **************************************************************/

#define IMPL ((QDOM_ElementPrivate*)impl)

/*!
  \class QDomElement qdom.h
  \brief The QDomElement class represents one element in the DOM tree.

  \module XML

  Elements have a name() and zero or more attributes associated with them.

  Attributes of the element are represented by QDomAttr objects, that can be
  queried using the attribute() and attributeNode() functions. You can set
  attributes with the setAttribute() and setAttributeNode() functions.

  For further information about the Document Objct Model see
  <a href="http://www.w3.org/TR/REC-DOM-Level-1/">http://www.w3.org/TR/REC-DOM-Level-1/</a>.
  For a more general introduction of the DOM implementation see the
  QDomDocument documentation.
*/

/*!
  Constructs an empty element. Use the QDomDocument::createElement() function
  to construct elements with content.
*/
QDomElement::QDomElement()
    : QDomNode()
{
}

/*!
  Copy constructor.

  The data of the copy is shared: modifying one will also change the other. If
  you want to make a real copy, use cloneNode() instead.
*/
QDomElement::QDomElement( const QDomElement& x )
    : QDomNode( x )
{
}

/*!
  \internal
*/
QDomElement::QDomElement( QDOM_ElementPrivate* n )
    : QDomNode( n )
{
}

/*!
  Assignment operator.

  The data of the copy is shared: modifying one will also change the other. If
  you want to make a real copy, use cloneNode() instead.
*/
QDomElement& QDomElement::operator= ( const QDomElement& x )
{
    return (QDomElement&) QDomNode::operator=( x );
}

/*!
  Destructor.
*/
QDomElement::~QDomElement()
{
}

/*!
  Returns \c ElementNode.
*/
QDomNode::NodeType QDomElement::nodeType() const
{
    return ElementNode;
}

/*!
  Sets the tag name of this element.

  \sa tagName()
*/
void QDomElement::setTagName( const QString& name )
{
    if ( impl )
	impl->name = name;
}

/*!
  Returns the tag name of this element. For an XML element like
  \code
  <img src="myimg.png">
  \endcode
  the tagname would return "img".

  \sa setTagName()
*/
QString QDomElement::tagName() const
{
    if ( !impl )
	return QString::null;
    return impl->nodeName();
}

/*!
  Returns the attribute with the name \a name. If the attribute does not exist
  \a defValue is returned.

  \sa setAttribute() attributeNode() setAttributeNode()
*/
QString QDomElement::attribute( const QString& name,  const QString& defValue ) const
{
    if ( !impl )
	return defValue;
    return IMPL->attribute( name, defValue );
}

/*!
  Sets the attribute with the name \a name to the string \a value.  If the
  attribute does not exist, a new one is created.
*/
void QDomElement::setAttribute( const QString& name, const QString& value )
{
    if ( !impl )
	return;
    IMPL->setAttribute( name, value );
}

/*!
  \overload
*/
void QDomElement::setAttribute( const QString& name, int value )
{
    if ( !impl )
	return;
    QString x;
    x.setNum( value );
    IMPL->setAttribute( name, x );
}

/*!
  \overload
*/
void QDomElement::setAttribute( const QString& name, uint value )
{
    if ( !impl )
	return;
    QString x;
    x.setNum( value );
    IMPL->setAttribute( name, x );
}

/*!
  \overload
*/
void QDomElement::setAttribute( const QString& name, double value )
{
    if ( !impl )
	return;
    QString x;
    x.setNum( value );
    IMPL->setAttribute( name, x );
}

/*!
  Removes the attribute with the name \a name from this element.

  \sa setAttribute() attribute()
*/
void QDomElement::removeAttribute( const QString& name )
{
    if ( !impl )
	return;
    IMPL->removeAttribute( name );
}

/*!
  Returns the QDomAttr object that corresponds to the attribute with the name
  \a name.  If no such attribute exists a null object is returned.

  \sa setAttributeNode() attribute() setAttribute()
*/
QDomAttr QDomElement::attributeNode( const QString& name)
{
    if ( !impl )
	return QDomAttr();
    return QDomAttr( IMPL->attributeNode( name ) );
}

/*!
  Adds the attribute \a newAttr to this element.

  If an attribute with the name \a newAttr exists in the element, the function
  returns this attribute; otherwise the function returns a null attribute.

  \sa attributeNode()
*/
QDomAttr QDomElement::setAttributeNode( const QDomAttr& newAttr )
{
    if ( !impl )
	return QDomAttr();
    return QDomAttr( IMPL->setAttributeNode( ((QDOM_AttrPrivate*)newAttr.impl) ) );
}

/*!
  Removes the attribute \a oldAttr from the element and returns it.

  \sa attributeNode() setAttributeNode()
*/
QDomAttr QDomElement::removeAttributeNode( const QDomAttr& oldAttr )
{
    if ( !impl )
	return QDomAttr(); // ### should this return oldAttr?
    return QDomAttr( IMPL->removeAttributeNode( ((QDOM_AttrPrivate*)oldAttr.impl) ) );
}

/*!
  Returns a QDomNodeList containing all descendant elements of this element
  with the name \a tagname. The order they are in the node list, is the order
  they are encountered in a preorder traversal of the element tree.
*/
QDomNodeList QDomElement::elementsByTagName( const QString& tagname ) const
{
    return QDomNodeList( new QDOM_NodeListPrivate( impl, tagname ) );
}


/*!
  Calling normalize() on an element brings all its children into a standard
  form. This means, that adjacent QDomText objects will be merged to
  one text object (QDomCDATASection nodes are not merged).
*/
void QDomElement::normalize()
{
    if ( !impl )
	return;
    IMPL->normalize();
}

/*!
  Returns TRUE.
*/
bool QDomElement::isElement() const
{
    return TRUE;
}

/*!
  Returns a QDomNamedNodeMap containing all attributes for this element.

  \sa attribute() setAttribute() attributeNode() setAttributeNode()
*/
QDomNamedNodeMap QDomElement::attributes() const
{
    if ( !impl )
	return QDomNamedNodeMap();
    return QDomNamedNodeMap( IMPL->attributes() );
}

/*!
  Returns TRUE is this element has an attribute with the name \a name,
  otherwise FALSE.
*/
bool QDomElement::hasAttribute( const QString& name ) const
{
    if ( !impl )
	return FALSE;
    return IMPL->hasAttribute( name );
}

/*!
  Returns the text contained inside this element.

  Example:
  \code
  <h1>Hello <b>Qt</b> <![CDATA[<xml is cool>]]></h1>
  \endcode

  The function text() of the QDomElement for the &lt;h1&gt; tag, 
  will return "Hello Qt &lt;xml is cool&gt;".

  Comments are ignored by this function. It evaluates only
  QDomText and QDomCDATASection objects.
*/
QString QDomElement::text() const
{
    if ( !impl )
	return QString::null;
    return IMPL->text();
}

#undef IMPL

/*==============================================================*/
/*                          Text                                */
/*==============================================================*/

/**************************************************************
 *
 * QDOM_TextPrivate
 *
 **************************************************************/

QString* QDOM_TextPrivate::textName = 0;

QDOM_TextPrivate::QDOM_TextPrivate( QDOM_DocumentPrivate* d, QDOM_NodePrivate* parent, const QString& value )
    : QDOM_CharacterDataPrivate( d, parent, value )
{
    if ( !textName )
	textName = new QString( "#text" );
    name = *textName;
}

QDOM_TextPrivate::QDOM_TextPrivate( QDOM_TextPrivate* n, bool deep )
    : QDOM_CharacterDataPrivate( n, deep )
{
}

QDOM_TextPrivate::~QDOM_TextPrivate()
{
}

QDOM_NodePrivate* QDOM_TextPrivate::cloneNode( bool deep)
{
    QDOM_NodePrivate* p = new QDOM_TextPrivate( this, deep );
    // We are not interested in this node
    p->deref();
    return p;
}

QDOM_TextPrivate* QDOM_TextPrivate::splitText( int offset )
{
    if ( !parent ) {
	qWarning( "QDomText::splitText  The node has no parent. So I can not split" );
	return 0;
    }

    QDOM_TextPrivate* t = new QDOM_TextPrivate( ownerDocument(), 0, value.mid( offset ) );
    value.truncate( offset );

    parent->insertAfter( t, this );

    return t;
}

void QDOM_TextPrivate::save( QTextStream& s, int ) const
{
    s << encodeAttr( value );
}

/**************************************************************
 *
 * QDomText
 *
 **************************************************************/

#define IMPL ((QDOM_TextPrivate*)impl)

/*!
  \class QDomText qdom.h
  \brief The QDomText class represents textual data in the parsed XML document.

  \module XML

  For further information about the Document Objct Model see
  <a href="http://www.w3.org/TR/REC-DOM-Level-1/">http://www.w3.org/TR/REC-DOM-Level-1/</a>.
  For a more general introduction of the DOM implementation see the
  QDomDocument documentation.
*/

/*!
  Constructs an empty QDomText object.

  To construct a QDomText with content, use QDomDocument::createTextNode().
*/
QDomText::QDomText()
    : QDomCharacterData()
{
}

/*!
  Copy constructor.

  The data of the copy is shared: modifying one will also change the other. If
  you want to make a real copy, use cloneNode() instead.
*/
QDomText::QDomText( const QDomText& x )
    : QDomCharacterData( x )
{
}

/*!
  \internal
*/
QDomText::QDomText( QDOM_TextPrivate* n )
    : QDomCharacterData( n )
{
}

/*!
  Assignment operator.

  The data of the copy is shared: modifying one will also change the other. If
  you want to make a real copy, use cloneNode() instead.
*/
QDomText& QDomText::operator= ( const QDomText& x )
{
    return (QDomText&) QDomNode::operator=( x );
}

/*!
  Destructor.
*/
QDomText::~QDomText()
{
}

/*!
  Returns \c TextNode.
*/
QDomNode::NodeType QDomText::nodeType() const
{
    return TextNode;
}

/*!
  Splits this object at position \a offset into two QDomText objects. The newly
  created object is inserted into the document tree after this object.

  The function returns the newly created object.

  \sa QDomElement::normalize()
*/
QDomText QDomText::splitText( int offset )
{
    if ( !impl )
	return QDomText();
    return QDomText( IMPL->splitText( offset ) );
}

/*!
  Returns TRUE.
*/
bool QDomText::isText() const
{
    return TRUE;
}

#undef IMPL

/*==============================================================*/
/*                          Comment                             */
/*==============================================================*/

/**************************************************************
 *
 * QDOM_CommentPrivate
 *
 **************************************************************/

class QDOM_CommentPrivate : public QDOM_CharacterDataPrivate
{
public:
    QDOM_CommentPrivate( QDOM_DocumentPrivate*, QDOM_NodePrivate* parent, const QString& value );
    QDOM_CommentPrivate( QDOM_CommentPrivate* n, bool deep );
    ~QDOM_CommentPrivate();

    // Overloaded from QDOM_NodePrivate
    virtual QDOM_NodePrivate* cloneNode( bool deep = TRUE );
    bool isComment() { return TRUE; }
    virtual void save( QTextStream& s, int ) const;

    static QString* commentName;
};

QString* QDOM_CommentPrivate::commentName = 0;

QDOM_CommentPrivate::QDOM_CommentPrivate( QDOM_DocumentPrivate* d, QDOM_NodePrivate* parent, const QString& value )
    : QDOM_CharacterDataPrivate( d, parent, value )
{
    if ( !commentName )
	commentName = new QString( "#comment" );
    name = *commentName;
}

QDOM_CommentPrivate::QDOM_CommentPrivate( QDOM_CommentPrivate* n, bool deep )
    : QDOM_CharacterDataPrivate( n, deep )
{
}

QDOM_CommentPrivate::~QDOM_CommentPrivate()
{
}

QDOM_NodePrivate* QDOM_CommentPrivate::cloneNode( bool deep)
{
    QDOM_NodePrivate* p = new QDOM_CommentPrivate( this, deep );
    // We are not interested in this node
    p->deref();
    return p;
}

void QDOM_CommentPrivate::save( QTextStream& s, int ) const
{
    s << "<!--" << value << "-->";
}

/**************************************************************
 *
 * QDomComment
 *
 **************************************************************/

#define IMPL ((QDOM_CommentPrivate*)impl)

/*!
  \class QDomComment qdom.h
  \brief The QDomComment class represents an XML comment.

  \module XML

  A comment in the parsed XML such as
  \code
  <!-- this is a comment -->
  \endcode
  is represented by QDomComment objects in the parsed Dom tree.

  For further information about the Document Objct Model see
  <a href="http://www.w3.org/TR/REC-DOM-Level-1/">http://www.w3.org/TR/REC-DOM-Level-1/</a>.
  For a more general introduction of the DOM implementation see the
  QDomDocument documentation.
*/

/*!
  Constructs an empty comment. To construct a comment with content, use
  the QDomDocument::createComment() function.
*/
QDomComment::QDomComment()
    : QDomCharacterData()
{
}

/*!
  Copy constructor.

  The data of the copy is shared: modifying one will also change the other. If
  you want to make a real copy, use cloneNode() instead.
*/
QDomComment::QDomComment( const QDomComment& x )
    : QDomCharacterData( x )
{
}

/*!
  \internal
*/
QDomComment::QDomComment( QDOM_CommentPrivate* n )
    : QDomCharacterData( n )
{
}

/*!
  Assignment operator.

  The data of the copy is shared: modifying one will also change the other. If
  you want to make a real copy, use cloneNode() instead.
*/
QDomComment& QDomComment::operator= ( const QDomComment& x )
{
    return (QDomComment&) QDomNode::operator=( x );
}

/*!
  Destructor.
*/
QDomComment::~QDomComment()
{
}

/*!
  Returns \c CommentNode.
*/
QDomNode::NodeType QDomComment::nodeType() const
{
    return CommentNode;
}

/*!
  Returns TRUE.
*/
bool QDomComment::isComment() const
{
    return TRUE;
}

#undef IMPL

/*==============================================================*/
/*                        CDATASection                          */
/*==============================================================*/

/**************************************************************
 *
 * QDOM_CDATASectionPrivate
 *
 **************************************************************/

class QDOM_CDATASectionPrivate : public QDOM_TextPrivate
{
public:
    QDOM_CDATASectionPrivate( QDOM_DocumentPrivate*, QDOM_NodePrivate* parent, const QString& value );
    QDOM_CDATASectionPrivate( QDOM_CDATASectionPrivate* n, bool deep );
    ~QDOM_CDATASectionPrivate();

    // Overloaded from QDOM_NodePrivate
    virtual QDOM_NodePrivate* cloneNode( bool deep = TRUE );
    virtual bool isCDATASection() { return TRUE; }
    virtual void save( QTextStream& s, int ) const;

    static QString* cdataName;
};

QString* QDOM_CDATASectionPrivate::cdataName = 0;

QDOM_CDATASectionPrivate::QDOM_CDATASectionPrivate( QDOM_DocumentPrivate* d, QDOM_NodePrivate* parent,
						    const QString& value )
    : QDOM_TextPrivate( d, parent, value )
{
    if ( !cdataName )
	cdataName = new QString( "#cdata-section" );
    name = *cdataName;
}

QDOM_CDATASectionPrivate::QDOM_CDATASectionPrivate( QDOM_CDATASectionPrivate* n, bool deep )
    : QDOM_TextPrivate( n, deep )
{
}

QDOM_CDATASectionPrivate::~QDOM_CDATASectionPrivate()
{
}

QDOM_NodePrivate* QDOM_CDATASectionPrivate::cloneNode( bool deep)
{
    QDOM_NodePrivate* p = new QDOM_CDATASectionPrivate( this, deep );
    // We are not interested in this node
    p->deref();
    return p;
}

void QDOM_CDATASectionPrivate::save( QTextStream& s, int ) const
{
    // #### How do we escape "]]>" ?
    s << "<![CDATA[" << value << "]]>";
}

/**************************************************************
 *
 * QDomCDATASection
 *
 **************************************************************/

#define IMPL ((QDOM_CDATASectionPrivate*)impl)

/*!
  \class QDomCDATASection qdom.h
  \brief The QDomCDATASection class represents an XML CDATA section.

  \module XML

  CDATA sections are used to escape blocks of text containing
  characters that would otherwise be regarded as markup. The only
  delimiter that is recognized in a CDATA section is the "]]&gt;"
  string that ends the CDATA section. CDATA sections can not be
  nested. The primary purpose is for including material such as XML
  fragments, without needing to escape all the delimiters.

  Adjacent QDomCDATASection nodes are not merged by the
  QDomElement.normalize() function.

  For further information about the Document Objct Model see
  <a href="http://www.w3.org/TR/REC-DOM-Level-1/">http://www.w3.org/TR/REC-DOM-Level-1/</a>.
  For a more general introduction of the DOM implementation see the
  QDomDocument documentation.
*/

/*!
  Constructs an empty CDATA section. To create a CDATA section with content,
  use the QDomDocument::createCDATASection() function.
*/
QDomCDATASection::QDomCDATASection()
    : QDomText()
{
}

/*!
  Copy constructor.

  The data of the copy is shared: modifying one will also change the other. If
  you want to make a real copy, use cloneNode() instead.
*/
QDomCDATASection::QDomCDATASection( const QDomCDATASection& x )
    : QDomText( x )
{
}

/*!
  \internal
*/
QDomCDATASection::QDomCDATASection( QDOM_CDATASectionPrivate* n )
    : QDomText( n )
{
}

/*!
  Assigment operator.

  The data of the copy is shared: modifying one will also change the other. If
  you want to make a real copy, use cloneNode() instead.
*/
QDomCDATASection& QDomCDATASection::operator= ( const QDomCDATASection& x )
{
    return (QDomCDATASection&) QDomNode::operator=( x );
}

/*!
  Destructor.
*/
QDomCDATASection::~QDomCDATASection()
{
}

/*!
  Returns \c CDATASection.
*/
QDomNode::NodeType QDomCDATASection::nodeType() const
{
    return CDATASectionNode;
}

/*!
  Returns TRUE
*/
bool QDomCDATASection::isCDATASection() const
{
    return TRUE;
}

#undef IMPL

/*==============================================================*/
/*                          Notation                            */
/*==============================================================*/

/**************************************************************
 *
 * QDOM_NotationPrivate
 *
 **************************************************************/

class QDOM_NotationPrivate : public QDOM_NodePrivate
{
public:
    QDOM_NotationPrivate( QDOM_DocumentPrivate*, QDOM_NodePrivate* parent, const QString& name,
			  const QString& pub, const QString& sys );
    QDOM_NotationPrivate( QDOM_NotationPrivate* n, bool deep );
    ~QDOM_NotationPrivate();

    // Overloaded from QDOM_NodePrivate
    virtual QDOM_NodePrivate* cloneNode( bool deep = TRUE );
    virtual bool isNotation() { return TRUE; }
    virtual void save( QTextStream& s, int ) const;

    // Variables
    QString m_sys;
    QString m_pub;
};

QDOM_NotationPrivate::QDOM_NotationPrivate( QDOM_DocumentPrivate* d, QDOM_NodePrivate* parent,
					    const QString& aname,
					    const QString& pub, const QString& sys )
    : QDOM_NodePrivate( d, parent )
{
    name = aname;
    m_pub = pub;
    m_sys = sys;
}

QDOM_NotationPrivate::QDOM_NotationPrivate( QDOM_NotationPrivate* n, bool deep )
    : QDOM_NodePrivate( n, deep )
{
    m_sys = n->m_sys;
    m_pub = n->m_pub;
}

QDOM_NotationPrivate::~QDOM_NotationPrivate()
{
}

QDOM_NodePrivate* QDOM_NotationPrivate::cloneNode( bool deep)
{
    QDOM_NodePrivate* p = new QDOM_NotationPrivate( this, deep );
    // We are not interested in this node
    p->deref();
    return p;
}

void QDOM_NotationPrivate::save( QTextStream& s, int ) const
{
    s << "<!NOTATION " << name << " ";
    if ( !m_pub.isEmpty() )  {
	s << "PUBLIC \"" << m_pub << "\"";
	if ( !m_sys.isEmpty() )
	    s << " \"" << m_sys << "\"";
    }  else {
	s << "SYSTEM \"" << m_sys << "\"";
    }
    s << ">";
}

/**************************************************************
 *
 * QDomNotation
 *
 **************************************************************/

#define IMPL ((QDOM_NotationPrivate*)impl)

/*!
  \class QDomNotation qdom.h
  \brief The QDomNotation class represents an XML notation.

  \module XML

  A notation either declares, by name, the format of an unparsed entity
  (see section 4.7 of the XML 1.0 specification), or is used for
  formal declaration of processing instruction targets (see section
  2.6 of the XML 1.0 specification).

  DOM does not support editing notation nodes; they are therefore readonly.

  A notation node does not have any parent.

  For further information about the Document Objct Model see
  <a href="http://www.w3.org/TR/REC-DOM-Level-1/">http://www.w3.org/TR/REC-DOM-Level-1/</a>.
  For a more general introduction of the DOM implementation see the
  QDomDocument documentation.
*/


/*!
  Constructor.
*/
QDomNotation::QDomNotation()
    : QDomNode()
{
}

/*!
  Copy constructor.

  The data of the copy is shared: modifying one will also change the other. If
  you want to make a real copy, use cloneNode() instead.
*/
QDomNotation::QDomNotation( const QDomNotation& x )
    : QDomNode( x )
{
}

/*!
  \internal
*/
QDomNotation::QDomNotation( QDOM_NotationPrivate* n )
    : QDomNode( n )
{
}

/*!
  Assignment operator.

  The data of the copy is shared: modifying one will also change the other. If
  you want to make a real copy, use cloneNode() instead.
*/
QDomNotation& QDomNotation::operator= ( const QDomNotation& x )
{
    return (QDomNotation&) QDomNode::operator=( x );
}

/*!
  Destructor.
*/
QDomNotation::~QDomNotation()
{
}

/*!
  Returns \c NotationNode.
*/
QDomNode::NodeType QDomNotation::nodeType() const
{
    return NotationNode;
}

/*!
  Returns the public identifier of this notation.
*/
QString QDomNotation::publicId() const
{
    if ( !impl )
	return QString::null;
    return IMPL->m_pub;
}

/*!
  Returns the system identifier of this notation.
*/
QString QDomNotation::systemId() const
{
    if ( !impl )
	return QString::null;
    return IMPL->m_sys;
}

/*!
  Returns TRUE.
*/
bool QDomNotation::isNotation() const
{
    return TRUE;
}

#undef IMPL


/*==============================================================*/
/*                          Entity                            */
/*==============================================================*/

/**************************************************************
 *
 * QDOM_EntityPrivate
 *
 **************************************************************/

class QDOM_EntityPrivate : public QDOM_NodePrivate
{
public:
    QDOM_EntityPrivate( QDOM_DocumentPrivate*, QDOM_NodePrivate* parent, const QString& name,
			const QString& pub, const QString& sys, const QString& notation );
    QDOM_EntityPrivate( QDOM_EntityPrivate* n, bool deep );
    ~QDOM_EntityPrivate();

    // Overloaded from QDOM_NodePrivate
    virtual QDOM_NodePrivate* cloneNode( bool deep = TRUE );
    virtual bool isEntity() { return TRUE; }
    virtual void save( QTextStream& s, int ) const;

    // Variables
    QString m_sys;
    QString m_pub;
    QString m_notationName;
};

QDOM_EntityPrivate::QDOM_EntityPrivate( QDOM_DocumentPrivate* d, QDOM_NodePrivate* parent,
					const QString& aname,
					const QString& pub, const QString& sys, const QString& notation )
    : QDOM_NodePrivate( d, parent )
{
    name = aname;
    m_pub = pub;
    m_sys = sys;
    m_notationName = notation;
}

QDOM_EntityPrivate::QDOM_EntityPrivate( QDOM_EntityPrivate* n, bool deep )
    : QDOM_NodePrivate( n, deep )
{
    m_sys = n->m_sys;
    m_pub = n->m_pub;
    m_notationName = n->m_notationName;
}

QDOM_EntityPrivate::~QDOM_EntityPrivate()
{
}

QDOM_NodePrivate* QDOM_EntityPrivate::cloneNode( bool deep)
{
    QDOM_NodePrivate* p = new QDOM_EntityPrivate( this, deep );
    // We are not interested in this node
    p->deref();
    return p;
}

/*
  Encode an entity value upon saving.
*/
static QCString encodeEntity( const QCString& str )
{
    QCString tmp( str );
    uint len = tmp.length();
    uint i = 0;
    const char* d = tmp.data();
    while ( i < len ) {
	if ( d[i] == '%' ){
	    tmp.replace( i, 1, "&#60;" );
	    d = tmp.data();
	    len += 4;
	    i += 5;
	}
	else if ( d[i] == '"' ) {
	    tmp.replace( i, 1, "&#34;" );
	    d = tmp.data();
	    len += 4;
	    i += 5;
	} else if ( d[i] == '&' && i + 1 < len && d[i+1] == '#' ) {
	    // Dont encode &lt; or &quot; or &custom;.
	    // Only encode character references
	    tmp.replace( i, 1, "&#38;" );
	    d = tmp.data();
	    len += 4;
	    i += 5;
	} else {
	    ++i;
	}
    }

    return tmp;
}

void QDOM_EntityPrivate::save( QTextStream& s, int ) const
{
    if ( m_sys.isEmpty() && m_pub.isEmpty() ) {
	s << "<!ENTITY " << name << " \"" << encodeEntity( value.utf8() ) << "\">";
    } else {
	s << "<!ENTITY " << name << " ";
	if ( m_pub.isEmpty() )
	    s << "SYSTEM \"" << m_sys << "\"";
	else
	    s << "PUBLIC \"" << m_pub << "\" \"" << m_sys << "\"";
	if ( !m_notationName.isEmpty() )
	    s << "NDATA" << m_notationName;
	s << ">";
    }
}

/**************************************************************
 *
 * QDomEntity
 *
 **************************************************************/

#define IMPL ((QDOM_EntityPrivate*)impl)

/*!
  \class QDomEntity qdom.h
  \brief The QDomEntity class represents an XML entity.

  \module XML

  This class represents an entity in an XML document, either parsed or
  unparsed. Note that this models the entity itself not the entity declaration.

  DOM does not support editing entity nodes; if a user wants to make changes to
  the contents of an entity, every related QDomEntityReference node has to be
  replaced in the DOM tree by a clone of the entity's contents, and then
  the desired changes must be made to each of those clones instead. All the
  descendants of an entity node are readonly.

  An entity node does not have any parent.

  For further information about the Document Objct Model see
  <a href="http://www.w3.org/TR/REC-DOM-Level-1/">http://www.w3.org/TR/REC-DOM-Level-1/</a>.
  For a more general introduction of the DOM implementation see the
  QDomDocument documentation.
*/


/*!
  Constructs an empty entity.
*/
QDomEntity::QDomEntity()
    : QDomNode()
{
}


/*!
  Copy constructor.

  The data of the copy is shared: modifying one will also change the other. If
  you want to make a real copy, use cloneNode() instead.
*/
QDomEntity::QDomEntity( const QDomEntity& x )
    : QDomNode( x )
{
}

/*!
  \internal
*/
QDomEntity::QDomEntity( QDOM_EntityPrivate* n )
    : QDomNode( n )
{
}

/*!
  Assignment operator.

  The data of the copy is shared: modifying one will also change the other. If
  you want to make a real copy, use cloneNode() instead.
*/
QDomEntity& QDomEntity::operator= ( const QDomEntity& x )
{
    return (QDomEntity&) QDomNode::operator=( x );
}

/*!
  Destructor.
*/
QDomEntity::~QDomEntity()
{
}

/*!
  Returns \c EntityNode.
*/
QDomNode::NodeType QDomEntity::nodeType() const
{
    return EntityNode;
}

/*!
  Returns the public identifier associated with this entity.
  If the public identifier was not specified QString::null is returned.
*/
QString QDomEntity::publicId() const
{
    if ( !impl )
	return QString::null;
    return IMPL->m_pub;
}

/*!
  Returns the system identifier associated with this entity.
  If the system identifier was not specified QString::null is returned.
*/
QString QDomEntity::systemId() const
{
    if ( !impl )
	return QString::null;
    return IMPL->m_sys;
}

/*!
  For unparsed entities this function returns the name of the notation for the
  entity. For parsed entities this function returns QString::null.
*/
QString QDomEntity::notationName() const
{
    if ( !impl )
	return QString::null;
    return IMPL->m_notationName;
}

/*!
  Returns TRUE.
*/
bool QDomEntity::isEntity() const
{
    return TRUE;
}

#undef IMPL


/*==============================================================*/
/*                      EntityReference                         */
/*==============================================================*/

/**************************************************************
 *
 * QDOM_EntityReferencePrivate
 *
 **************************************************************/

class QDOM_EntityReferencePrivate : public QDOM_NodePrivate
{
public:
    QDOM_EntityReferencePrivate( QDOM_DocumentPrivate*, QDOM_NodePrivate* parent, const QString& name );
    QDOM_EntityReferencePrivate( QDOM_NodePrivate* n, bool deep );
    ~QDOM_EntityReferencePrivate();

    // Overloaded from QDOM_NodePrivate
    virtual QDOM_NodePrivate* cloneNode( bool deep = TRUE );
    virtual bool isEntityReference() { return TRUE; }
    virtual void save( QTextStream& s, int ) const;
};

QDOM_EntityReferencePrivate::QDOM_EntityReferencePrivate( QDOM_DocumentPrivate* d, QDOM_NodePrivate* parent, const QString& aname )
    : QDOM_NodePrivate( d, parent )
{
    name = aname;
}

QDOM_EntityReferencePrivate::QDOM_EntityReferencePrivate( QDOM_NodePrivate* n, bool deep )
    : QDOM_NodePrivate( n, deep )
{
}

QDOM_EntityReferencePrivate::~QDOM_EntityReferencePrivate()
{
}

QDOM_NodePrivate* QDOM_EntityReferencePrivate::cloneNode( bool deep)
{
    QDOM_NodePrivate* p = new QDOM_EntityReferencePrivate( this, deep );
    // We are not interested in this node
    p->deref();
    return p;
}

void QDOM_EntityReferencePrivate::save( QTextStream& s, int ) const
{
    s << "&" << name << ";";
}

/**************************************************************
 *
 * QDomEntityReference
 *
 **************************************************************/

#define IMPL ((QDOM_EntityReferencePrivate*)impl)

/*!
  \class QDomEntityReference qdom.h
  \brief The QDomEntityReference class represents an XML entity reference.

  \module XML

  A QDomEntityReference object may be inserted into the
  DOM tree when an entity reference is in the source document,
  or when the user wishes to insert an entity reference.

  Note that character references and references to predefined entities are
  expanded by the XML processor so that characters are represented by their
  Unicode equivalent rather than by an entity reference.

  Moreover, the XML processor may completely expand references to entities
  while building the DOM tree, instead of providing QDomEntityReference
  objects.

  If it does provide such objects, then for a given entity reference node, it
  may be that there is no entity node representing the referenced entity; but
  if such an entity exists, then the child list of the entity reference node is
  the same as that of the entity  node.  As with the entity node, all
  descendants of the entity reference are readonly.

  For further information about the Document Objct Model see
  <a href="http://www.w3.org/TR/REC-DOM-Level-1/">http://www.w3.org/TR/REC-DOM-Level-1/</a>.
  For a more general introduction of the DOM implementation see the
  QDomDocument documentation.
*/



/*!
  Constructs an empty entity reference. Use
  QDomDocument::createEntityReference() to create a entity reference with
  content.
*/
QDomEntityReference::QDomEntityReference()
    : QDomNode()
{
}

/*!
  Copy constructor.

  The data of the copy is shared: modifying one will also change the other. If
  you want to make a real copy, use cloneNode() instead.
*/
QDomEntityReference::QDomEntityReference( const QDomEntityReference& x )
    : QDomNode( x )
{
}

/*!
  \internal
*/
QDomEntityReference::QDomEntityReference( QDOM_EntityReferencePrivate* n )
    : QDomNode( n )
{
}

/*!
  Assignment operator.

  The data of the copy is shared: modifying one will also change the other. If
  you want to make a real copy, use cloneNode() instead.
*/
QDomEntityReference& QDomEntityReference::operator= ( const QDomEntityReference& x )
{
    return (QDomEntityReference&) QDomNode::operator=( x );
}

/*!
  Destructor.
*/
QDomEntityReference::~QDomEntityReference()
{
}

/*!
  Returns \c EntityReference.
*/
QDomNode::NodeType QDomEntityReference::nodeType() const
{
    return EntityReferenceNode;
}

/*!
  Returns TRUE.
*/
bool QDomEntityReference::isEntityReference() const
{
    return TRUE;
}

#undef IMPL


/*==============================================================*/
/*                      ProcessingInstruction                   */
/*==============================================================*/

/**************************************************************
 *
 * QDOM_ProcessingInstructionPrivate
 *
 **************************************************************/

class QDOM_ProcessingInstructionPrivate : public QDOM_NodePrivate
{
public:
    QDOM_ProcessingInstructionPrivate( QDOM_DocumentPrivate*, QDOM_NodePrivate* parent, const QString& target,
				       const QString& data);
    QDOM_ProcessingInstructionPrivate( QDOM_ProcessingInstructionPrivate* n, bool deep );
    ~QDOM_ProcessingInstructionPrivate();

    // Overloaded from QDOM_NodePrivate
    virtual QDOM_NodePrivate* cloneNode( bool deep = TRUE );
    virtual bool isProcessingInstruction() { return TRUE; }
    virtual void save( QTextStream& s, int ) const;
};

QDOM_ProcessingInstructionPrivate::QDOM_ProcessingInstructionPrivate( QDOM_DocumentPrivate* d,
								      QDOM_NodePrivate* parent,
								      const QString& target,
								      const QString& data )
    : QDOM_NodePrivate( d, parent )
{
    name = target;
    value = data;
}

QDOM_ProcessingInstructionPrivate::QDOM_ProcessingInstructionPrivate( QDOM_ProcessingInstructionPrivate* n, bool deep )
    : QDOM_NodePrivate( n, deep )
{
}

QDOM_ProcessingInstructionPrivate::~QDOM_ProcessingInstructionPrivate()
{
}

QDOM_NodePrivate* QDOM_ProcessingInstructionPrivate::cloneNode( bool deep)
{
    QDOM_NodePrivate* p = new QDOM_ProcessingInstructionPrivate( this, deep );
    // We are not interested in this node
    p->deref();
    return p;
}

void QDOM_ProcessingInstructionPrivate::save( QTextStream& s, int ) const
{
    s << "<?" << name << " " << value << "?>";
}

/**************************************************************
 *
 * QDomProcessingInstruction
 *
 **************************************************************/

#define IMPL ((QDOM_ProcessingInstructionPrivate*)impl)

/*!
  \class QDomProcessingInstruction qdom.h
  \brief The QDomProcessingInstruction class represents an XML processing
  instruction.

  \module XML

  Processing instructions are used in XML as a way to keep processor-specific
  information in the text of the document.

  For further information about the Document Objct Model see
  <a href="http://www.w3.org/TR/REC-DOM-Level-1/">http://www.w3.org/TR/REC-DOM-Level-1/</a>.
  For a more general introduction of the DOM implementation see the
  QDomDocument documentation.
*/

/*!
  Constructs an empty processing instruction. Use
  QDomDocument::createProcessingInstruction() to create a processing
  instruction with content.
*/
QDomProcessingInstruction::QDomProcessingInstruction()
    : QDomNode()
{
}

/*!
  Copy constructor.

  The data of the copy is shared: modifying one will also change the other. If
  you want to make a real copy, use cloneNode() instead.
*/
QDomProcessingInstruction::QDomProcessingInstruction( const QDomProcessingInstruction& x )
    : QDomNode( x )
{
}

/*!
  \internal
*/
QDomProcessingInstruction::QDomProcessingInstruction( QDOM_ProcessingInstructionPrivate* n )
    : QDomNode( n )
{
}

/*!
  Assignment operator.

  The data of the copy is shared: modifying one will also change the other. If
  you want to make a real copy, use cloneNode() instead.
*/
QDomProcessingInstruction& QDomProcessingInstruction::operator= ( const QDomProcessingInstruction& x )
{
    return (QDomProcessingInstruction&) QDomNode::operator=( x );
}

/*!
  Destructor.
*/
QDomProcessingInstruction::~QDomProcessingInstruction()
{
}

/*!
  Returns \c ProcessingInstructionNode.
*/
QDomNode::NodeType QDomProcessingInstruction::nodeType() const
{
    return ProcessingInstructionNode;
}

/*!
  Returns the target of this processing instruction.

  \sa data()
*/
QString QDomProcessingInstruction::target() const
{
    if ( !impl )
	return QString::null;
    return impl->nodeName();
}

/*!
  Returns the content of this processing instruction.

  \sa setData() target()
*/
QString QDomProcessingInstruction::data() const
{
    if ( !impl )
	return QString::null;
    return impl->nodeValue();
}

/*!
  Sets the data contained in the processing instruction.

  \sa data()
*/
void QDomProcessingInstruction::setData( const QString& d )
{
    if ( !impl )
	return;
    impl->setNodeValue( d );
}

/*!
  Returns TRUE.
*/
bool QDomProcessingInstruction::isProcessingInstruction() const
{
    return TRUE;
}

#undef IMPL

/*==============================================================*/
/*                 Document                                     */
/*==============================================================*/

/**************************************************************
 *
 * QDOM_DocumentPrivate
 *
 **************************************************************/

class QDOM_DocumentPrivate : public QDOM_NodePrivate
{
public:
    QDOM_DocumentPrivate();
    QDOM_DocumentPrivate( const QString& name );
    QDOM_DocumentPrivate( QDOM_DocumentPrivate* n, bool deep );
    ~QDOM_DocumentPrivate();

    bool setContent( QXmlInputSource& source );

    // Attributes
    QDOM_DocumentTypePrivate* doctype() { return type; };
    QDOM_ImplementationPrivate* implementation() { return impl; };
    QDOM_ElementPrivate* documentElement();

    // Factories
    QDOM_ElementPrivate*               createElement( const QString& tagName );
    QDOM_DocumentFragmentPrivate*      createDocumentFragment();
    QDOM_TextPrivate*                  createTextNode( const QString& data );
    QDOM_CommentPrivate*               createComment( const QString& data );
    QDOM_CDATASectionPrivate*          createCDATASection( const QString& data );
    QDOM_ProcessingInstructionPrivate* createProcessingInstruction( const QString& target, const QString& data );
    QDOM_AttrPrivate*                  createAttribute( const QString& name );
    QDOM_EntityReferencePrivate*       createEntityReference( const QString& name );
    QDOM_NodeListPrivate*              elementsByTagName( const QString& tagname );

    // Overloaded from QDOM_NodePrivate
    virtual QDOM_NodePrivate* cloneNode( bool deep = TRUE );
    virtual bool isDocument() { return TRUE; }
    virtual void clear();
    virtual void save( QTextStream&, int ) const;

    // Variables
    QDOM_ImplementationPrivate* impl;
    QDOM_DocumentTypePrivate* type;

    static QString* docName;
};

QString* QDOM_DocumentPrivate::docName = 0;

QDOM_DocumentPrivate::QDOM_DocumentPrivate()
    : QDOM_NodePrivate( 0 )
{
    impl = new QDOM_ImplementationPrivate();
    type = new QDOM_DocumentTypePrivate( this, this );

    if ( !docName )
	docName = new QString( "#document" );
    name = *docName;
}

QDOM_DocumentPrivate::QDOM_DocumentPrivate( const QString& aname )
    : QDOM_NodePrivate( 0 )
{
    impl = new QDOM_ImplementationPrivate();
    type = new QDOM_DocumentTypePrivate( this, this );
    type->name = aname;

    if ( !docName )
	docName = new QString( "#document" );
    QDOM_DocumentPrivate::name = *docName;
}

QDOM_DocumentPrivate::QDOM_DocumentPrivate( QDOM_DocumentPrivate* n, bool deep )
    : QDOM_NodePrivate( n, deep )
{
    impl = n->impl->clone();
    // Reference count is down to 0, so we set it to 1 here.
    impl->ref();
    type = (QDOM_DocumentTypePrivate*)n->type->cloneNode();
    type->setParent( this );
    // Reference count is down to 0, so we set it to 1 here.
    type->ref();
}

QDOM_DocumentPrivate::~QDOM_DocumentPrivate()
{
    // qDebug("~Document %x", this);
    if ( impl->deref() ) delete impl;
    if ( type->deref() ) delete type;
}

void QDOM_DocumentPrivate::clear()
{
    if ( impl->deref() ) delete impl;
    if ( type->deref() ) delete type;
    impl = 0;
    type = 0;
    QDOM_NodePrivate::clear();
}

bool QDOM_DocumentPrivate::setContent( QXmlInputSource& source )
{
    clear();
    impl = new QDOM_ImplementationPrivate;
    type = new QDOM_DocumentTypePrivate( this, this );

    QXmlSimpleReader reader;
    QDomHandler hnd( this );
    reader.setContentHandler( &hnd );
    reader.setErrorHandler( &hnd );
    reader.setLexicalHandler( &hnd );
    reader.setDeclHandler( &hnd );
    reader.setDTDHandler( &hnd );
#if defined(Q_BROKEN_ALPHA) // #### very ugly hack, ws should really be able to get rid of that
    reader.setFeature( "http://xml.org/sax/features/namespaces", TRUE );
#else
    reader.setFeature( "http://xml.org/sax/features/namespaces", FALSE );
#endif
    reader.setFeature( "http://xml.org/sax/features/namespace-prefixes", TRUE );
    reader.setFeature( "http://trolltech.com/xml/features/report-whitespace-only-CharData", FALSE );

    if ( !reader.parse( source ) ) {
	qWarning("Parsing error");
	return FALSE;
    }

    return TRUE;
}

QDOM_NodePrivate* QDOM_DocumentPrivate::cloneNode( bool deep)
{
    QDOM_NodePrivate* p = new QDOM_DocumentPrivate( this, deep );
    // We are not interested in this node
    p->deref();
    return p;
}

QDOM_ElementPrivate* QDOM_DocumentPrivate::documentElement()
{
    QDOM_NodePrivate* p = first;
    while ( p && !p->isElement() )
	p = p->next;

    return (QDOM_ElementPrivate*)p;
}

QDOM_ElementPrivate* QDOM_DocumentPrivate::createElement( const QString& tagName )
{
    QDOM_ElementPrivate* e = new QDOM_ElementPrivate( this, this, tagName );
    e->deref();
    return e;
}

QDOM_DocumentFragmentPrivate* QDOM_DocumentPrivate::createDocumentFragment()
{
    QDOM_DocumentFragmentPrivate* f = new QDOM_DocumentFragmentPrivate( this, this );
    f->deref();
    return f;
}

QDOM_TextPrivate* QDOM_DocumentPrivate::createTextNode( const QString& data )
{
    QDOM_TextPrivate* t = new QDOM_TextPrivate( this, this, data );
    t->deref();
    return t;
}

QDOM_CommentPrivate* QDOM_DocumentPrivate::createComment( const QString& data )
{
    QDOM_CommentPrivate* c = new QDOM_CommentPrivate( this, this, data );
    c->deref();
    return c;
}

QDOM_CDATASectionPrivate* QDOM_DocumentPrivate::createCDATASection( const QString& data )
{
    QDOM_CDATASectionPrivate* c = new QDOM_CDATASectionPrivate( this, this, data );
    c->deref();
    return c;
}

QDOM_ProcessingInstructionPrivate* QDOM_DocumentPrivate::createProcessingInstruction( const QString& target, const QString& data )
{
    QDOM_ProcessingInstructionPrivate* p = new QDOM_ProcessingInstructionPrivate( this, this, target, data );
    p->deref();
    return p;
}

QDOM_AttrPrivate* QDOM_DocumentPrivate::createAttribute( const QString& aname )
{
    QDOM_AttrPrivate* a = new QDOM_AttrPrivate( this, this, aname );
    a->deref();
    return a;
}

QDOM_EntityReferencePrivate* QDOM_DocumentPrivate::createEntityReference( const QString& aname )
{
    QDOM_EntityReferencePrivate* e = new QDOM_EntityReferencePrivate( this, this, aname );
    e->deref();
    return e;
}

void QDOM_DocumentPrivate::save( QTextStream& s, int ) const
{
    bool doc = FALSE;

    QDOM_NodePrivate* n = first;
    while ( n ) {
	if ( !doc && !n->isProcessingInstruction() ) {
	    type->save( s, 0 );
	    doc = TRUE;
	}
	n->save( s, 0 );
	n = n->next;
    }
}

/**************************************************************
 *
 * QDomDocument
 *
 **************************************************************/

#define IMPL ((QDOM_DocumentPrivate*)impl)

/*!
  \class QDomDocument qdom.h
  \brief The QDomDocument class is the representation of an XML document.

  \module XML

  The QDomDocument class represents the entire XML document. Conceptually, it
  is the root of the document tree, and provides the primary access to the
  document's data.

  Since elements, text nodes, comments, processing instructions, etc. cannot
  exist outside the context of a document, the document class also contains the
  factory functions needed to create these objects. The node objects created
  have an  ownerDocument() function which associates them with the document
  within whose context they were created.

  The parsed XML is represented internally by a tree of objects that can be
  accessed using the various QDom classes. All QDom classes do only reference
  objects in the internal tree. The internal objects in the DOM tree will get
  deleted, once the last QDom object referencing them and the QDomDocument are
  deleted.

  Creation of elements, text nodes, etc. is done via the various factory
  functions provided in this class. Using the default constructors of the QDom
  classes will only result in empty objects, that can not be manipulated or
  inserted into the Document.

  The QDom classes are typically used as follows:
  \code
  QDomDocument doc( "mydocument" );
  QFile f( "mydocument.xml" );
  if ( !f.open( IO_ReadOnly ) )
      return;
  if ( !doc.setContent( &f ) ) {
      f.close();
      return;
  }
  f.close();

  // print out the element names of all elements that are a direct child
  // of the outermost element.
  QDomElement docElem = doc.documentElement();

  QDomNode n = docElem.firstChild();
  while( !n.isNull() ) {
      QDomElement e = n.toElement(); // try to convert the node to an element.
      if( !e.isNull() ) { // the node was really an element.
          cout << e.tagName() << endl;
      }
      n = n.nextSibling();
  }

  // lets append a new element to the end of the document
  QDomElement elem = doc.createElement( "img" );
  elem.setAttribute( "src", "myimage.png" );
  docElem.appendChild( elem );
  \endcode

  Once \c doc and \c elem go out of scode, the whole internal tree representing
  the XML document will get deleted.

  For further information about the Document Objct Model see
  <a href="http://www.w3.org/TR/REC-DOM-Level-1/">http://www.w3.org/TR/REC-DOM-Level-1/</a>.
*/


/*!
  Constructs an empty document.
*/
QDomDocument::QDomDocument()
{
}

/*!
  Creates a document with the name \a name.
*/
QDomDocument::QDomDocument( const QString& name )
{
    // We take over ownership
    impl = new QDOM_DocumentPrivate( name );
}

/*!
  Copy constructor.

  The data of the copy is shared: modifying one will also change the other. If
  you want to make a real copy, use cloneNode() instead.
*/
QDomDocument::QDomDocument( const QDomDocument& x )
    : QDomNode( x )
{
}

/*!
  \internal
*/
QDomDocument::QDomDocument( QDOM_DocumentPrivate* x )
    : QDomNode( x )
{
}

/*!
  Assignment operator.

  The data of the copy is shared: modifying one will also change the other. If
  you want to make a real copy, use cloneNode() instead.
*/
QDomDocument& QDomDocument::operator= ( const QDomDocument& x )
{
    return (QDomDocument&) QDomNode::operator=( x );
}

/*!
  Destructor.
*/
QDomDocument::~QDomDocument()
{
}

/*!
  This function parses the string \a text and sets it as the content of the
  document.
*/
bool QDomDocument::setContent( const QString& text )
{
    if ( !impl )
	impl = new QDOM_DocumentPrivate;
    QXmlInputSource source;
    source.setData( text );
    return IMPL->setContent( source );
}

/*!
  \overload
*/
bool QDomDocument::setContent( const QByteArray& buffer )
{
    if ( !impl )
	impl = new QDOM_DocumentPrivate;
    QTextStream ts( buffer, IO_ReadOnly );
    QXmlInputSource source( ts );
    return IMPL->setContent( source );
}

/*!
  \overload
*/
bool QDomDocument::setContent( const QCString& buffer )
{
    return setContent( QString::fromUtf8( buffer, buffer.length() ) );
}

/*!
  \overload
*/
bool QDomDocument::setContent( QIODevice* dev )
{
    if ( !impl )
	impl = new QDOM_DocumentPrivate;
    QTextStream ts( dev );
    QXmlInputSource source( ts );
    return IMPL->setContent( source );
}

/*!
  Converts the parsed document back to its textual representation.
*/
QString QDomDocument::toString() const
{
    QString str;
    QTextStream s( str, IO_WriteOnly );
    s << *this;

    return str;
}

/*!
  \fn QCString QDomDocument::toCString() const

  Converts the parsed document back to its textual representation.
*/


/*!
  Returns the document type of this document.
*/
QDomDocumentType QDomDocument::doctype() const
{
    if ( !impl )
	return QDomDocumentType();
    return QDomDocumentType( IMPL->doctype() );
}

/*!
  Returns a QDomImplementation object.
*/
QDomImplementation QDomDocument::implementation() const
{
    if ( !impl )
	return QDomImplementation();
    return QDomImplementation( IMPL->implementation() );
}

/*!
  Returns the root element of the document.
*/
QDomElement QDomDocument::documentElement() const
{
    if ( !impl )
	return QDomElement();
    return QDomElement( IMPL->documentElement() );
}

/*!
  Creates a new element with the name \a tagName that can be inserted into the
  DOM tree.
*/
QDomElement QDomDocument::createElement( const QString& tagName )
{
    if ( !impl )
	return QDomElement();
    return QDomElement( IMPL->createElement( tagName ) );
}

/*!
  Creates a new document fragment, that can be used to hold parts
  of the document, when doing complex manipulations of the document tree.
*/
QDomDocumentFragment QDomDocument::createDocumentFragment()
{
    if ( !impl )
	return QDomDocumentFragment();
    return QDomDocumentFragment( IMPL->createDocumentFragment() );
}

/*!
  Creates a text node that can be inserted into the document tree.
*/
QDomText QDomDocument::createTextNode( const QString& value )
{
    if ( !impl )
	return QDomText();
    return QDomText( IMPL->createTextNode( value ) );
}

/*!
  Creates a new comment that can be inserted into the Document.
*/
QDomComment QDomDocument::createComment( const QString& value )
{
    if ( !impl )
	return QDomComment();
    return QDomComment( IMPL->createComment( value ) );
}

/*!
  Creates a new CDATA section that can be inserted into the document.
*/
QDomCDATASection QDomDocument::createCDATASection( const QString& value )
{
    if ( !impl )
	return QDomCDATASection();
    return QDomCDATASection( IMPL->createCDATASection( value ) );
}

/*!
  Creates a new processing instruction that can be inserted into the document.
*/
QDomProcessingInstruction QDomDocument::createProcessingInstruction( const QString& target,
								     const QString& data )
{
    if ( !impl )
	return QDomProcessingInstruction();
    return QDomProcessingInstruction( IMPL->createProcessingInstruction( target, data ) );
}


/*!
  Creates a new attribute that can be inserted into an element.
*/
QDomAttr QDomDocument::createAttribute( const QString& name )
{
    if ( !impl )
	return QDomAttr();
    return QDomAttr( IMPL->createAttribute( name ) );
}

/*!
  Creates a new entity reference.
*/
QDomEntityReference QDomDocument::createEntityReference( const QString& name )
{
    if ( !impl )
	return QDomEntityReference();
    return QDomEntityReference( IMPL->createEntityReference( name ) );
}

/*!
  Returns a QDomNodeList, that contains all elements in the document
  with the tag name \a tagname. The order of the node list, is the
  order they are encountered in a preorder traversal of the element tree.
*/
QDomNodeList QDomDocument::elementsByTagName( const QString& tagname ) const
{
    return QDomNodeList( new QDOM_NodeListPrivate( impl, tagname ) );
}

/*!
  Returns \c DocumentNode.
*/
QDomNode::NodeType QDomDocument::nodeType() const
{
    return DocumentNode;
}

/*!
  Returns TRUE
*/
bool QDomDocument::isDocument() const
{
    return TRUE;
}


#undef IMPL

/*==============================================================*/
/*               Node casting functions                         */
/*==============================================================*/

/*!
  Converts a QDomNode into a QDomAttr. If the node is not an attribute,
  the returned object will be null.

  \sa isAttr()
*/
QDomAttr QDomNode::toAttr()
{
    if ( impl && impl->isAttr() )
	return QDomAttr( ((QDOM_AttrPrivate*)impl) );
    return QDomAttr();
}

/*!
  Converts a QDomNode into a QDomCDATASection. If the node is not a CDATA
  section, the returned object will be null.

  \sa isCDATASection()
*/
QDomCDATASection QDomNode::toCDATASection()
{
    if ( impl && impl->isCDATASection() )
	return QDomCDATASection( ((QDOM_CDATASectionPrivate*)impl) );
    return QDomCDATASection();
}

/*!
  Converts a QDomNode into a QDomDocumentFragment. If the node is not a
  document fragment the returned object will be null.

  \sa isDocumentFragment()
*/
QDomDocumentFragment QDomNode::toDocumentFragment()
{
    if ( impl && impl->isDocumentFragment() )
	return QDomDocumentFragment( ((QDOM_DocumentFragmentPrivate*)impl) );
    return QDomDocumentFragment();
}

/*!
  Converts a QDomNode into a QDomDocument. If the node is not a document
  the returned object will be null.

  \sa isDocument()
*/
QDomDocument QDomNode::toDocument()
{
    if ( impl && impl->isDocument() )
	return QDomDocument( ((QDOM_DocumentPrivate*)impl) );
    return QDomDocument();
}

/*!
  Converts a QDomNode into a QDomDocumentType. If the node is not a document
  type the returned object will be null.

  \sa isDocumentType()
*/
QDomDocumentType QDomNode::toDocumentType()
{
    if ( impl && impl->isDocumentType() )
	return QDomDocumentType( ((QDOM_DocumentTypePrivate*)impl) );
    return QDomDocumentType();
}

/*!
  Converts a QDomNode into a QDomElement. If the node is not an element
  the returned object will be null.

  \sa isElement()
*/
QDomElement QDomNode::toElement()
{
    if ( impl && impl->isElement() )
	return QDomElement( ((QDOM_ElementPrivate*)impl) );
    return QDomElement();
}

/*!
  Converts a QDomNode into a QDomEntityReference. If the node is not an entity
  reference, the returned object will be null.

  \sa isEntityReference()
*/
QDomEntityReference QDomNode::toEntityReference()
{
    if ( impl && impl->isEntityReference() )
	return QDomEntityReference( ((QDOM_EntityReferencePrivate*)impl) );
    return QDomEntityReference();
}

/*!
  Converts a QDomNode into a QDomText. If the node is not a text, the returned
  object will be null.

  \sa isText()
*/
QDomText QDomNode::toText()
{
    if ( impl && impl->isText() )
	return QDomText( ((QDOM_TextPrivate*)impl) );
    return QDomText();
}

/*!
  Converts a QDomNode into a QDomEntity. If the node is not an entity the
  returned object will be null.

  \sa isEntity()
*/
QDomEntity QDomNode::toEntity()
{
    if ( impl && impl->isEntity() )
	return QDomEntity( ((QDOM_EntityPrivate*)impl) );
    return QDomEntity();
}

/*!
  Converts a QDomNode into a QDomNotation. If the node is not a notation
  the returned object will be null.

  \sa isNotation()
*/
QDomNotation QDomNode::toNotation()
{
    if ( impl && impl->isNotation() )
	return QDomNotation( ((QDOM_NotationPrivate*)impl) );
    return QDomNotation();
}

/*!
  Converts a QDomNode into a QDomProcessingInstruction. If the node is not a
  processing instruction the returned object will be null.

  \sa isProcessingInstruction()
*/
QDomProcessingInstruction QDomNode::toProcessingInstruction()
{
    if ( impl && impl->isProcessingInstruction() )
	return QDomProcessingInstruction( ((QDOM_ProcessingInstructionPrivate*)impl) );
    return QDomProcessingInstruction();
}

/*!
  Converts a QDomNode into a QDomCharacterData. If the node is not a character
  data node the returned object will be null.

  \sa isCharacterData()
*/
QDomCharacterData QDomNode::toCharacterData()
{
    if ( impl && impl->isCharacterData() )
	return QDomCharacterData( ((QDOM_CharacterDataPrivate*)impl) );
    return QDomCharacterData();
}

/*!
  Converts a QDomNode into a QDomComment. If the node is not a comment the
  returned object will be null.

  \sa isComment()
*/
QDomComment QDomNode::toComment()
{
    if ( impl && impl->isComment() )
	return QDomComment( ((QDOM_CommentPrivate*)impl) );
    return QDomComment();
}

/*==============================================================*/
/*                      QDomHandler                             */
/*==============================================================*/

QDomHandler::QDomHandler( QDOM_DocumentPrivate* adoc )
{
    doc = adoc;
    node = doc;
    cdata = FALSE;
}

QDomHandler::~QDomHandler()
{
}

void QDomHandler::setDocumentLocator( QXmlLocator* locator )
{
    loc = locator;
}

bool QDomHandler::endDocument()
{
    // ### is this really necessary? (rms)
    if ( node != doc )
	return FALSE;
    return TRUE;
}

bool QDomHandler::startDTD( const QString& name, const QString&, const QString&)
{
    doc->doctype()->name = name;
    return TRUE;
}

bool QDomHandler::startElement( const QString&, const QString&, const QString& qName, const QXmlAttributes& atts )
{
    // tag name
#if 0
    // ### do we really need this?
    if ( node == doc ) {
	// Has to be a special tag
	if ( qName != doc->doctype()->nodeName() ) {
	    // TODO: Exception
	    return FALSE;
	}
    }
#endif
    QDOM_NodePrivate* n = doc->createElement( qName );
    node->appendChild( n );
    node = n;

    // attributes
    for ( int i=0; i<atts.length(); i++ )
    {
	if ( !node->isElement() ) {
	    // TODO: Exception
	    return FALSE;
	}
	((QDOM_ElementPrivate*)node)->setAttribute( atts.qName(i), atts.value(i) );
    }

    return TRUE;
}

bool QDomHandler::endElement( const QString&, const QString&, const QString& )
{
    if ( node == doc )
	return FALSE;
    node = node->parent;

    return TRUE;
}

bool QDomHandler::characters( const QString&  ch )
{
    // No text as child of some document
    if ( node == doc )
	return FALSE;

    if ( cdata ) {
	node->appendChild( doc->createCDATASection( ch ) );
    } else {
	node->appendChild( doc->createTextNode( ch ) );
    }

    return TRUE;
}

bool QDomHandler::processingInstruction( const QString& target, const QString& data )
{
    node->appendChild( doc->createProcessingInstruction( target, data ) );
    return TRUE;
}

bool QDomHandler::fatalError( const QXmlParseException& exception )
{
    qDebug( "fatal parsing error: " +  exception.message() + " in line %d",
	    exception.lineNumber() );
    return QXmlDefaultHandler::fatalError( exception );
}

bool QDomHandler::startCDATA()
{
    cdata = TRUE;
    return TRUE;
}

bool QDomHandler::endCDATA()
{
    cdata = FALSE;
    return TRUE;
}

bool QDomHandler::comment( const QString& ch )
{
    node->appendChild( doc->createComment( ch ) );
    return TRUE;
}

bool QDomHandler::unparsedEntityDecl( const QString &name, const QString &publicId, const QString &systemId, const QString &notationName )
{
    QDOM_EntityPrivate* e = new QDOM_EntityPrivate( doc, 0, name,
	    publicId, systemId, notationName );
    doc->doctype()->appendChild( e );
    return TRUE;
}

bool QDomHandler::externalEntityDecl( const QString &name, const QString &publicId, const QString &systemId )
{
    return unparsedEntityDecl( name, publicId, systemId, QString::null );
}

bool QDomHandler::notationDecl( const QString & name, const QString & publicId, const QString & systemId )
{
    QDOM_NotationPrivate* n = new QDOM_NotationPrivate( doc, 0, name, publicId, systemId );
    doc->doctype()->appendChild( n );
    return TRUE;
}

#if 0
bool QDomConsumer::entity( const QString& name, const QString& value )
{
    QDOM_EntityPrivate* e = new QDOM_EntityPrivate( doc, 0, name, QString::null, QString::null, QString::null );
    e->value = value;
    doc->doctype()->appendChild( e );

    return TRUE;
}

bool QDomConsumer::entityRef( const QString& name )
{
    if ( node == doc )
	return FALSE;

    // TODO: Find corresponding entity
    QDOM_NamedNodeMapPrivate* m = doc->doctype()->entities;
    if ( !m )
	return FALSE;
    QDOM_NodePrivate* n = m->namedItem( name );
    if ( !n || !n->isEntity() ) {
	qWarning( "Entity of name %s unsupported", name.latin1() );
	return FALSE;
    }

    node->appendChild( doc->createEntityReference( name ) );

    return TRUE;
}
#endif

#endif //QT_NO_DOM
