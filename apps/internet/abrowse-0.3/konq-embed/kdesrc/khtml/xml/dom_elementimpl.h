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
#ifndef _DOM_ELEMENTImpl_h_
#define _DOM_ELEMENTImpl_h_

#include "dom_nodeimpl.h"

#include "htmltoken.h"
#include "dtd.h"

namespace DOM {

class ElementImpl;
class DocumentImpl;
class NamedAttrMapImpl;

class AttrImpl : public NodeImpl
{
    friend class ElementImpl;
    friend class NamedAttrMapImpl;

public:
    AttrImpl();
    AttrImpl(DocumentImpl *doc, const DOMString &name);
    AttrImpl(DocumentImpl *doc, int id);
    AttrImpl(const AttrImpl &other);

    AttrImpl &operator = (const AttrImpl &other);
    ~AttrImpl();

    virtual const DOMString nodeName() const;
    virtual unsigned short nodeType() const;
    virtual bool isAttributeNode() const { return true; }

    DOMString name() const;
    bool specified() const { return m_specified; }

    virtual DOMString value() const;
    virtual void setValue( const DOMString &v );

    virtual DOMString nodeValue() const { return value(); }

    virtual void setNodeValue( const DOMString &, int &exceptioncode );

    virtual NodeImpl *parentNode() const;
    virtual NodeImpl *previousSibling() const;
    virtual NodeImpl *nextSibling() const;
    virtual NodeImpl *cloneNode ( bool deep, int &exceptioncode );

    virtual bool deleteMe();
    DOMStringImpl *val() { return _value; }
    virtual bool childAllowed( NodeImpl *newChild );

protected:
//    AttrImpl(const DOMString &name, const DOMString &value, DocumentImpl *doc, bool specified);
    AttrImpl(const khtml::Attribute *attr, DocumentImpl *doc, ElementImpl *element);
    AttrImpl(const DOMString &name, const DOMString &value, DocumentImpl *doc);
    AttrImpl(int _id, const DOMString &value, DocumentImpl *doc);

    void setName(const DOMString &n);

    DOMStringImpl *_name;
    DOMStringImpl *_value;

    ElementImpl *_element;
public:
    unsigned char attrId;

};


class ElementImpl : public NodeBaseImpl
{
    friend class DocumentImpl;
    friend class NamedAttrMapImpl;
    friend class AttrImpl;

public:
    ElementImpl(DocumentImpl *doc);
    ~ElementImpl();

    virtual bool isInline() const;

    virtual unsigned short nodeType() const;
    virtual bool isElementNode() const { return true; }

    virtual bool isHTMLElement() const { return false; }

    DOMString tagName() const;

    DOMString getAttribute ( const DOMString &name ) const;

    void setAttribute ( const DOMString &name, const DOMString &value);
    void removeAttribute ( const DOMString &name );

    AttrImpl *getAttributeNode ( const DOMString &name );
    AttrImpl *setAttributeNode ( AttrImpl *newAttr, int &exceptioncode );
    AttrImpl *removeAttributeNode ( AttrImpl *oldAttr, int &exceptioncode );

    NodeListImpl *getElementsByTagName ( const DOMString &name );
    NodeListImpl *getElementsByNameAttr ( const DOMString &name );

    void normalize ( int &exceptioncode );

    virtual void applyChanges(bool = true, bool = true);

    virtual NodeImpl *cloneNode ( bool deep, int &exceptioncode );
    virtual NamedNodeMapImpl *attributes() const;

    /**
     * override this in subclasses if you need to parse
     * attributes. This is always called, whenever an attribute changed
      */
    virtual void parseAttribute(AttrImpl *) {}

    virtual tagStatus startTag() const { return DOM::REQUIRED; }
    virtual tagStatus endTag() const { return DOM::REQUIRED; }

    // not part of the DOM

    DOMString getAttribute ( int id );
    AttrImpl *getAttributeNode ( int index ) const;
    int getAttributeCount() const;
    void setAttribute ( int id, const DOMString &value );
    void setAttribute ( const khtml::AttributeList &list );

    // State of the element.
    virtual QString state() { return QString::null; }

    virtual void attach(KHTMLView *w);
    virtual void detach();
    virtual void recalcStyle();
    virtual void setOwnerDocument(DocumentImpl *_document);

    virtual bool mouseEvent( int x, int y,
			     int _tx, int _ty,
			     MouseEvent *ev);
    virtual void setFocus(bool = true);
    virtual void setActive(bool = true);
    virtual void mouseEventHandler( MouseEvent */*ev*/, bool /*inside*/ ) {};
    virtual khtml::FindSelectionResult findSelectionNode( int _x, int _y, int _tx, int _ty,
                                                   DOM::Node & node, int & offset );
    virtual bool isSelectable() const;
    virtual bool childAllowed( NodeImpl *newChild );

    virtual short tabIndex() const;
    virtual void setTabIndex( short );

    virtual DOM::CSSStyleDeclarationImpl *styleRules() const { return m_styleDecls; }

protected: // member variables

    friend class NodeImpl;
    NamedAttrMapImpl *namedAttrMap;

    // map of default attributes. derived element classes are responsible
    // for setting this according to the corresponding element description
    // in the DTD
    virtual khtml::AttributeList *defaultMap() const;
    DOM::CSSStyleDeclarationImpl *m_styleDecls;
};


class XMLElementImpl : public ElementImpl
{

public:
    XMLElementImpl(DocumentImpl *doc, DOMStringImpl *_name);
    XMLElementImpl(DocumentImpl *doc, DOMStringImpl *_name, DOMStringImpl *_namespaceURI);
    ~XMLElementImpl();

    virtual const DOMString nodeName() const;
    virtual DOMString namespaceURI() const;
    virtual unsigned short id() const { return m_id; };

protected:
    DOMStringImpl *m_name;
    DOMStringImpl *m_namespaceURI;
    unsigned short m_id;
};



class NamedAttrMapImpl : public NamedNodeMapImpl
{
public:
    NamedAttrMapImpl(ElementImpl *e);
    virtual ~NamedAttrMapImpl();
    NamedAttrMapImpl &operator =(const khtml::AttributeList &list);
    NamedAttrMapImpl &operator =(const NamedAttrMapImpl &other);

    unsigned long length(int &exceptioncode) const;
    unsigned long length() const; // ### remove?

    NodeImpl *getNamedItem ( const DOMString &name, int &exceptioncode ) const;
    NodeImpl *getNamedItem ( const DOMString &name ) const; // ### remove?
    AttrImpl *getIdItem ( int id ) const;

    NodeImpl *setNamedItem ( const Node &arg, int &exceptioncode );
    AttrImpl *setIdItem ( AttrImpl *attr, int& exceptioncode );

    NodeImpl *removeNamedItem ( const DOMString &name, int &exceptioncode );
    AttrImpl *removeIdItem ( int id );

    NodeImpl *item ( unsigned long index, int &exceptioncode ) const;
    NodeImpl *item ( unsigned long index ) const; // ### remove?
    AttrImpl *removeAttr( AttrImpl *oldAttr, int &exceptioncode );
    void detachFromElement();

protected:
    ElementImpl *element;
    AttrImpl **attrs;
    uint len;
    void clearAttrs();

};

}; //namespace

#endif
