/*
 *  This file is part of the KDE libraries
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
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

#include "kjs.h"
#include "operations.h"
#include "object_object.h"
#include "types.h"

using namespace KJS;

Completion ObjectObject::execute(const List &args)
{
  KJSO result;

  List argList;
  if (args.isEmpty()) {
    result = construct(argList);
  } else {
    KJSO arg = args[0];
    if (arg.isA(NullType) || arg.isA(UndefinedType)) {
      argList.append(arg);
      result = construct(argList);
    } else
      result = arg.toObject();
  }
  return Completion(Normal, result);
}

// ECMA 15.2.2
Object ObjectObject::construct(const List &args)
{
  // if no arguments have been passed ...
  if (args.isEmpty())
    return Object::create(ObjectClass);

  KJSO arg = *args.begin();
  Object obj = Object::dynamicCast(arg);
  if (!obj.isNull()) {
    /* TODO: handle host objects */
    return obj;
  }

  switch (arg.type()) {
  case StringType:
  case BooleanType:
  case NumberType:
    return arg.toObject();
  default:
    assert(!"unhandled switch case in ObjectConstructor");
  case NullType:
  case UndefinedType:
    return Object::create(ObjectClass);
  }
}

ObjectPrototype::ObjectPrototype()
  : ObjectImp(ObjectClass)
{
  // the spec says that [[Property]] should be `null'.
  // Not sure if Null or C's NULL is needed.
}

KJSO ObjectPrototype::get(const UString &p) const
{
  if (p == "toString")
    return Function(new ObjectProtoFunc(ToString));
  else if (p == "valueOf")
    return Function(new ObjectProtoFunc(ValueOf));
  else
    return Imp::get(p);
}

ObjectProtoFunc::ObjectProtoFunc(int i)
  : id(i)
{
}

// ECMA 15.2.4.2 + 15.2.4.3
Completion ObjectProtoFunc::execute(const List &)
{
  Object thisObj = Object::dynamicCast(thisValue());

  /* TODO: what to do with non-objects. Is this possible at all ? */
  if (thisObj.isNull()) {
    UString str = "[object ";
    str += thisValue().isNull() ? "null" : thisValue().imp()->typeInfo()->name;
    str += "]";
    return Completion(ReturnValue, String(str));
  }

  // valueOf()
  if (id == ObjectPrototype::ValueOf)
    /* TODO: host objects*/
    return Completion(Normal, thisObj);

  // toString()
  UString str;
  switch(thisObj.getClass()) {
  case StringClass:
    str = "[object String]";
    break;
  case BooleanClass:
    str = "[object Boolean]";
    break;
  case NumberClass:
    str = "[object Number]";
    break;
  case ObjectClass:
  {
    str = "[object Object]";
    break;
  }
  default:
    str = "[undefined object]";
  }

  return Completion(Normal, String(str));
}
