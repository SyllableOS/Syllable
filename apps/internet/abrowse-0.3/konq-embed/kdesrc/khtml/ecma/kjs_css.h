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

#ifndef _KJS_CSS_H_
#define _KJS_CSS_H_

#include <dom/dom_node.h>
#include <kjs/object.h>
#include <dom/css_value.h>
#include "kjs_binding.h"

namespace KJS {

/*  class Style : public HostImp {
  public:
    Style(const DOM::Node &n) : node(n) { }
    virtual KJSO get(const UString &p) const;
    virtual void put(const UString &p, const KJSO& v);
  private:
    DOM::Node node;
  };
*/

  class DOMCSSStyleDeclaration : public DOMObject {
  public:
    DOMCSSStyleDeclaration(DOM::CSSStyleDeclaration s) : styleDecl(s) { }
    ~DOMCSSStyleDeclaration();
    virtual KJSO tryGet(const UString &p) const;
    virtual void tryPut(const UString &p, const KJSO& v);
    virtual const TypeInfo* typeInfo() const { return &info; }
    static const TypeInfo info;
  protected:
    DOM::CSSStyleDeclaration styleDecl;
  };

  class DOMCSSStyleDeclarationFunc : public DOMFunction {
    friend class DOMNode;
  public:
    DOMCSSStyleDeclarationFunc(DOM::CSSStyleDeclaration s, int i) : styleDecl(s), id(i) { }
    Completion tryExecute(const List &);
    enum { GetPropertyValue, GetPropertyCSSValue, RemoveProperty, GetPropertyPriority,
           SetProperty, Item };
  private:
    DOM::CSSStyleDeclaration styleDecl;
    int id;
  };

  KJSO getDOMCSSStyleDeclaration(DOM::CSSStyleDeclaration n);

}; // namespace

#endif
