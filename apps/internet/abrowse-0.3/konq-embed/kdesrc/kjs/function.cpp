/*
 *  This file is part of the KDE libraries
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 *  Boston, MA 02111-1307, USA.
 */

#include "function.h"

#include "kjs.h"
#include "types.h"
#include "internal.h"
#include "operations.h"
#include "nodes.h"

using namespace KJS;

const TypeInfo FunctionImp::info = { "Function", FunctionType,
				      &ObjectImp::info, 0, 0 };
const TypeInfo InternalFunctionImp::info = { "InternalFunction",
					      InternalFunctionType,
					      &FunctionImp::info, 0, 0 };
const TypeInfo ConstructorImp::info = { "Constructor", ConstructorType,
					 &InternalFunctionImp::info, 0, 0 };

namespace KJS {

  class Parameter {
  public:
    Parameter(const UString &n) : name(n), next(0L) { }
    ~Parameter() { delete next; }
    UString name;
    Parameter *next;
  };

};

FunctionImp::FunctionImp()
  : ObjectImp(/*TODO*/BooleanClass), param(0L)
{
}

FunctionImp::FunctionImp(const UString &n)
  : ObjectImp(/*TODO*/BooleanClass), ident(n), param(0L)
{
}

FunctionImp::~FunctionImp()
{
  delete param;
}

KJSO FunctionImp::thisValue() const
{
  return KJSO(Context::current()->thisValue());
}

void FunctionImp::addParameter(const UString &n)
{
  Parameter **p = &param;
  while (*p)
    p = &(*p)->next;

  *p = new Parameter(n);
}

// ECMA 10.1.3
void FunctionImp::processParameters(const List *args)
{
  KJSO variable = Context::current()->variableObject();

  assert(args);

  if (param) {
    ListIterator it = args->begin();
    Parameter **p = &param;
    while (*p) {
      if (it != args->end()) {
	variable.put((*p)->name, *it);
	it++;
      } else
	variable.put((*p)->name, Undefined());
      p = &(*p)->next;
    }
  }
}

// ECMA 13.2.1
KJSO FunctionImp::executeCall(Imp *thisV, const List *args)
{
  bool dummyList = false;
  if (!args) {
    args = new List();
    dummyList = true;
  }

  Context *save = Context::current();

  Context *ctx = new Context(codeType(), save, this, args, thisV);
  Context::setCurrent(ctx);

  // assign user supplied arguments to parameters
  processParameters(args);

  Completion comp = execute(*args);

  if (dummyList)
    delete args;

  delete ctx;
  Context::setCurrent(save);

  if (comp.complType() == Throw)
    return comp.value();
  else if (comp.isValueCompletion())
    return comp.value();
  else
    return Undefined();
}

UString FunctionImp::name() const
{
  return ident;
}

InternalFunctionImp::InternalFunctionImp()
{
}

InternalFunctionImp::InternalFunctionImp(int l)
{
    if (l >= 0)
	put("length", Number(l), ReadOnly|DontDelete|DontEnum);
}

InternalFunctionImp::InternalFunctionImp(const UString &n)
  : FunctionImp(n)
{
}

Completion InternalFunctionImp::execute(const List &)
{
  return Completion(Normal, Undefined());
}

ConstructorImp::ConstructorImp() {
  //  setPrototype(KJScript::global().functionPrototype());
  // TODO ???  put("constructor", this);
  put("length", Number(1), DontEnum);
}

ConstructorImp::ConstructorImp(const UString &n)
  : InternalFunctionImp(n)
{
}

ConstructorImp::ConstructorImp(const KJSO &p, int len)
{
  setPrototype(p);
  // TODO ???  put("constructor", *this);
  put("length", Number(len), DontEnum);
}

ConstructorImp::ConstructorImp(const UString &n, const KJSO &p, int len)
  : InternalFunctionImp(n)
{
  setPrototype(p);
  // TODO ???  put("constructor", *this);
  put("length", Number(len), DontEnum);
}

ConstructorImp::~ConstructorImp() { }

Completion ConstructorImp::execute(const List &)
{
  /* TODO */
  return Completion(Normal, Null());
}

Function::Function(Imp *d)
  : KJSO(d)
{
  if (d) {
    static_cast<FunctionImp*>(rep)->attr = ImplicitNone;
    assert(Global::current().hasProperty("[[Function.prototype]]"));
    setPrototype(Global::current().functionPrototype());
  }
}

Completion Function::execute(const List &args)
{
  assert(rep);
  return static_cast<FunctionImp*>(rep)->execute(args);
}

bool Function::hasAttribute(FunctionAttribute a) const
{
  assert(rep);
  return static_cast<FunctionImp*>(rep)->hasAttribute(a);
}

#if 0
InternalFunction::InternalFunction(Imp *d)
  : Function(d)
{
  param = 0L;
}

InternalFunction::~InternalFunction()
{
}
#endif

Constructor::Constructor(Imp *d)
  : Function(d)
{
  if (d) {
    assert(Global::current().hasProperty("[[Function.prototype]]"));
    setPrototype(Global::current().get("[[Function.prototype]]"));
    put("constructor", *this);
    put("length", 1, DontEnum);
  }
}

#if 0
Constructor::Constructor(const Object& proto, int len)
{
  setPrototype(proto);
  put("constructor", *this);
  put("length", len, DontEnum);
}
#endif

Constructor::~Constructor()
{
}

Completion Constructor::execute(const List &)
{
  /* TODO: call construct instead ? */
  return Completion(Normal, Undefined());
}

Object Constructor::construct(const List &args)
{
  assert(rep && rep->type() == ConstructorType);
  return ((ConstructorImp*)rep)->construct(args);
}

Constructor Constructor::dynamicCast(const KJSO &obj)
{
  // return null object on type mismatch
  if (!obj.isA(ConstructorType))
    return Constructor(0L);

  return Constructor(obj.imp());
}

KJSO Function::thisValue() const
{
  return KJSO(Context::current()->thisValue());
}
