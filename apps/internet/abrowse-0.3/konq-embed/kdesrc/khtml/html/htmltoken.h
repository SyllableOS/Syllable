/*
    This file is part of the KDE libraries

    Copyright (C) 1997 Martin Jones (mjones@kde.org)
              (C) 1997 Torben Weis (weis@kde.org)
              (C) 1998 Waldo Bastian (bastian@kde.org)
              (C) 1999 Lars Knoll (knoll@kde.org)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/
//----------------------------------------------------------------------------
//
// KDE HTML Widget -- Tokenizers
// $Id$

#ifndef HTMLTOKEN_H
#define HTMLTOKEN_H

#include <qstring.h>

#include "stringit.h"
#include "dom_stringimpl.h"

namespace DOM {
    class AttrImpl;
}

namespace khtml {
/**
 * @internal
 * represents one HTML Attribute
 */
class Attribute
{
    friend class DOM::AttrImpl;
public:
    Attribute() { id = 0, v = 0, n = 0; }
    ~Attribute()
    {
        if(v) v->deref();
        if(n) n->deref();
    }

    Attribute &operator = (const Attribute &other) {
	id = other.id;
	if(v == other.v && n == other.n) return *this;
	if(v) v->deref();
	v = other.v;
	if(v) v->ref();
	if(n) n->deref();
	n = other.n;
	if(n) n->ref();
	return *this;
    }

    void setValue(QChar *_s, int _l) {
	if(v) v->deref();
        if ( _l >= 0 )
        {
            v = new DOMStringImpl(_s, _l);
            v->ref();
        }
    }
    void setValue( DOMString s ) {
	if(v) v->deref();
	v = s.implementation();
	v->ref();
    }
    DOMString value() const { return v; }
    DOMStringImpl *val() const { return v; }

    DOMString name() const;
    void setName(const DOMString &name);

    unsigned char id;
protected:
    DOM::DOMStringImpl *n;
    DOM::DOMStringImpl *v;
};

/**
 * @internal
 * A list of Attributes
 */
class AttributeList
{

public:
    AttributeList();
    AttributeList( Attribute *list, unsigned int length);
    AttributeList( const AttributeList &other);
    virtual ~AttributeList();

    AttributeList &operator = (const AttributeList &other );

    ushort id(uint index) const;
    DOMString name(uint index) const;
    DOMString value(uint index) const;
    DOMString valueForId(uint id)
	{
	    int i = find(id);
	    if(i == -1) return 0;
	    return value(i);
	}
    // adds the attribute. If it exists, nothing happens
    void add(const Attribute &attr);

    unsigned int length() const;

    // returns -1 if not found
    int find(const DOMString &name) const;
    int find(ushort id) const;

    DOMString operator [] (const DOMString &name) const;
    Attribute *operator [] (uint index) const;

private:
    void remove(DOMString name);
    void remove(uint index);

protected:
    Attribute *_list;
    uint _len;
};


/**
 * @internal
 * represents one HTML tag. Consists of a numerical id, and the list
 * of attributes. Can also represent text. In this case the id = 0 and
 * text contains the text.
 */
class Token
{
public:
    Token() { id = 0; complexText = false; }
    // contructor for a text token
    Token(QChar *text, uint len);
    // tag without attributes
    Token(ushort id);
    // tag with attributes
    Token(ushort id, Attribute *lst, uint len);

    ushort id;
    AttributeList attrs;
    DOMString text;
    bool complexText;
};


// makes code in the parser nicer
inline bool operator==( const Attribute &a, const int &i )
{ return a.id == i; }

inline bool operator==( const Attribute &a, const QString &s )
{ return a.value() == s; }

};
#endif // HTMLTOKEN

