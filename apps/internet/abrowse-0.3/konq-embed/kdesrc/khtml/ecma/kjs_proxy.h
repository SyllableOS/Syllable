/*
 *  This file is part of the KDE libraries
 *  Copyright (C) 1999 Harri Porten (porten@kde.org)
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

#ifndef _KJS_PROXY_H_
#define _KJS_PROXY_H_

#include <qvariant.h>

class KJScript;
class KHTMLPart;

namespace DOM {
  class Node;
};

// callback functions for KJSProxy
typedef KJScript* (KJSCreateFunc)(KHTMLPart *);
typedef QVariant (KJSEvalFunc)(KJScript *script, const QChar *, unsigned int,
			   const DOM::Node &, KHTMLPart *);
typedef void (KJSClearFunc)(KJScript *script);
typedef const char* (KJSSpecialFunc)(KJScript *script, const char *);
typedef void (KJSDestroyFunc)(KJScript *script);
extern "C" {
  KJSCreateFunc kjs_create;
  KJSEvalFunc kjs_eval;
  KJSClearFunc kjs_clear;
  KJSSpecialFunc kjs_special;
  KJSDestroyFunc kjs_destroy;
}

/**
 * @short Proxy class serving as interface when being dlopen'ed.
 */
class KJSProxy {
public:
  KJSProxy(KJScript *s, KJSCreateFunc cr, KJSEvalFunc e, KJSClearFunc c,
	   KJSSpecialFunc sp, KJSDestroyFunc d)
    : create(cr), script(s), eval(e), clr(c), spec(sp), destr(d) { };
  ~KJSProxy() { (*destr)(script); }
  QVariant evaluate(const QChar *c, unsigned int l, const DOM::Node &n);
  const char *special(const char *c);
  void clear();
  KHTMLPart *khtmlpart;
private:
  KJSCreateFunc *create;
  KJScript *script;
  KJSEvalFunc *eval;
  KJSClearFunc *clr;
  KJSSpecialFunc *spec;
  KJSDestroyFunc *destr;
};

inline QVariant KJSProxy::evaluate(const QChar *c, unsigned int l,
			       const DOM::Node &n) {
  if (!script)
    script = (*create)(khtmlpart);
  return (*eval)(script, c, l, n, khtmlpart);
}

inline const char *KJSProxy::special(const char *c) {
  return (script ? (*spec)(script, c) : "");
}

inline void KJSProxy::clear() {
  if (script) {
    (*clr)(script);
    script = 0L;
  }
}

#endif
