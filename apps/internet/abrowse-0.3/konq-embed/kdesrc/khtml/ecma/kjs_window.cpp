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

#include <qtimer.h>
#ifndef __ATHEOS__
#include <qinputdialog.h>
#endif
#include <qstringlist.h>
#include <qptrdict.h>
#include <qpaintdevicemetrics.h>
#ifndef __ATHEOS__
#include <qapplication.h>
#endif
#include <dom_string.h>
#include <kdebug.h>
#include <kurl.h>
#include <kmessagebox.h>
#ifndef __ATHEOS__
#include <klocale.h>
#endif
#include <kparts/browserextension.h>
#ifndef __ATHEOS__
#include <kwinmodule.h>
#endif
#include <kconfig.h>

#include <kjs/operations.h>
#include "kjs_window.h"
#include "kjs_navigator.h"
#include "kjs_html.h"
#include "kjs_dom.h"

#include <qevent.h>
#include "khtmlview.h"
#include <html_element.h>
#include <html_documentimpl.h>
#include "khtml_part.h"
#include "xml/dom_docimpl.h"

#ifdef __ATHEOS__
#include <gui/desktop.h>
#include <gui/bitmap.h>
#include <gui/window.h>
#endif // __ATHEOS__

using namespace KJS;

static QPtrDict<Window> *window_dict = 0L;

// ### put into class Window namespace
Window *KJS::newWindow(KHTMLPart *p)
{
  Window *w;
  if (!window_dict)
    window_dict = new QPtrDict<Window>;
  else if ((w = window_dict->find(p)) != 0L)
    return w;

  w = new Window(p);
  window_dict->insert(p, w);

  return w;
}


class History : public HostImp {
  friend class HistoryFunc;
public:
  History(KHTMLPart */*p*/) /*: part(p)*/ { }
  virtual KJSO get(const UString &p) const;
  virtual bool hasProperty(const UString &p, bool recursive) const;
private:
  //  QGuardedPtr<KHTMLPart> part;
};

class HistoryFunc : public DOMFunction {
public:
  HistoryFunc(const History *h, int i) : history(h), id(i) { };
  Completion tryExecute(const List &);
  enum { Back, Forward, Go };

private:
  const History *history;
  int id;
};

class FrameArray : public HostImp {
public:
  FrameArray(KHTMLPart *p) : part(p) { }
  KJSO get(const UString &p) const;
private:
  QGuardedPtr<KHTMLPart> part;
};

class Screen : public ObjectImp {
public:
  Screen() : ObjectImp( UndefClass ) { }
  KJSO get(const UString &p) const;
private:
  KHTMLView *view;
};

KJSO Screen::get(const UString &p) const
{
#ifdef __ATHEOS__    
  if (p == "height")
    return Number( os::Desktop().GetResolution().y );
  else if (p == "width")
    return Number( os::Desktop().GetResolution().x );
  else if (p == "colorDepth" || p == "pixelDepth")
    return Number( os::BitsPerPixel( os::Desktop().GetColorSpace()) );
  else if (p == "availLeft")
    return Number( 0 );
  else if (p == "availTop")
    return Number( 0 );
  else if (p == "availHeight")
    return Number( os::Desktop().GetResolution().y );
  else if (p == "availWidth")
    return Number( os::Desktop().GetResolution().x );
  else
    return Undefined();
#else // __ATHEOS__
  KWinModule info;
  QPaintDeviceMetrics m(QApplication::desktop());

  if (p == "height")
    return Number(QApplication::desktop()->height());
  else if (p == "width")
    return Number(QApplication::desktop()->width());
  else if (p == "colorDepth" || p == "pixelDepth")
    return Number(m.depth());
  else if (p == "availLeft")
    return Number(info.workArea().left());
  else if (p == "availTop")
    return Number(info.workArea().top());
  else if (p == "availHeight")
    return Number(info.workArea().height());
  else if (p == "availWidth")
    return Number(info.workArea().width());
  else
    return Undefined();
#endif // __ATHEOS__
}

Window::Window(KHTMLPart *p)
  : part(p), winq(0L)
{
}

Window::~Window()
{
  window_dict->remove(part);
  if (window_dict->isEmpty()) {
    delete window_dict;
    window_dict = 0L;
  }
  delete winq;
}

bool Window::hasProperty(const UString &p, bool recursive) const
{
  if (p == "closed")
    return true;

  // we don't want any operations on a closed window
  if (part.isNull())
    return false;

  // Properties
  if (p == "crypto" ||
      p == "defaultStatus" ||
      p == "document" ||
      p == "Node" ||
      p == "frames" ||
      p == "history" ||
      p == "innerHeight" ||
      p == "innerWidth" ||
      p == "length" ||
      p == "location" ||
      p == "name" ||
      p == "navigator" ||
      p == "offscreenBuffering" ||
      p == "opener" ||
      p == "outerHeight" ||
      p == "outerWidth" ||
      p == "pageXOffset" ||
      p == "pageYOffset" ||
      p == "parent" ||
      p == "personalbar" ||
      p == "screenX" ||
      p == "screenY" ||
      p == "scrollbars" ||
      p == "self" ||
      p == "top" ||
      p == "screen" ||
      p == "Image" ||
      p == "Option" ||
  // Methods
      p == "alert" ||
      p == "confirm" ||
      p == "blur" ||
      p == "clearInterval" ||
      p == "clearTimeout" ||
      p == "close" ||
      p == "focus" ||
      p == "moveBy" ||
      p == "moveTo" ||
      p == "open" ||
      p == "prompt" ||
      p == "resizeBy" ||
      p == "resizeTo" ||
      p == "scroll" ||
      p == "scrollBy" ||
      p == "scrollTo" ||
      p == "setInterval" ||
      p == "setTimeout" ||
      HostImp::hasProperty(p,recursive) ||
      part->findFrame( p.qstring() ))
    return true;

  // allow shortcuts like 'Image1' instead of document.images.Image1
  if (part->document().isHTMLDocument()) { // might be XML
    DOM::HTMLCollection coll = part->htmlDocument().all();
    DOM::HTMLElement element = coll.namedItem(p.string());
    if (!element.isNull()) {
        return true;
    }
  }

  return false;
}

KJSO Window::get(const UString &p) const
{
  if (p == "closed")
    return Boolean(part.isNull());

  // we don't want any operations on a closed window
  if (part.isNull())
    return Undefined();

  if (p == "crypto")
    return Undefined(); // ###
  else if (p == "defaultStatus" || p == "defaultstatus")
    return String(UString(part->jsDefaultStatusBarText()));
  else if (p == "status")
    return String(UString(part->jsStatusBarText()));
  else if (p == "document") {
    if (isSafeScript())
      return getDOMNode(part->document());
    else
      return Undefined();
  }
  else if (p == "Node")
    return getNodePrototype();
  else if (p == "frames")
    return new FrameArray(part);
  else if (p == "history")
    return KJSO(new History(part));
  else if (p == "innerHeight")
    return Number(part->view()->visibleHeight());
  else if (p == "innerWidth")
    return Number(part->view()->visibleWidth());
  else if (p == "length")
    return Number(part->frames().count());
  else if (p == "location") {
    if (isSafeScript())
      return KJSO(Location::retrieve(part));
    else
      return Undefined();
  }
  else if (p == "name")
    return String(part->name());
  else if (p == "navigator")
    return KJSO(new Navigator(part));
  else if (p == "offscreenBuffering")
    return Boolean(true);
  else if (p == "opener")
    if (opener.isNull())
      return Undefined();
    else
      return newWindow(opener);
  else if (p == "outerHeight")
    return Number(part->view() ? part->view()->height() : 0); // ###
  else if (p == "outerWidth")
    return Number(part->view() ? part->view()->width() : 0); // ###
  else if (p == "pageXOffset")
    return Number(part->view()->contentsX());
  else if (p == "pageYOffset")
    return Number(part->view()->contentsY());
  else if (p == "parent")
    return KJSO(newWindow(part->parentPart() ? part->parentPart() : (KHTMLPart*)part));
  else if (p == "personalbar")
    return Undefined(); // ###
  else if (p == "screenX")
    return Number(part->view() ? part->view()->mapToGlobal(QPoint(0,0)).x() : 0);
  else if (p == "screenY")
    return Number(part->view() ? part->view()->mapToGlobal(QPoint(0,0)).y() : 0);
  else if (p == "scrollbars")
    return Undefined(); // ###
  else if (p == "scroll")
    return Function(new WindowFunc(this, WindowFunc::ScrollTo)); // compatibility
  else if (p == "scrollBy")
    return Function(new WindowFunc(this, WindowFunc::ScrollBy));
  else if (p == "scrollTo")
    return Function(new WindowFunc(this, WindowFunc::ScrollTo));
  else if (p == "moveBy")
    return Function(new WindowFunc(this, WindowFunc::MoveBy));
  else if (p == "moveTo")
    return Function(new WindowFunc(this, WindowFunc::MoveTo));
  else if (p == "resizeBy")
    return Function(new WindowFunc(this, WindowFunc::ResizeBy));
  else if (p == "resizeTo")
    return Function(new WindowFunc(this, WindowFunc::ResizeTo));
  else if (p == "self" || p == "window")
    return KJSO(newWindow(part));
  else if (p == "top") {
    KHTMLPart *p = part;
    while (p->parentPart())
      p = p->parentPart();
    return KJSO(newWindow(p));
  }
  else if (p == "screen")
    return KJSO(new Screen());
  else if (p == "Image")
    return KJSO(new ImageConstructor(Global::current(), part->document()));
  else if (p == "Option")
    return KJSO(new OptionConstructor(part->document()));
  else if (p == "alert")
    return Function(new WindowFunc(this, WindowFunc::Alert));
  else if (p == "confirm")
    return Function(new WindowFunc(this, WindowFunc::Confirm));
  else if (p == "prompt")
    return Function(new WindowFunc(this, WindowFunc::Prompt));
  else if (p == "open")
    return Function(new WindowFunc(this, WindowFunc::Open));
  else if (p == "setTimeout") {
    if (isSafeScript())
      return Function(new WindowFunc(this, WindowFunc::SetTimeout));
    else
      return Undefined();
  }
  else if (p == "clearTimeout") {
    if (isSafeScript())
      return Function(new WindowFunc(this, WindowFunc::ClearTimeout));
    else
      return Undefined();
  }
  else if (p == "focus")
    return Function(new WindowFunc(this, WindowFunc::Focus));
  else if (p == "blur")
    return Function(new WindowFunc(this, WindowFunc::Blur));
  else if (p == "close")
    return Function(new WindowFunc(this, WindowFunc::Close));
  else if (p == "setInterval") {
    if (isSafeScript())
      return Function(new WindowFunc(this, WindowFunc::SetInterval));
    else
      return Undefined();
  }
  else if (p == "clearInterval") {
    if (isSafeScript())
      return Function(new WindowFunc(this, WindowFunc::ClearInterval));
    else
      return Undefined();
  }

  if (Imp::hasProperty(p,true)) {
    if (isSafeScript())
      return Imp::get(p);
    else
      return Undefined();
  }

  KHTMLPart *kp = part->findFrame( p.qstring() );
  if (kp)
    return KJSO(newWindow(kp));

  // allow shortcuts like 'Image1' instead of document.images.Image1
  if (isSafeScript() &&
      part->document().isHTMLDocument()) { // might be XML
    DOM::HTMLCollection coll = part->htmlDocument().all();
    DOM::HTMLElement element = coll.namedItem(p.string());
    if (!element.isNull()) {
        return getDOMNode(element);
    }
  }

  return Undefined();
}

void Window::put(const UString &p, const KJSO &v)
{
  if (p == "status") {
    String s = v.toString();
    part->setJSStatusBarText(s.value().qstring());
  } else if (p == "defaultStatus" || p == "defaultstatus") {
    String s = v.toString();
    part->setJSDefaultStatusBarText(s.value().qstring());
  } else if (p == "location") {
    QString str = v.toString().value().qstring().prepend( "target://_self/#" );
    part->scheduleRedirection(0, str);
  } else if (p == "onload") {
    if (isSafeScript() && v.isA(ConstructorType)) {
      DOM::DOMString s = ((FunctionImp*)v.imp())->name().string() + "()";
      static_cast<DOM::HTMLDocumentImpl *>(part->htmlDocument().handle())->setOnload(s.string());
    }
  } else {
    if (isSafeScript())
      Imp::put(p, v);
  }
}

Boolean Window::toBoolean() const
{
  return Boolean(!part.isNull());
}

int Window::installTimeout(const UString &handler, int t, bool singleShot)
{
  if (!winq)
    winq = new WindowQObject(this);

  return winq->installTimeout(handler, t, singleShot);
}

void Window::clearTimeout(int timerId)
{
  if (winq)
    winq->clearTimeout(timerId);
}

void Window::scheduleClose()
{
  if (!winq)
    winq = new WindowQObject(this);
  kdDebug(6070) << "WindowFunc::tryExecute window.close() " << part << endl;
  QTimer::singleShot( 0, winq, SLOT( timeoutClose() ) );
}

bool Window::isSafeScript() const
{
  return originCheck(part->url().url(),KJS::Global::current().get("[[ScriptURL]]").toString().value().qstring());
}

Completion WindowFunc::tryExecute(const List &args)
{
  KJSO result;
  QString str, str2;
  int i;

  if (!window->part)
    return Completion(Normal, Undefined());

  KHTMLPart *part = window->part;
  if (!part)
    return Completion(Normal, Undefined());

  KHTMLView *widget = part->view();
  KJSO v = args[0];
  String s = v.toString();
  str = s.value().qstring();

  switch (id) {
#ifndef __ATHEOS__
  case Alert:
    part->xmlDocImpl()->updateRendering();
    KMessageBox::error(widget, str, "JavaScript");
    result = Undefined();
    break;
  case Confirm:
    part->xmlDocImpl()->updateRendering();
    i = KMessageBox::warningYesNo(widget, str, "JavaScript",
                                  i18n("OK"), i18n("Cancel"));
    result = Boolean((i == KMessageBox::Yes));
    break;
  case Prompt:
    part->xmlDocImpl()->updateRendering();
    if (args.size() >= 2)
      str2 = QInputDialog::getText("Konqueror: Prompt", str,
                                   args[1].toString().value().qstring());
    else
      str2 = QInputDialog::getText("Konqueror: Prompt", str);
    result = String(str2);
    break;
#endif // __ATHEOS__  
  case Open:
  {
//    KConfig *config = new KConfig("konquerorrc");
//    config->setGroup("Java/JavaScript Settings");

    if (false/*config->readBoolEntry("DisableWindowOpen")*/) {
      result = Undefined();
    } else {
        KParts::WindowArgs winargs;

        // scan feature argument
        v = args[2];
        QString features;
        if (!v.isA(UndefinedType)) {
            features = v.toString().value().qstring();
            QStringList flist = QStringList::split(',', features);
            QStringList::ConstIterator it = flist.begin();
            while (it != flist.end()) {
                int pos = (*it).find('=');
                if (pos >= 0) {
                    QString key = (*it).left(pos).stripWhiteSpace().lower();
                    QString val = (*it).mid(pos + 1).stripWhiteSpace().lower();
                    if (key == "left" || key == "screenx")
                      winargs.x = val.toInt();
                    else if (key == "top" || key == "screeny")
                      winargs.y = val.toInt();
                    else if (key == "height")
                      winargs.height = val.toInt();
                    else if (key == "width")
                      winargs.width = val.toInt();
                    else if (key == "menubar")
                      winargs.menuBarVisible = (val == "1" || val == "yes");
                    else if (key == "toolbar")
                      winargs.toolBarsVisible = (val == "1" || val == "yes");
                    else if (key == "status" || key == "statusbar")
                      winargs.statusBarVisible = (val == "1" || val == "yes");
                    else if (key == "resizable")
                      winargs.resizable = (val == "1" || val == "yes");
                    else if (key == "fullscreen")
                      winargs.fullscreen = (val == "1" || val == "yes");
                }
                it++;
            }
        }

        // prepare arguments
        KURL url;
        if (!str.isEmpty()) {
	    if (part->baseURL().isEmpty())
    	    url = KURL(part->url(), str);
	    else
	        url = KURL(part->baseURL(), str);
        }
        KParts::URLArgs uargs;
        uargs.frameName = args[1].toString().value().qstring();
        uargs.serviceType = "text/html";

        // request new window
        KParts::ReadOnlyPart *newPart = 0L;
        emit part->browserExtension()->createNewWindow("", uargs,winargs,newPart);
//        if (newPart && newPart->inherits("KHTMLPart")) {
        if ( dynamic_cast<KHTMLPart*>(newPart) != NULL ) {
	    Window *win = newWindow(static_cast<KHTMLPart*>(newPart));
	    win->opener = part;
	    uargs.serviceType = QString::null;
	    if (uargs.frameName == "_blank")
              uargs.frameName = QString::null;
	    emit static_cast<KHTMLPart*>(newPart)->browserExtension()->openURLRequest(url,uargs);
	    result = win;
        } else
            result = Undefined();
     }
//  delete config;
  break;
  }
  case ScrollBy:
    if(args.size() == 2 && widget)
      widget->scrollBy(args[0].toInt32(), args[1].toInt32());
    result = Undefined();
    break;
  case ScrollTo:
    if(args.size() == 2 && widget)
      widget->setContentsPos(args[0].toInt32(), args[1].toInt32());
    result = Undefined();
    break;
  case MoveBy:
    if(args.size() == 2 && widget)
    {
#ifdef __ATHEOS__
      os::Window * tl = widget->GetWindow();
      os::Rect cFrame = tl->GetFrame();
      os::Point dest = cFrame.LeftTop() + os::Point( args[0].toInt32(), args[1].toInt32() );
      // Security check (the spec talks about UniversalBrowserWrite to disable this check...)
      os::Desktop cDesktop;
      if ( dest.x >= 0 && dest.y >= 0 &&
           dest.x+cFrame.Width() + 1.0f <= cDesktop.GetResolution().x &&
           dest.y+cFrame.Height() + 1.0f <= cDesktop.GetResolution().y )
          tl->MoveTo( dest );
#else // __ATHEOS__
      QWidget * tl = widget->topLevelWidget();
      QPoint dest = tl->pos() + QPoint( args[0].toInt32(), args[1].toInt32() );
      // Security check (the spec talks about UniversalBrowserWrite to disable this check...)
      os::Desktop cDesktop;
      if ( dest.x() >= 0 && dest.y() >= 0 &&
           dest.x()+tl->width() <= cDesktop.GetResolution().x &&
           dest.y()+tl->height() <= cDesktop.GetResolution().y )
          tl->move( dest );
#endif // __ATHEOS__
    }
    break;
  case MoveTo:
    if(args.size() == 2 && widget)
    {
#ifdef __ATHEOS__
      os::Window * tl = widget->GetWindow();
      os::Rect cFrame = tl->GetFrame();
      os::Point dest = os::Point( args[0].toInt32(), args[1].toInt32() );
      // Security check (the spec talks about UniversalBrowserWrite to disable this check...)
      os::Desktop cDesktop;
      if ( dest.x >= 0 && dest.y >= 0 &&
           dest.x+cFrame.Width() <= cDesktop.GetResolution().x &&
           dest.y+cFrame.Height() <= cDesktop.GetResolution().y )
          tl->MoveTo( dest );	
#else // __ATHEOS__
      QWidget * tl = widget->topLevelWidget();
      QPoint dest = QPoint( args[0].toInt32(), args[1].toInt32() );
      // Security check (the spec talks about UniversalBrowserWrite to disable this check...)
      os::Desktop cDesktop;
      if ( dest.x() >= 0 && dest.y() >= 0 &&
           dest.x()+tl->width() <= cDesktop.GetResolution().x &&
           dest.y()+tl->height() <= cDesktop.GetResolution().y )
          tl->move( dest );
#endif // __ATHEOS__
    }
    break;
  case ResizeBy:
    if(args.size() == 2 && widget)
    {
#ifdef __ATHEOS__
      os::Window * tl = widget->GetWindow();
      os::Rect cFrame = tl->GetFrame();
      os::Point dest( cFrame.Width() + args[0].toInt32() - 1.0f, cFrame.Height() + args[1].toInt32() - 1.0f );
      // Security check (the spec talks about UniversalBrowserWrite to disable this check...)
      os::Desktop cDesktop;
      if ( cFrame.left+dest.x + 1.0f <= cDesktop.GetResolution().x &&
           cFrame.top+dest.y + 1.0f <= cDesktop.GetResolution().y &&
           dest.x >= 99 && dest.y >= 99 )
          tl->ResizeTo( dest );
#else // __ATHEOS__
      QWidget * tl = widget->topLevelWidget();
      QSize dest = tl->size() + QSize( args[0].toInt32(), args[1].toInt32() );
      // Security check: within desktop limits and bigger than 100x100 (per spec)
      os::Desktop cDesktop;
      if ( tl->x()+dest.width() <= cDesktop.GetResolution().x &&
           tl->y()+dest.height() <= cDesktop.GetResolution().y &&
           dest.width() >= 100 && dest.height() >= 100 )
          tl->resize( dest );
#endif // __ATHEOS__
    }
    break;
  case ResizeTo:
    if(args.size() == 2 && widget)
    {
#ifdef __ATHEOS__
      os::Window * tl = widget->GetWindow();
      os::Rect cFrame = tl->GetFrame();
      os::Point dest( args[0].toInt32() - 1.0f, args[1].toInt32() - 1.0f );
      // Security check (the spec talks about UniversalBrowserWrite to disable this check...)
      os::Desktop cDesktop;
      if ( cFrame.left+dest.x + 1.0f <= cDesktop.GetResolution().x &&
           cFrame.top+dest.y + 1.0f <= cDesktop.GetResolution().y &&
           dest.x >= 99 && dest.y >= 99 )
          tl->ResizeTo( dest );
#else // __ATHEOS__
      QWidget * tl = widget->topLevelWidget();
      QSize dest = QSize( args[0].toInt32(), args[1].toInt32() );
      // Security check: within desktop limits and bigger than 100x100 (per spec)
      os::Desktop cDesktop;
      if ( tl->x()+dest.width() <= cDesktop.GetResolution().x &&
           tl->y()+dest.height() <= cDesktop.GetResolution().y &&
           dest.width() >= 100 && dest.height() >= 100 )
          tl->resize( dest );
#endif // __ATHEOS__
    }
    break;
  case SetTimeout:
    if (args.size() == 2 && v.isA(StringType)) {
      int i = args[1].toInt32();
      int r = (const_cast<Window*>(window))->installTimeout(s.value(), i, true /*single shot*/);
      result = Number(r);
    } else
      result = Undefined();
    break;
  case SetInterval:
    if (args.size() == 2 && v.isA(StringType)) {
      int i = args[1].toInt32();
      int r = (const_cast<Window*>(window))->installTimeout(s.value(), i, false);
      result = Number(r);
    } else
      result = Undefined();
    break;
  case ClearTimeout:
  case ClearInterval:
    result = Undefined();
    (const_cast<Window*>(window))->clearTimeout(v.toInt32());
    break;
  case Focus:
    result = Undefined();
    // TODO
    break;
  case Blur:
    result = Undefined();
    // TODO
    break;
  case Close:
#ifdef __ATHEOS__
      printf( "Warning: WindowFunc::tryExecute() Close not implemented\n" );
#else      
      /* From http://developer.netscape.com/docs/manuals/js/client/jsref/window.htm :
        The close method closes only windows opened by JavaScript using the open method.
        If you attempt to close any other window, a confirm is generated, which
        lets the user choose whether the window closes.
        This is a security feature to prevent "mail bombs" containing self.close().
        However, if the window has only one document (the current one) in its
        session history, the close is allowed without any confirm. This is a
        special case for one-off windows that need to open other windows and
        then dispose of themselves.
        */
    if (window->opener.isNull())
    {
        // To conform to the SPEC, we should only ask if the window
        // has more than one entry in the history (NS does that too). Bah.
        if ( KMessageBox::questionYesNo( window->part->widget(), i18n("Close window ?"), i18n("Confirmation required") ) == KMessageBox::Yes )
            (const_cast<Window*>(window))->scheduleClose();
    }
    else
    {
        (const_cast<Window*>(window))->scheduleClose();
    }

#endif // __ATHEOS__
    result = Undefined();
    break;
  }
  return Completion(Normal, result);

}

void WindowFunc::initJScript(KHTMLPart *p)
{
  (void)p->jScript(); // dummy call to create an interpreter
}

WindowQObject::WindowQObject(Window *w)
  : parent(w), timer(0L)
{
    connect( parent->part, SIGNAL( destroyed() ),
             this, SLOT( parentDestroyed() ) );
}

WindowQObject::~WindowQObject()
{
  delete timer;
}

void WindowQObject::parentDestroyed()
{
  delete timer;
  timer = 0;
}

int WindowQObject::installTimeout(const UString &handler, int t, bool singleShot)
{
  /* TODO: multiple timers */
  if (!timer) {
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), SLOT(timeout()));
  }
  timeoutHandler = handler;
  timer->start(t, singleShot);

  return 0;
}

void WindowQObject::clearTimeout(int /* timerId */)
{
  /* TODO: delete on id basis */
  if (timer) {
    timer->stop();
    timeoutHandler = "";
  }
}

void WindowQObject::timeout()
{
  if (!parent->part.isNull())
    parent->part->executeScript(timeoutHandler.qstring());
}

void WindowQObject::timeoutClose()
{
  if (!parent->part.isNull())
  {
    kdDebug(6070) << "WindowQObject::timeoutClose -> closing window" << endl;
    delete parent->part;
  }
}

KJSO FrameArray::get(const UString &p) const
{
  QList<KParts::ReadOnlyPart> frames = part->frames();
  int len = frames.count();
  if (p == "length")
    return Number(len);

  // check for the name or number
  KParts::ReadOnlyPart *frame = part->findFrame(p.qstring());
  if (!frame) {
    int i = (int)p.toDouble();
    if (i >= 0 && i < len)
      frame = frames.at(i);
  }

  // we are potentially fetching a reference to a another Window object here.
  // i.e. we may be accessing objects from another interpreter instance.
  // Therefore we have to be a bit careful with memory managment.
  if ( dynamic_cast<KHTMLPart*>(frame) != NULL ) {
    const KHTMLPart *khtml = static_cast<const KHTMLPart*>(frame);
    return KJSO(newWindow(const_cast<KHTMLPart*>(khtml)));
  }

  return Undefined();
}

static QPtrDict<Location> *location_dict = 0L;

// private to ensure use of dictionary via retrieve()
Location::Location(KHTMLPart *p) : part(p) { }

Location::~Location()
{
  location_dict->remove(part);
  if (location_dict->isEmpty()) {
    delete location_dict;
    location_dict = 0L;
  }
}

Location* Location::retrieve(KHTMLPart *p)
{
  Location *loc;
  if (!location_dict)
    location_dict = new QPtrDict<Location>;
  else if ((loc = location_dict->find(p)) != 0L)
    return loc;
  loc = new Location(p);
  location_dict->insert(p, loc);
  return loc;
}

KJSO Location::get(const UString &p) const
{
  if (part.isNull())
    return Undefined();

  KURL url = part->url();
  QString str;

  if (p == "hash")
    str = url.ref();
  else if (p == "host") {
    str = url.host();
    if (url.port())
      str += ":" + QString::number(url.port());
  } else if (p == "hostname")
    str = url.host();
  else if (p == "href"){
      if (!url.hasPath())
          str = url.prettyURL()+"/";
      else
          str = url.prettyURL();
  }
  else if (p == "pathname")
    str = url.path().isEmpty() ? QString("/") : url.path();
  else if (p == "port")
    str = url.port() ? QString::number(url.port()) : QString::fromLatin1("");
  else if (p == "protocol")
    str = url.protocol()+":";
  else if (p == "search")
    str = url.query();
  else if (p == "[[==]]")
    return toString();
  else if (p == "replace")
    return Function(new LocationFunc(part, LocationFunc::Replace));
  else if (p == "reload")
    return Function(new LocationFunc(part, LocationFunc::Reload));
  else
    return HostImp::get(p);

  return String(str);
}

void Location::put(const UString &p, const KJSO &v)
{
  if (part.isNull())
    return;

  QString str = v.toString().value().qstring();

  KURL url = part->url();

  if (p == "href") url = KURL(url,str);
  else if (p == "hash") url.setRef(str);
  else if (p == "host") {
    // danimo: KURL doesn't have a way to
    // set hostname _and_ port at once, right?
    QString host = str.left(str.find(":"));
    QString port = str.mid(str.find(":")+1);
    url.setHost(host);
    url.setPort(port.toUInt());
  } else if (p == "hostname") url.setHost(str);
  else if (p == "pathname") url.setPath(str);
  else if (p == "port") url.setPort(str.toUInt());
  else if (p == "protocol")  url.setProtocol(str);
  else if (p == "search"){ /* TODO */}
  else {
    HostImp::put(p, v);
    return;
  }

  part->scheduleRedirection(0, url.url().prepend( "target://_self/#" ) );

}

KJSO Location::toPrimitive(Type) const
{
    return toString();
}

String Location::toString() const
{

 if (!part->url().hasPath())
        return String(part->url().prettyURL()+"/");
    else
        return String(part->url().prettyURL());
}

LocationFunc::LocationFunc(KHTMLPart *p, int i) : part(p), id(i) { };

Completion LocationFunc::tryExecute(const List &args)
{
  if (!part.isNull()) {
    switch (id) {
    case Replace:
      part->scheduleRedirection(0, args[0].toString().value().qstring().prepend( "target://_self/#" ) );
      break;
    case Reload:
      part->scheduleRedirection(0, part->url().url().prepend( "target://_self/#" ) );
      break;
    }
  }
  return Completion(Normal, Undefined());
}

////////////////////// History Object ////////////////////////

bool History::hasProperty(const UString &p, bool recursive) const
{
  if (p == "back" ||
      p == "forward" ||
      p == "go" ||
      p == "length")
      return true;

  return HostImp::hasProperty(p, recursive);
}

KJSO History::get(const UString &p) const
{
  if (p == "back")
    return Function(new HistoryFunc(this, HistoryFunc::Back));
  else if (p == "forward")
    return Function(new HistoryFunc(this, HistoryFunc::Forward));
  else if (p == "go")
    return Function(new HistoryFunc(this, HistoryFunc::Go));
  else if (p == "length")
    return Number(0);
  else
    return HostImp::get(p);
}

Completion HistoryFunc::tryExecute(const List &)
{
  switch (id) {
  case Back:
      // ###
  case Forward:
      // ###
  case Go:
    // ###
  default:
    kdDebug(6070) << "history.back/forward/go() not implemented yet." << endl;
    break;
  }

  return Completion(Normal, Undefined());
}

#include "kjs_window.moc"

