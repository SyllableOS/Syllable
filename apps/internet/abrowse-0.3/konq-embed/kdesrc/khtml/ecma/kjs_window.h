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

#ifndef _KJS_WINDOW_H_
#define _KJS_WINDOW_H_

#include <qobject.h>
#include <qguardedptr.h>
#include <kjs/object.h>
#include <kjs/function.h>

#include "kjs_binding.h"

class QTimer;
class KHTMLView;
class KHTMLPart;

namespace KJS {

  class WindowFunc;
  class WindowQObject;

  class Window : public HostImp {
    friend class WindowFunc;
    friend class WindowQObject;
    friend Window *newWindow(KHTMLPart*);
    Window(KHTMLPart *p);
  public:
    ~Window();
    virtual bool hasProperty(const UString &p, bool recursive = true) const;
    virtual KJSO get(const UString &p) const;
    virtual void put(const UString &p, const KJSO& v);
    virtual Boolean toBoolean() const;
    int installTimeout(const UString &handler, int t, bool singleShot);
    void clearTimeout(int timerId);
    void scheduleClose();
    bool isSafeScript() const;
  private:
    QGuardedPtr<KHTMLPart> part;
    QGuardedPtr<KHTMLPart> opener;
    WindowQObject *winq;
  };

  /**
   * Returns and registers a window object. In case there's already a Window
   * for the specified part p this will be returned in order to have unique
   * bindings.
   */
  Window *newWindow(KHTMLPart *p);

  class WindowFunc : public DOMFunction {
  public:
    WindowFunc(const Window *w, int i) : window(w), id(i) { };
    Completion tryExecute(const List &);
    enum { Alert, Confirm, Prompt, Open, SetTimeout, ClearTimeout, Focus, Blur, Close,
          MoveBy, MoveTo, ResizeBy, ResizeTo, ScrollBy, ScrollTo, SetInterval, ClearInterval };

    static void initJScript(KHTMLPart *);
  private:
    const Window *window;
    int id;
  };

  class WindowQObject : public QObject {
    Q_OBJECT
  public:
    WindowQObject(Window *w);
    ~WindowQObject();
    int installTimeout(const UString &handler, int t, bool singleShot);
    void clearTimeout(int timerId);
  public slots:
    void timeout();
    void timeoutClose();
  protected slots:
    void parentDestroyed();
  private:
    Window *parent;
    QTimer *timer;
    UString timeoutHandler;
  };

  class Location : public HostImp {
  public:
    ~Location();
    static Location* retrieve(KHTMLPart *);

    virtual KJSO get(const UString &p) const;
    virtual void put(const UString &p, const KJSO &v);
    virtual KJSO toPrimitive(Type preferred) const;
    virtual String toString() const;
  private:
    Location(KHTMLPart *p);
    QGuardedPtr<KHTMLPart> part;
  };

  class LocationFunc : public DOMFunction {
  public:
    LocationFunc(KHTMLPart *p, int i);
    Completion tryExecute(const List &);
    enum { Replace, Reload };
  private:
    QGuardedPtr<KHTMLPart> part;
    int id;
  };

}; // namespace

#endif
