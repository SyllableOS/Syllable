/**
 * This file is part of the DOM implementation for KDE.
 *
 * (C) 1999 Lars Knoll (knoll@kde.org)
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
#ifndef _CSS_css_valueimpl_h_
#define _CSS_css_valueimpl_h_

#include <css_value.h>
#include "dom_string.h"
#include "cssparser.h"
#include "misc/loader_client.h"

#include <qintdict.h>

namespace khtml {
    class CachedImage;
};

namespace DOM {

class CSSRuleImpl;
class CSSValueImpl;
class NodeImpl;


class CSSStyleDeclarationImpl : public StyleBaseImpl
{
public:
    CSSStyleDeclarationImpl(CSSRuleImpl *parentRule);
    CSSStyleDeclarationImpl(CSSRuleImpl *parentRule, QList<CSSProperty> *lstValues);

    virtual ~CSSStyleDeclarationImpl();

    unsigned long length() const;
    CSSRuleImpl *parentRule() const;
    DOM::DOMString getPropertyValue ( const DOM::DOMString &propertyName );
    CSSValueImpl *getPropertyCSSValue ( const DOM::DOMString &propertyName );
    DOM::DOMString removeProperty ( const DOM::DOMString &propertyName );
    DOM::DOMString removeProperty ( int propertyId );
    DOM::DOMString getPropertyPriority ( const DOM::DOMString &propertyName );
    void setProperty ( const DOM::DOMString &propertyName, const DOM::DOMString &value,
		       const DOM::DOMString &priority );
    void setProperty( const DOMString &propName, const DOMString &value, bool important, bool nonCSSHint);
    void setProperty ( int propertyId, const DOM::DOMString &value, bool important = false, bool nonCSSHint = false);
    // this treats integers as pixels!
    // needed for conversion of html attributes
    void setLengthProperty(int id, const DOM::DOMString &value, bool important, bool nonCSSHint = true);

    // add a whole, unparsed property
    void setProperty ( const DOMString &propertyString);
    DOM::DOMString item ( unsigned long index );

    DOM::DOMString cssText() const;
    void setCssText(DOM::DOMString str);

    virtual bool isStyleDeclaration() { return true; }

    virtual bool parseString( const DOMString &string, bool = false );

    CSSValueImpl *getPropertyCSSValue( int propertyID );
    bool getPropertyPriority( int propertyID );

    QList<CSSProperty> *values() { return m_lstValues; }
    void setNode(NodeImpl *_node) { m_node = _node; }

protected:
    QList<CSSProperty> *m_lstValues;
    NodeImpl *m_node;
};

class CSSValueImpl : public StyleBaseImpl
{
public:
    CSSValueImpl();

    virtual ~CSSValueImpl();

    virtual unsigned short valueType() const = 0;

    virtual DOM::DOMString cssText() const;
    void setCssText(DOM::DOMString str);

    virtual bool isValue() { return true; }
};

class CSSInheritedValueImpl : public CSSValueImpl
{
public:
    CSSInheritedValueImpl() : CSSValueImpl() {}
    virtual ~CSSInheritedValueImpl() {}

    virtual unsigned short valueType() const { return CSSValue::CSS_INHERIT; }
    virtual DOM::DOMString cssText() const;
};


class CSSValueListImpl : public CSSValueImpl
{
public:
    CSSValueListImpl();

    virtual ~CSSValueListImpl();

    unsigned long length() const { return m_values.count(); }
    CSSValueImpl *item ( unsigned long index ) { return m_values.at(index); }

    virtual bool isValueList() { return true; }

    virtual unsigned short valueType() const;

    void append(CSSValueImpl *val);
    virtual DOM::DOMString cssText() const;

protected:
    QList<CSSValueImpl> m_values;
};


class Counter;
class RGBColor;
class Rect;

class CSSPrimitiveValueImpl : public CSSValueImpl
{
public:
    CSSPrimitiveValueImpl();
    CSSPrimitiveValueImpl(int ident);
    CSSPrimitiveValueImpl(float num, CSSPrimitiveValue::UnitTypes type);
    CSSPrimitiveValueImpl(const DOMString &str, CSSPrimitiveValue::UnitTypes type);
    CSSPrimitiveValueImpl(const Counter &c);
    CSSPrimitiveValueImpl(const Rect &r);
    CSSPrimitiveValueImpl(const RGBColor &rgb);
    CSSPrimitiveValueImpl(const QColor &color);

    virtual ~CSSPrimitiveValueImpl();

    void cleanup();
    
    unsigned short primitiveType() const;
    // use with care!!!
    void setPrimitiveType(unsigned short type) { m_type = type; }
    void setFloatValue ( unsigned short unitType, float floatValue, int &exceptioncode );
    float getFloatValue ( unsigned short unitType);
    void setStringValue ( unsigned short stringType, const DOM::DOMString &stringValue, int &exceptioncode );
    DOM::DOMStringImpl *getStringValue (  );
    Counter *getCounterValue (  );
    Rect *getRectValue (  );
    RGBColor *getRGBColorValue (  );

    virtual bool isPrimitiveValue() { return true; }
    virtual unsigned short valueType() const;

    int getIdent();

    virtual bool parseString( const DOMString &string, bool = false);
    virtual DOM::DOMString cssText() const;

protected:
    int m_type;
    union {
	int ident;
	float num;
	DOM::DOMStringImpl *string;
	Counter *counter;
	Rect *rect;
	RGBColor *rgbcolor;
    } m_value;
};

class CSSImageValueImpl : public CSSPrimitiveValueImpl, public khtml::CachedObjectClient
{
public:
    CSSImageValueImpl(const DOMString &url, const DOMString &baseurl, StyleBaseImpl *style);
    CSSImageValueImpl();
    virtual ~CSSImageValueImpl();

    khtml::CachedImage *image() { return m_image; }
protected:
    khtml::CachedImage *m_image;
};
// ------------------------------------------------------------------------------

}; // namespace

#endif
