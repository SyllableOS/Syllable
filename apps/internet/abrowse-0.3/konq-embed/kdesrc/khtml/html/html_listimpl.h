/**
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
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
#ifndef HTML_LISTIMPL_H
#define HTML_LISTIMPL_H

/*
 * we ignore the deprecated compact attribute. Netscape does so too...
 */

#include "html_elementimpl.h"

namespace DOM
{

class DOMString;

class HTMLUListElementImpl : public HTMLElementImpl
{
public:
    HTMLUListElementImpl(DocumentImpl *doc) : HTMLElementImpl(doc) {}
    virtual ~HTMLUListElementImpl() {}

    virtual const DOMString nodeName() const;
    virtual ushort id() const;

    virtual tagStatus startTag() const { return ULStartTag; }
    virtual tagStatus endTag() const { return ULEndTag; }

    virtual void parseAttribute(AttrImpl *);

    virtual void attach(KHTMLView *);
    virtual int start() const { return 1; }
};

// -------------------------------------------------------------------------

class HTMLDirectoryElementImpl : public HTMLElementImpl
{
public:
    HTMLDirectoryElementImpl(DocumentImpl *doc) : HTMLElementImpl(doc) {}
    virtual ~HTMLDirectoryElementImpl() {}

    virtual const DOMString nodeName() const;
    virtual ushort id() const;

    virtual tagStatus startTag() const { return DIRStartTag; }
    virtual tagStatus endTag() const { return DIREndTag; }

    virtual void attach(KHTMLView *);
};

// -------------------------------------------------------------------------

class HTMLMenuElementImpl : public HTMLElementImpl
{
public:
    HTMLMenuElementImpl(DocumentImpl *doc) : HTMLElementImpl(doc) {}
    virtual ~HTMLMenuElementImpl() {}

    virtual const DOMString nodeName() const;
    virtual ushort id() const;

    virtual tagStatus startTag() const { return MENUStartTag; }
    virtual tagStatus endTag() const { return MENUEndTag; }

    virtual void attach(KHTMLView *);
};

// -------------------------------------------------------------------------

class HTMLOListElementImpl : public HTMLUListElementImpl
{
public:
    HTMLOListElementImpl(DocumentImpl *doc) : HTMLUListElementImpl(doc) {}
    virtual ~HTMLOListElementImpl() {}

    virtual const DOMString nodeName() const;
    virtual ushort id() const;

    virtual tagStatus startTag() const { return OLStartTag; }
    virtual tagStatus endTag() const { return OLEndTag; }

    virtual void parseAttribute(AttrImpl *);

    virtual void attach(KHTMLView *);

    int start() const { return _start; }
private:
    int _start;
};

// -------------------------------------------------------------------------

class HTMLLIElementImpl : public HTMLElementImpl
{
public:
    HTMLLIElementImpl(DocumentImpl *doc) : HTMLElementImpl(doc) {}
    virtual ~HTMLLIElementImpl() {}

    virtual const DOMString nodeName() const;
    virtual ushort id() const;

    virtual tagStatus startTag() const { return LIStartTag; }
    virtual tagStatus endTag() const { return LIEndTag; }

    virtual void parseAttribute(AttrImpl *attr);
};

// -------------------------------------------------------------------------

class HTMLDListElementImpl : public HTMLElementImpl
{
public:
    HTMLDListElementImpl(DocumentImpl *doc) : HTMLElementImpl(doc) {}
    virtual ~HTMLDListElementImpl() {}

    virtual const DOMString nodeName() const;
    virtual ushort id() const;

    virtual tagStatus startTag() const { return DLStartTag; }
    virtual tagStatus endTag() const { return DLEndTag; }
};

}; //namespace

#endif
