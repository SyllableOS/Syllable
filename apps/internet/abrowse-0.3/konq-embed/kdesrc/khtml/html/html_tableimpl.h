/*
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright (C) 1997 Martin Jones (mjones@kde.org)
 *           (C) 1997 Torben Weis (weis@kde.org)
 *           (C) 1998 Waldo Bastian (bastian@kde.org)
 *           (C) 1999 Lars Knoll (knoll@kde.org)
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
#ifndef HTML_TABLEIMPL_H
#define HTML_TABLEIMPL_H

#include "html_elementimpl.h"

namespace DOM {

class DOMString;
class HTMLTableElementImpl;
class HTMLTableSectionElementImpl;
class HTMLTableSectionElement;
class HTMLTableRowElementImpl;
class HTMLTableRowElement;
class HTMLTableCellElementImpl;
class HTMLTableCellElement;
class HTMLTableColElementImpl;
class HTMLTableColElement;
class HTMLTableCaptionElementImpl;
class HTMLTableCaptionElement;
class HTMLElement;
class HTMLCollection;

class HTMLTableElementImpl : public HTMLElementImpl
{
public:
    enum Rules {
	None    = 0x00,
	RGroups = 0x01,
	CGroups = 0x02,
	Groups  = 0x03,
	Rows    = 0x05,
	Cols    = 0x0a,
	All     = 0x0f
    };
    enum Frame {
	Void   = 0x00,
	Above  = 0x01,
	Below  = 0x02,
	Lhs    = 0x04,
	Rhs    = 0x08,
	Hsides = 0x03,
	Vsides = 0x0c,
	Box    = 0x0f
    };

    HTMLTableElementImpl(DocumentImpl *doc);
    ~HTMLTableElementImpl();

    virtual const DOMString nodeName() const;
    virtual ushort id() const;

    virtual tagStatus startTag() const { return TABLEStartTag; }
    virtual tagStatus endTag() const { return TABLEEndTag; }

    HTMLTableCaptionElementImpl *caption() const { return tCaption; }
    NodeImpl *setCaption( HTMLTableCaptionElementImpl * );

    HTMLTableSectionElementImpl *tHead() const { return head; }
    NodeImpl *setTHead( HTMLTableSectionElementImpl * );

    HTMLTableSectionElementImpl *tFoot() const { return foot; }
    NodeImpl *setTFoot( HTMLTableSectionElementImpl * );

    HTMLElementImpl *createTHead (  );
    void deleteTHead (  );
    HTMLElementImpl *createTFoot (  );
    void deleteTFoot (  );
    HTMLElementImpl *createCaption (  );
    void deleteCaption (  );
    HTMLElementImpl *insertRow ( long index );
    void deleteRow ( long index );

    // overrides
    virtual NodeImpl *addChild(NodeImpl *child);
    virtual void parseAttribute(AttrImpl *attr);

    virtual void attach(KHTMLView *);

protected:
    HTMLTableSectionElementImpl *head;
    HTMLTableSectionElementImpl *foot;
    HTMLTableSectionElementImpl *firstBody;
    HTMLTableCaptionElementImpl *tCaption;

    Frame frame;
    Rules rules;

    bool incremental : 1;
    bool m_noBorder  : 1;
    friend class HTMLTableCellElementImpl;
};

// -------------------------------------------------------------------------

class HTMLTablePartElementImpl : public HTMLElementImpl

{
public:
    HTMLTablePartElementImpl(DocumentImpl *doc)
	: HTMLElementImpl(doc)
	{ }

    virtual void parseAttribute(AttrImpl *attr);

    void attach(KHTMLView *);
};

// -------------------------------------------------------------------------

class HTMLTableSectionElementImpl : public HTMLTablePartElementImpl
{
public:
    HTMLTableSectionElementImpl(DocumentImpl *doc, ushort tagid);

    ~HTMLTableSectionElementImpl();

    virtual const DOMString nodeName() const;
    virtual ushort id() const;

    virtual tagStatus startTag() const { return THEADStartTag; }
    virtual tagStatus endTag() const { return THEADEndTag; }

    HTMLElementImpl *insertRow ( long index );
    void deleteRow ( long index );

    int numRows() const { return nrows; }

protected:
    ushort _id;
    int nrows;
};

// -------------------------------------------------------------------------

class HTMLTableRowElementImpl : public HTMLTablePartElementImpl
{
public:
    HTMLTableRowElementImpl(DocumentImpl *doc);

    ~HTMLTableRowElementImpl();

    virtual const DOMString nodeName() const;
    virtual ushort id() const;

    virtual tagStatus startTag() const { return TRStartTag; }
    virtual tagStatus endTag() const { return TREndTag; }

    long rowIndex() const;
    long sectionRowIndex() const;

    HTMLElementImpl *insertCell ( long index );
    void deleteCell ( long index );

    // overrides
    virtual void parseAttribute(AttrImpl *attr);

protected:
    int ncols;
};

// -------------------------------------------------------------------------

class HTMLTableCellElementImpl : public HTMLTablePartElementImpl
{
public:
    HTMLTableCellElementImpl(DocumentImpl *doc, int tagId);

    ~HTMLTableCellElementImpl();

    virtual const DOMString nodeName() const;
    virtual ushort id() const { return _id; }

    virtual tagStatus startTag() const { return TDStartTag; }
    virtual tagStatus endTag() const { return TDEndTag; }

    // ### FIX these two...
    long cellIndex() const { return 0; }

    int col() const { return _col; }
    void setCol(int col) { _col = col; }
    int row() const { return _row; }
    void setRow(int r) { _row = r; }

    // overrides
    virtual void parseAttribute(AttrImpl *attr);
    virtual void attach(KHTMLView *);

protected:
    int _row;
    int _col;
    int rSpan;
    int cSpan;
    bool nWrap;
    int _id;
    int rowHeight;
};

// -------------------------------------------------------------------------

class HTMLTableColElementImpl : public HTMLElementImpl
{
public:
    HTMLTableColElementImpl(DocumentImpl *doc, ushort i);

    ~HTMLTableColElementImpl();

    virtual const DOMString nodeName() const;
    virtual ushort id() const;

    virtual tagStatus startTag() const { return COLStartTag; }
    virtual tagStatus endTag() const { return COLEndTag; }

    void setTable(HTMLTableElementImpl *t) { table = t; }

    virtual NodeImpl *addChild(NodeImpl *child);

    // overrides
    virtual void parseAttribute(AttrImpl *attr);

protected:
    // could be ID_COL or ID_COLGROUP ... The DOM is not quite clear on
    // this, but since both elements work quite similar, we use one
    // DOMElement for them...
    ushort _id;
    int _span;
    HTMLTableElementImpl *table;
};

// -------------------------------------------------------------------------

class HTMLTableCaptionElementImpl : public HTMLTablePartElementImpl
{
public:
    HTMLTableCaptionElementImpl(DocumentImpl *doc);

    ~HTMLTableCaptionElementImpl();

    virtual const DOMString nodeName() const;
    virtual ushort id() const;

    virtual tagStatus startTag() const { return CAPTIONStartTag; }
    virtual tagStatus endTag() const { return CAPTIONEndTag; }

    virtual void parseAttribute(AttrImpl *attr);
};

}; //namespace

#endif

