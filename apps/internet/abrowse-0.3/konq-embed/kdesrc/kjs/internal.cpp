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

#include "internal.h"

#include <assert.h>
#include <stdio.h>

#include "kjs.h"
#include "object.h"
#include "types.h"
#include "operations.h"
#include "regexp.h"
#include "nodes.h"
#include "lexer.h"
#include "collector.h"
#include "debugger.h"

#define I18N_NOOP(s) s

extern int kjsyyparse();

using namespace KJS;

const TypeInfo UndefinedImp::info = { "Undefined", UndefinedType, 0, 0, 0 };
const TypeInfo NullImp::info = { "Null", NullType, 0, 0, 0 };
const TypeInfo NumberImp::info = { "Number", NumberType, 0, 0,0  };
const TypeInfo StringImp::info = { "String", StringType, 0, 0, 0 };
const TypeInfo BooleanImp::info = { "Boolean", BooleanType, 0, 0, 0 };
const TypeInfo CompletionImp::info = { "Completion", CompletionType, 0, 0, 0 };
const TypeInfo ReferenceImp::info = { "Reference", ReferenceType, 0, 0, 0 };

UndefinedImp::UndefinedImp()
{
}

KJSO UndefinedImp::toPrimitive(Type) const
{
  return (Imp*)this;
}

Boolean UndefinedImp::toBoolean() const
{
  return Boolean(false);
}

Number UndefinedImp::toNumber() const
{
  return Number(NaN);
}

String UndefinedImp::toString() const
{
  return String("undefined");
}

Object UndefinedImp::toObject() const
{
  return Error::createObject(TypeError, I18N_NOOP("Undefined value"));
}

NullImp::NullImp()
{
}

KJSO NullImp::toPrimitive(Type) const
{
  return (Imp*)this;
}

Boolean NullImp::toBoolean() const
{
  return Boolean(false);
}

Number NullImp::toNumber() const
{
  return Number(0);
}

String NullImp::toString() const
{
  return String("null");
}

Object NullImp::toObject() const
{
  return Error::createObject(TypeError, I18N_NOOP("Null value"));
}

KJSO BooleanImp::toPrimitive(Type) const
{
  return (Imp*)this;
}

Boolean BooleanImp::toBoolean() const
{
  return Boolean((BooleanImp*)this);
}

Number BooleanImp::toNumber() const
{
  return Number(val ? 1 : 0);
}

String BooleanImp::toString() const
{
  return String(val ? "true" : "false");
}

Object BooleanImp::toObject() const
{
  return Object::create(BooleanClass, Boolean((BooleanImp*)this));
}

NumberImp::NumberImp(double v)
  : val(v)
{
}

KJSO NumberImp::toPrimitive(Type) const
{
  return (Imp*)this;
}

Boolean NumberImp::toBoolean() const
{
  bool b = !((val == 0) /* || (iVal() == N0) */ || isNaN(val));

  return Boolean(b);
}

Number NumberImp::toNumber() const
{
  return Number((NumberImp*)this);
}

String NumberImp::toString() const
{
  return String(UString::from(val));
}

Object NumberImp::toObject() const
{
  return Object::create(NumberClass, Number((NumberImp*)this));
}

StringImp::StringImp(const UString& v)
  : val(v)
{
}

KJSO StringImp::toPrimitive(Type) const
{
  return (Imp*)this;
}

Boolean StringImp::toBoolean() const
{
  return Boolean(val.size() > 0);
}

Number StringImp::toNumber() const
{
  return Number(val.toDouble());
}

String StringImp::toString() const
{
  return String((StringImp*)this);
}

Object StringImp::toObject() const
{
  return Object::create(StringClass, String((StringImp*)this));
}

ReferenceImp::ReferenceImp(const KJSO& b, const UString& p)
  : base(b), prop(p)
{
}

void ReferenceImp::mark(Imp*)
{
  Imp::mark();
  Imp *im = base.imp();
  if (im && im->refcount == 0)
    im->mark();
}

CompletionImp::CompletionImp(Compl c, const KJSO& v, const UString& t)
  : comp(c), val(v), tar(t)
{
}

void CompletionImp::mark(Imp*)
{
  Imp::mark();
  Imp *im = val.imp();
  if (im && im->refcount == 0)
    im->mark();
}

RegExpImp::RegExpImp()
  : ObjectImp(RegExpClass), reg(0L)
{
}

RegExpImp::~RegExpImp()
{
  delete reg;
}

// ECMA 10.2
Context::Context(CodeType type, Context *callingContext,
		 FunctionImp *func, const List *args, Imp *thisV)
{
  Global glob(Global::current());

  // create and initialize activation object (ECMA 10.1.6)
  if (type == FunctionCode || type == AnonymousCode || type == HostCode) {
    activation = KJSO(new ActivationImp(func, args));
    variable = activation;
  } else {
    activation = KJSO();
    variable = glob;
  }

  // ECMA 10.2
  switch(type) {
    case EvalCode:
      if (callingContext) {
	scopeChain = callingContext->copyOfChain();
	variable = callingContext->variableObject();
	thisVal = callingContext->thisValue();
	break;
      } // else same as GlobalCode
    case GlobalCode:
      scopeChain = new List();
      scopeChain->append(glob);
      variable = glob;
      thisVal = glob.imp();
      break;
    case FunctionCode:
    case AnonymousCode:
      scopeChain = new List();
      scopeChain->append(activation);
      scopeChain->append(glob);
      variable = activation; /* TODO: DontDelete ? (ECMA 10.2.3) */
      if (thisV->type() >= ObjectType) {
	thisVal = thisV;
      }
      else
	thisVal = glob.imp();
      break;
    case HostCode:
      if (thisV->type() >= ObjectType)
	thisVal = thisV;
      else
	thisVal = glob.imp();
      variable = activation; /* TODO: DontDelete (ECMA 10.2.4) */
      scopeChain = new List();
      scopeChain->append(activation);
      if (func->hasAttribute(ImplicitThis))
	scopeChain->append(KJSO(thisVal));
      if (func->hasAttribute(ImplicitParents)) {
	/* TODO ??? */
      }
      scopeChain->append(glob);
      break;
    }
}

Context::~Context()
{
  delete scopeChain;
}

void Context::mark()
{
  assert(thisVal);
  if (thisVal->refcount == 0)
    thisVal->mark();
  if (activation.imp() && activation.imp()->refcount == 0)
    activation.imp()->mark();
  if (variable.imp() && variable.imp()->refcount == 0)
    variable.imp()->mark();
  /* TODO: scopeChain ? */
}

Context *Context::current()
{
  return KJScriptImp::curr ? KJScriptImp::curr->con : 0L;
}

void Context::setCurrent(Context *c)
{
  KJScriptImp::current()->con = c;
}

void Context::pushScope(const KJSO &s)
{
  scopeChain->prepend(s);
}

void Context::popScope()
{
  scopeChain->removeFirst();
}

List* Context::copyOfChain()
{
  return scopeChain->copy();
}


AnonymousFunction::AnonymousFunction()
  : Function(0L)
{
  /* TODO */
}

DeclaredFunctionImp::DeclaredFunctionImp(const UString &n, StatementNode *b)
  : ConstructorImp(n), block(b)
{
}

Completion DeclaredFunctionImp::execute(const List &)
{
 /* TODO */

#ifdef KJS_DEBUGGER
  Debugger *dbg = KJScriptImp::current()->debugger();
  int oldSourceId = -1;
  if (dbg) {
    oldSourceId = dbg->sourceId();
    dbg->setSourceId(block->sourceId());
  }
#endif

  Completion result = block->execute();

#ifdef KJS_DEBUGGER
  if (dbg) {
    dbg->setSourceId(oldSourceId);
  }
#endif

  return result;
}

Object DeclaredFunctionImp::construct(const List &args)
{
  Object obj(ObjectClass);
  KJSO p = get("prototype");
  if (p.isObject())
    obj.setPrototype(p);
  else
    obj.setPrototype(Global::current().get("[[Object.prototype]]"));

  KJSO res = executeCall(obj.imp(), &args);

  Object v = Object::dynamicCast(res);
  if (v.isNull())
    return obj;
  else
    return v;
}

Completion AnonymousFunction::execute(const List &)
{
 /* TODO */
  return Completion(Normal, Null());
}

const TypeInfo ActivationImp::info = { "Activation", ActivationType, 0, 0, 0 };

// ECMA 10.1.7 (draft April 98, 10.1.6 previously)
ActivationImp::ActivationImp(FunctionImp *f, const List *args)
{
  func = f;

  put("arguments", this, DontDelete | DontEnum);
  if (func->hasProperty("arguments"))
    put("OldArguments", func->get("arguments"));
  put("callee", Function(func), DontEnum);
  if (args) {
    put("length", Number(args->size()), DontEnum);
    ListIterator arg = args->begin();
    for (int i = 0; arg != args->end(); arg++, i++) {
      put(UString::from(i), *arg);
    }
  }
  func->put("arguments", this);
}

// ECMA 10.1.6
ActivationImp::~ActivationImp()
{
  /* TODO */
#if 0
  if (!hasProperty("OldArguments"))
    func->deleteProperty("arguments");
  else
    func->put("arguments", get("OldArguments")); /* TODO: deep copy ? */
#endif
}

KJScriptImp* KJScriptImp::curr = 0L;
KJScriptImp* KJScriptImp::hook = 0L;

KJScriptImp::KJScriptImp()
  : initialized(false),
    glob(0L),
#ifdef KJS_DEBUGGER
    dbg(0L),
#endif
    retVal(0L)
{
  KJScriptImp::curr = this;
  clearException();
  lex = new Lexer();
}

KJScriptImp::~KJScriptImp()
{
  KJScriptImp::curr = this;

#ifdef KJS_DEBUGGER
  attachDebugger(0L);
#endif

  clear();

  delete lex;
  lex = 0L;

  KJScriptImp::curr = 0L;
}

void KJScriptImp::mark()
{
  assert(glob.imp());
  glob.imp()->mark();
  if (exVal && exVal->refcount == 0)
    exVal->mark();
  if (retVal && retVal->refcount == 0)
    retVal->mark();
  if (con)
    con->mark();
}

void KJScriptImp::init()
{
  KJScriptImp::curr = this;

  clearException();
  retVal = 0L;

  if (!initialized) {
    // add this interpreter to the global chain
    // as a root set for garbage collection
    if (hook) {
      prev = hook;
      next = hook->next;
      hook->next->prev = this;
      hook->next = this;
    } else {
      hook = next = prev = this;
    }

    glob.init();
    con = new Context();
    firstNode = 0L;
    progNode = 0L;
    recursion = 0;
    errMsg = "";
    initialized = true;
#ifdef KJS_DEBUGGER
    sid = -1;
#endif
  }
}

void KJScriptImp::clear()
{
  if (initialized) {
    KJScriptImp::curr = this;

    Node::deleteAllNodes(&firstNode, &progNode);

    clearException();
    retVal = 0L;

    delete con; con = 0L;
    // remove from global chain (see init())
    next->prev = prev;
    prev->next = next;
    hook = next;
    if (hook == this)
      hook = 0L;

    Collector::collect();
#ifdef KJS_DEBUGGER
    sid = -1;
#endif

    initialized = false;
  }
}

bool KJScriptImp::evaluate(const UChar *code, unsigned int length, Imp *thisV,
			   bool onlyCheckSyntax)
{
  init();

#ifdef KJS_DEBUGGER
  sid++;
  if (debugger())
    debugger()->setSourceId(sid);
#endif
  if (recursion > 0) {
#ifndef NDEBUG
    fprintf(stderr, "Blocking recursive JS call.\n");
#endif
    return false;
  }

  assert(Lexer::curr());
  Lexer::curr()->setCode(code, length);
  int parseError = kjsyyparse();

  if (parseError) {
    errType = 99; /* TODO */
    errLine = Lexer::curr()->lineNo();
    errMsg = "Parse error at line " + UString::from(errLine);
#ifndef NDEBUG
    fprintf(stderr, "JavaScript parse error at line %d.\n", errLine);
#endif
    /* TODO: either clear everything or keep previously
       parsed function definitions */
    //    Node::deleteAllNodes();
    return false;
  }

  if (onlyCheckSyntax)
      return true;

  Context *context = Context::current();
  clearException();

  if (thisV) {
    context->setThisValue(thisV);
    context->pushScope(thisV);
  }

  recursion++;
  assert(progNode);
  KJSO res = progNode->evaluate();
  recursion--;

  if (hadException()) {
    KJSO err = exception();
    errType = 99; /* TODO */
    errLine = err.get("line").toInt32();
    errMsg = err.get("name").toString().value() + ". ";
    errMsg += err.get("message").toString().value();
#ifdef KJS_DEBUGGER
    if (dbg)
      dbg->setSourceId(err.get("sid").toInt32());
#endif
    clearException();
  } else {
    errType = 0;
    errLine = -1;
    errMsg = "";

    // catch return value
    retVal = 0L;
    if (res.isA(CompletionType)) {
      Completion *com = static_cast<Completion*>(&res);
      if (com->isValueCompletion())
	retVal = com->value().imp();
    }
  }

  if (thisV)
    context->popScope();

  if (progNode)
    progNode->deleteStatements();

  return !errType;
}

bool KJScriptImp::call(const UString &func, const List &args)
{
  init();
  Context *ctx = Context::current();
  const List *chain = ctx->pScopeChain();
  ListIterator scope = chain->begin();
  while (scope != chain->end()) {
    if (scope->hasProperty(func))
	break;
    scope++;
  }
  if (scope == chain->end()) {
#ifndef NDEBUG
      fprintf(stderr, "couldn't resolve function name %s. call() failed\n",
	      func.ascii());
#endif
      return false;
  }

  KJSO v = scope->get(func);
  if (!v.isA(ConstructorType)) {
#ifndef NDEBUG
      fprintf(stderr, "%s is not a function. call() failed.\n", func.ascii());
#endif
      return false;
  }
  ConstructorImp *ctor = static_cast<ConstructorImp*>(v.imp());
  Null nil; /* TODO */
  ctor->executeCall(nil.imp(), &args);
  return !hadException();
}

void KJScriptImp::setException(Imp *e)
{
  assert(curr);
  curr->exVal = e;
  curr->exMsg = "Exception"; // not very meaningful but we use !0L to test
}

void KJScriptImp::setException(const char *msg)
{
  assert(curr);
  curr->exVal = 0L;		// will be created later on exception()
  curr->exMsg = msg;
}

KJSO KJScriptImp::exception()
{
  assert(curr);
  if (!curr->exMsg)
    return Undefined();
  if (curr->exVal)
    return curr->exVal;
  return Error::create(GeneralError, curr->exMsg);
}

void KJScriptImp::clearException()
{
  assert(curr);
  curr->exMsg = 0L;
  curr->exVal = 0L;
}

#ifdef KJS_DEBUGGER
void KJScriptImp::attachDebugger(Debugger *d)
{
  static bool detaching = false;
  if (detaching) // break circular detaching
    return;

  if (dbg) {
    detaching = true;
    dbg->detach();
    detaching = false;
  }

  dbg = d;
}

bool KJScriptImp::setBreakpoint(int id, int line, bool set)
{
  init();
  return Node::setBreakpoint(firstNode, id, line, set);
}

#endif

bool PropList::contains(const UString &name)
{
  PropList *p = this;
  while (p) {
    if (name == p->name)
      return true;
    p = p->next;
  }
  return false;
}

bool LabelStack::push(const UString &id)
{
  if (id.isEmpty() || contains(id))
    return false;

  StackElm *newtos = new StackElm;
  newtos->id = id;
  newtos->prev = tos;
  tos = newtos;
  return true;
}

bool LabelStack::contains(const UString &id) const
{
  if (id.isEmpty())
    return true;

  for (StackElm *curr = tos; curr; curr = curr->prev)
    if (curr->id == id)
      return true;

  return false;
}

void LabelStack::pop()
{
  if (tos) {
    StackElm *prev = tos->prev;
    delete tos;
    tos = prev;
  }
}

LabelStack::~LabelStack()
{
  StackElm *prev;

  while (tos) {
    prev = tos->prev;
    delete tos;
    tos = prev;
  }
}
