/*
 *  This file is part of the KDE libraries
 *  Copyright (C) 2000 Harri Porten (porten@kde.org)
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "kjs_css.h"

#include <qptrdict.h>
#include <qregexp.h>

#include <xml/dom_nodeimpl.h>
#include <dom/html_element.h>
#include <dom/dom_string.h>
#include <html/html_elementimpl.h>
#include <rendering/render_style.h>
#include <kjs/types.h>
#include <css/cssproperties.h>
#include "kjs_binding.h"

using namespace KJS;
#include <kdebug.h>

QPtrDict<DOMCSSStyleDeclaration> domCSSStyleDeclarations;

static QString jsNameToProp( const UString &p )
{
    QString prop = p.qstring();
    int i = prop.length();
    while( --i ) {
	char c = prop[i].latin1();
	if ( c < 'A' || c > 'Z' )
	    continue;
	prop.insert( i, '-' );
    }

    return prop.lower();
}

/*
KJSO Style::get(const UString &p) const
{
    return KJSO();
}

void Style::put(const UString &p, const KJSO& v)
{

}
*/



DOMCSSStyleDeclaration::~DOMCSSStyleDeclaration()
{
  domCSSStyleDeclarations.remove(styleDecl.handle());
}

const TypeInfo DOMCSSStyleDeclaration::info = { "CSSStyleDeclaration", HostType, 0, 0, 0 };


KJSO DOMCSSStyleDeclaration::tryGet(const UString &p) const
{
  if (p == "cssText" )
    return getString(styleDecl.cssText());
  else if (p == "getPropertyValue")
    return new DOMCSSStyleDeclarationFunc(styleDecl,DOMCSSStyleDeclarationFunc::GetPropertyValue);
  else if (p == "getPropertyCSSValue")
    return new DOMCSSStyleDeclarationFunc(styleDecl,DOMCSSStyleDeclarationFunc::GetPropertyCSSValue);
  else if (p == "removeProperty")
    return new DOMCSSStyleDeclarationFunc(styleDecl,DOMCSSStyleDeclarationFunc::RemoveProperty);
  else if (p == "getPropertyPriority")
    return new DOMCSSStyleDeclarationFunc(styleDecl,DOMCSSStyleDeclarationFunc::GetPropertyPriority);
  else if (p == "setProperty")
    return new DOMCSSStyleDeclarationFunc(styleDecl,DOMCSSStyleDeclarationFunc::SetProperty);
  else if (p == "length" )
    return Number(styleDecl.length());
  else if (p == "item")
    return new DOMCSSStyleDeclarationFunc(styleDecl,DOMCSSStyleDeclarationFunc::Item);
  else if (p == "parentRule" )
    return Undefined(); // ###
  else {
    DOM::CSSStyleDeclaration styleDecl2 = styleDecl;
    return getString(styleDecl2.getPropertyValue(DOMString(jsNameToProp(p))));
  }
}


void DOMCSSStyleDeclaration::tryPut(const UString &p, const KJSO& v)
{
  if (p == "cssText") {
    styleDecl.setCssText(v.toString().value().string());
  }
  else {
    QString prop = jsNameToProp(p);
    QString propvalue = v.toString().value().qstring();

    if(prop.left(6) == "pixel-")
    {
      prop = prop.mid(6); // cut it away
      propvalue += "px";
    }
    styleDecl.removeProperty(prop);
    if(!propvalue.isEmpty())
      styleDecl.setProperty(prop,DOM::DOMString(propvalue),""); // ### is "" ok for priority?
  }
}

Completion DOMCSSStyleDeclarationFunc::tryExecute(const List &args)
{
  KJSO result;
  String str = args[0].toString();
  DOM::DOMString s = str.value().string();

  switch (id) {
    case GetPropertyValue:
      result = getString(styleDecl.getPropertyValue(s));
      break;
    case GetPropertyCSSValue:
      result = Undefined(); // ###
      break;
    case RemoveProperty:
      result = getString(styleDecl.removeProperty(s));
      break;
    case GetPropertyPriority:
      result = getString(styleDecl.getPropertyPriority(s));
      break;
    case SetProperty:
      styleDecl.setProperty(args[0].toString().value().string(),
                            args[1].toString().value().string(),
                            args[2].toString().value().string());
      result = Undefined();
      break;
    case Item:
      result = getString(styleDecl.item(args[0].toNumber().intValue()));
      break;
    default:
      result = Undefined();
  }

  return Completion(Normal, result);
}

KJSO KJS::getDOMCSSStyleDeclaration(DOM::CSSStyleDeclaration s)
{
  DOMCSSStyleDeclaration *ret;
  if (s.isNull())
    return Null();
  else if ((ret = domCSSStyleDeclarations[s.handle()]))
    return ret;
  else {
    ret = new DOMCSSStyleDeclaration(s);
    domCSSStyleDeclarations.insert(s.handle(),ret);
    return ret;
  }
}


