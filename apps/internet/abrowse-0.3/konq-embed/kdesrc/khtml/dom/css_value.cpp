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
#include "css_rule.h"
#include "css_value.h"
#include "css_valueimpl.h"
#include "dom_exception.h"
#include "dom_string.h"
using namespace DOM;

CSSStyleDeclaration::CSSStyleDeclaration()
{
    impl = 0;
}

CSSStyleDeclaration::CSSStyleDeclaration(const CSSStyleDeclaration &other)
{
    impl = other.impl;
    if(impl) impl->ref();
}

CSSStyleDeclaration::CSSStyleDeclaration(CSSStyleDeclarationImpl *i)
{
    impl = i;
    if(impl) impl->ref();
}

CSSStyleDeclaration &CSSStyleDeclaration::operator = (const CSSStyleDeclaration &other)
{
    if(impl) impl->deref();
    impl = other.impl;
    if(impl) impl->ref();
    return *this;
}

CSSStyleDeclaration::~CSSStyleDeclaration()
{
    if(impl) impl->deref();
}

DOMString CSSStyleDeclaration::cssText() const
{
    if(!impl) return 0;
    return static_cast<CSSStyleDeclarationImpl *>(impl)->cssText();
}

void CSSStyleDeclaration::setCssText( const DOMString &value )
{
    // ###
    if(!impl) return;
    impl->setCssText(value);
}

DOMString CSSStyleDeclaration::getPropertyValue( const DOMString &propertyName )
{
    if(!impl) return 0;
    return static_cast<CSSStyleDeclarationImpl *>(impl)->getPropertyValue( propertyName );
}

CSSValue CSSStyleDeclaration::getPropertyCSSValue( const DOMString &propertyName )
{
    if(!impl) return 0;
    return static_cast<CSSStyleDeclarationImpl *>(impl)->getPropertyCSSValue( propertyName );
}

DOMString CSSStyleDeclaration::removeProperty( const DOMString &propertyName )
{
    if(!impl) return 0;
    return static_cast<CSSStyleDeclarationImpl *>(impl)->removeProperty( propertyName );
    return 0;
}

DOMString CSSStyleDeclaration::getPropertyPriority( const DOMString &propertyName )
{
    if(!impl) return 0;
    return impl->getPropertyPriority(propertyName);
}

void CSSStyleDeclaration::setProperty( const DOMString &propertyName, const DOMString &value, const DOMString &priority )
{
    // ###
    if(!impl) return;
    static_cast<CSSStyleDeclarationImpl *>(impl)->setProperty( propertyName, value, priority );
}

unsigned long CSSStyleDeclaration::length() const
{
    if(!impl) return 0;
    return static_cast<CSSStyleDeclarationImpl *>(impl)->length();
}

DOMString CSSStyleDeclaration::item( unsigned long index )
{
    if(!impl) return 0;
    return static_cast<CSSStyleDeclarationImpl *>(impl)->item( index );
}

CSSRule CSSStyleDeclaration::parentRule() const
{
    if(!impl) return 0;
    return static_cast<CSSStyleDeclarationImpl *>(impl)->parentRule();
}

CSSStyleDeclarationImpl *CSSStyleDeclaration::handle() const
{
    return impl;
}

bool CSSStyleDeclaration::isNull() const
{
    return (impl == 0);
}

// ----------------------------------------------------------

CSSValue::CSSValue()
{
    impl = 0;
}

CSSValue::CSSValue(const CSSValue &other)
{
    impl = other.impl;
    if(impl) impl->ref();
}

CSSValue::CSSValue(CSSValueImpl *i)
{
    impl = i;
    if(impl) impl->ref();
}

CSSValue &CSSValue::operator = (const CSSValue &other)
{
    if(impl) impl->deref();
    impl = other.impl;
    if(impl) impl->ref();
    return *this;
}

CSSValue::~CSSValue()
{
    if(impl) impl->deref();
}

DOMString CSSValue::cssText() const
{
    return 0;
    //if(!impl) return 0;
    //return ((ElementImpl *)impl)->getAttribute("cssText");
}

void CSSValue::setCssText( const DOMString &/*value*/ )
{
    //if(impl) ((ElementImpl *)impl)->setAttribute("cssText", value);
}

unsigned short CSSValue::valueType() const
{
    if(!impl) return 0;
    return ((CSSValueImpl *)impl)->valueType();
}

// ----------------------------------------------------------

// ### perhaps just use impl instead of vimpl (not sure why it
// is here)

CSSValueList::CSSValueList() : CSSValue()
{
}

CSSValueList::CSSValueList(const CSSValueList &other) : CSSValue(other)
{
}

CSSValueList::CSSValueList(CSSValueListImpl *impl) : CSSValue(impl)
{
}

CSSValueList &CSSValueList::operator = (const CSSValueList &/*other*/)
{
    // TODO
    //CSSValue::operator = (other);
    return *this;
}

CSSValueList::~CSSValueList()
{
}

unsigned long CSSValueList::length() const
{
    if(!vimpl) return 0;
    return ((CSSValueListImpl *)vimpl)->length();
}

CSSValue CSSValueList::item( unsigned long /*index*/ )
{
    //if(!vimpl) return 0;
    //return ((CSSValueListImpl *)vimpl)->item( index );
    return 0;
}

// ----------------------------------------------------------

CSSPrimitiveValue::CSSPrimitiveValue() : CSSValue()
{
}

CSSPrimitiveValue::CSSPrimitiveValue(const CSSPrimitiveValue &other) : CSSValue(other)
{
}

CSSPrimitiveValue::CSSPrimitiveValue(CSSPrimitiveValueImpl *impl) : CSSValue(impl)
{
}

CSSPrimitiveValue &CSSPrimitiveValue::operator = (const CSSPrimitiveValue &/*other*/)
{
    //CSSValue::operator = (other);
    return *this;
}

CSSPrimitiveValue::~CSSPrimitiveValue()
{
}

unsigned short CSSPrimitiveValue::primitiveType() const
{
    if(!impl) return 0;
    return ((CSSPrimitiveValueImpl *)impl)->primitiveType();
}

void CSSPrimitiveValue::setFloatValue( unsigned short /*unitType*/, float /*floatValue*/ )
{
    //if(impl)
    //    ((CSSPrimitiveValueImpl *)impl)->setFloatValue( unitType, floatValue );
}

float CSSPrimitiveValue::getFloatValue( unsigned short unitType )
{
    if(!impl) return 0;
    // ### add unit conversion
    if(primitiveType() != unitType)
	throw CSSException(CSSException::SYNTAX_ERR);
    return ((CSSPrimitiveValueImpl *)impl)->getFloatValue( unitType );
}

void CSSPrimitiveValue::setStringValue( unsigned short stringType, const DOMString &stringValue )
{
    int exceptioncode = 0;
    if(impl)
        ((CSSPrimitiveValueImpl *)impl)->setStringValue( stringType, stringValue, exceptioncode );
    if ( exceptioncode >= CSSException::_EXCEPTION_OFFSET )
	throw CSSException( exceptioncode - CSSException::_EXCEPTION_OFFSET );
    if ( exceptioncode )
	throw DOMException( exceptioncode );

}

DOMString CSSPrimitiveValue::getStringValue(  )
{
    return 0;
    //if(!impl) return 0;

    //return ((CSSPrimitiveValueImpl *)impl)->getStringValue(  );
}

Counter CSSPrimitiveValue::getCounterValue(  )
{
    //if(!impl) return 0;
    //return ((CSSPrimitiveValueImpl *)impl)->getCounterValue(  );
  return Counter();
}

Rect CSSPrimitiveValue::getRectValue(  )
{
    //if(!impl) return 0;
    //return ((CSSPrimitiveValueImpl *)impl)->getRectValue(  );
  return Rect();
}

RGBColor CSSPrimitiveValue::getRGBColorValue(  )
{
    //if(!impl) return 0;
    //return ((CSSPrimitiveValueImpl *)impl)->getRGBColorValue(  );
  return RGBColor();
}

// -------------------------------------------------------------------

Counter::Counter()
{
}

Counter::Counter(const Counter &/*other*/)
{
    //###
}

Counter &Counter::operator = (const Counter &/*other*/)
{
    //::operator = (other);
    return *this;
}

Counter::~Counter()
{
}

DOMString Counter::identifier() const
{
  return DOMString();
}

DOMString Counter::listStyle() const
{
  return DOMString();
}

DOMString Counter::separator() const
{
  return DOMString();
}

// --------------------------------------------------------------------

RGBColor::RGBColor()
{
}

RGBColor::RGBColor(const RGBColor &other)
{
    m_color = other.m_color;
}

RGBColor::RGBColor(const QColor &color)
{
    m_color = color;
}

RGBColor &RGBColor::operator = (const RGBColor &other)
{
    m_color = other.m_color;
    return *this;
}

RGBColor::~RGBColor()
{
}

CSSPrimitiveValue RGBColor::red() const
{
    return new CSSPrimitiveValueImpl((float)m_color.red(), CSSPrimitiveValue::CSS_DIMENSION);
}

CSSPrimitiveValue RGBColor::green() const
{
    return new CSSPrimitiveValueImpl((float)m_color.green(), CSSPrimitiveValue::CSS_DIMENSION);
}

CSSPrimitiveValue RGBColor::blue() const
{
    return new CSSPrimitiveValueImpl((float)m_color.blue(), CSSPrimitiveValue::CSS_DIMENSION);
}


// ---------------------------------------------------------------------

Rect::Rect()
{
}

Rect::Rect(const Rect &/*other*/)
{
}

Rect &Rect::operator = (const Rect &/*other*/)
{
  // TODO
  return *this;
}

Rect::~Rect()
{
}

CSSPrimitiveValue Rect::top() const
{
  return CSSPrimitiveValue();
}

CSSPrimitiveValue Rect::right() const
{
  return CSSPrimitiveValue();
}

CSSPrimitiveValue Rect::bottom() const
{
  return CSSPrimitiveValue();
}

CSSPrimitiveValue Rect::left() const
{
  return CSSPrimitiveValue();
}









