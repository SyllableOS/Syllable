/*
 *  This file is part of the KDE libraries
 *  Copyright (C) 1999-2001 Harri Porten (porten@kde.org)
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

#ifdef KJS_DEBUGGER

#include "debugger.h"
#include "kjs.h"
#include "ustring.h"

using namespace KJS;

Debugger::Debugger(KJScript *engine)
  : eng(0L),
    sid(-1)
{
  attach(engine);
}

Debugger::~Debugger()
{
  detach();
}

void Debugger::attach(KJScript *e)
{
  dmode = Disabled;
  if (e) {
    if (!eng || e->rep != eng->rep) {
      eng = e;
      eng->rep->attachDebugger(this);
    }
  } else {
    eng = 0L;
  }
  reset();
}

KJScript *Debugger::engine() const
{
  return eng;
}

void Debugger::detach()
{
  reset();
  if (!eng)
    return;
  eng->rep->attachDebugger(0L);
  eng = 0L;
}

void Debugger::setMode(Mode m)
{
  dmode = m;
}

Debugger::Mode Debugger::mode() const
{
  return dmode;
}

// supposed to be overriden by the user
bool Debugger::stopEvent()
{
  return true;
}

void Debugger::reset()
{
  l = -1;
}

int Debugger::freeSourceId() const
{
  return eng ? eng->rep->sourceId()+1 : -1;
}

bool Debugger::setBreakpoint(int id, int line)
{
  if (!eng)
    return false;
  return eng->rep->setBreakpoint(id, line, true);
}

bool Debugger::deleteBreakpoint(int id, int line)
{
  if (!eng)
    return false;
  return eng->rep->setBreakpoint(id, line, false);
}

void Debugger::clearAllBreakpoints(int id)
{
  if (!eng)
    return;
  eng->rep->setBreakpoint(id, -1, false);
}

UString Debugger::varInfo(const UString &ident)
{
  if (!eng)
    return UString();
  const List *chain = Context::current()->pScopeChain();
  ListIterator scope = chain->begin();
  while (scope != chain->end()) {
    if (scope->hasProperty(ident)) {
      KJSO val = scope->get(ident);
      return UString(val.imp()->typeInfo()->name) + ":" +
	val.toString().value();
    }
    scope++;
  }
  return UString();
}

bool Debugger::setVar(const UString &ident, const KJSO &value)
{
  if (!eng)
    return false;
  const List *chain = Context::current()->pScopeChain();
  ListIterator scope = chain->begin();
  while (scope != chain->end()) {
    if (scope->hasProperty(ident)) {
      if (!scope->canPut(ident))
	return false;
      scope->put(ident, value);
      return true;
    }
    scope++;
  }
  // didn't find variable
  return false;
}

// called from the scripting engine each time a statement node is hit.
bool Debugger::hit(int line, bool breakPoint)
{
  l = line;
  if (!eng)
    return true;

  if (!breakPoint && ( mode() == Continue || mode() == Disabled ) )
      return true;

  bool ret = stopEvent();
  eng->init();	// in case somebody used a different interpreter meanwhile
  return ret;
}

#endif
