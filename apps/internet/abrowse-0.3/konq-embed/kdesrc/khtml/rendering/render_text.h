/*
 * This file is part of the DOM implementation for KDE.
 *
 * (C) 1999 Lars Knoll (knoll@kde.org)
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
#ifndef RENDERTEXT_H
#define RENDERTEXT_H

#include <qvector.h>
#include <qarray.h>

#include "dom_string.h"
#include "dom_stringimpl.h"
#include "render_object.h"

#include <assert.h>

class QPainter;
class QFontMetrics;

namespace khtml
{
    class RenderText;
    class RenderStyle;

class TextSlave
{
public:
    TextSlave(int x, int y, QChar *text, int len,
              int height, int baseline, int width,
              bool reversed = false, bool firstLine = false)
    {
        m_x = x;
        m_y = y;
        m_text = text;
        m_len = len;
        m_height = height;
        m_baseline = baseline;
        m_width = width;
        m_reversed = reversed;
        m_firstLine = firstLine;
    }
    ~TextSlave();
    void print( QPainter *p, int _tx, int _ty);
    void printDecoration( QPainter *p, int _tx, int _ty, int decoration);
    void printBoxDecorations(QPainter *p, RenderStyle* style, RenderText *parent, int _tx, int _ty, bool begin, bool end);
    void printSelection(QPainter *p, RenderStyle* style, int tx, int ty, int startPos, int endPos);

    bool checkPoint(int _x, int _y, int _tx, int _ty);

    // Return before, after (offset set to max), or inside the text, at @p offset
    FindSelectionResult checkSelectionPoint(int _x, int _y, int _tx, int _ty, QFontMetrics * fm, int & offset);

    /**
     * if this textslave was rendered @ref _ty pixels below the upper edge
     * of a view, would the @ref _y -coordinate be inside the vertical range
     * of this object's representation?
     */
    bool checkVerticalPoint(int _y, int _ty, int _h)
    { if((_ty + m_y > _y + _h) || (_ty + m_y + m_height < _y)) return false; return true; }

    QChar* m_text;
    int m_y;
    unsigned short m_len;
    unsigned short m_x;
    unsigned short m_height;
    unsigned short m_baseline;
    unsigned short m_width;

    // If m_reversed is true, this is right to left text.
    // In this case, m_text will point to a QChar array which holds the already
    // reversed string, and the slave has to delete this string by itself.
    // If false, m_text points into the render text's own QChar array,
    // and in this case it's allowed to do substractions between m_texts of
    // different slaves.
    bool m_reversed : 1;
    bool m_firstLine : 1;
private:
    // this is just for QVector::bsearch. Don't use it otherwise
    TextSlave(int _x, int _y)
    {
        m_x = _x;
        m_y = _y;
        m_reversed = false;
    };
    friend class RenderText;
};

class TextSlaveArray : public QVector<TextSlave> // ### change this to QArray for Qt 3.0
{
public:
    TextSlaveArray();

    TextSlave* first();

    int	  findFirstMatching( Item ) const;
    virtual int compareItems( Item, Item );
};

class RenderText : public RenderObject
{
public:
    RenderText(DOM::DOMStringImpl *_str);
    virtual ~RenderText();

    virtual const char *renderName() const { return "RenderText"; }

    virtual void setStyle(RenderStyle *style);

    virtual bool isRendered() const { return true; }

    virtual void print( QPainter *, int x, int y, int w, int h,
                        int tx, int ty);
    virtual void printObject( QPainter *, int x, int y, int w, int h,
                        int tx, int ty);

    void deleteSlaves();

    DOM::DOMString data() const { return str; }
    DOM::DOMStringImpl *string() const { return str; }

    virtual void layout() {/*assert(false);*/}

    bool checkPoint(int _x, int _y, int _tx, int _ty);

    // Return before, after (offset set to max), or inside the text, at @p offset
    virtual FindSelectionResult checkSelectionPoint( int _x, int _y, int _tx, int _ty, int & offset );

    virtual unsigned int length() const { return str->l; }
    // no need for this to be virtual, however length needs to be!
    inline QChar *text() const { return str->s; }
    virtual void position(int x, int y, int from, int len, int width, bool reverse, bool firstLine);

    unsigned int width(unsigned int from, unsigned int len, QFontMetrics *fm) const;
    virtual unsigned int width(unsigned int from, unsigned int len, bool firstLine = false) const;
    virtual short width() const;
    virtual int height() const;

    // from BiDiObject
    // height of the contents (without paddings, margins and borders)
    virtual int bidiHeight() const;

    // overrides
    virtual void calcMinMaxWidth();
    virtual short minWidth() const { return m_minWidth; }
    virtual short maxWidth() const { return m_maxWidth; }

    // returns the minimum x position of all slaves relative to the parent.
    // defaults to 0.
    int minXPos() const;

    virtual int xPos() const;
    virtual int yPos() const;

    virtual short baselineOffset() const;
    virtual short verticalPositionHint() const;

    bool firstLine() const { return hasFirstLine; }
    bool hasReturn() const { return m_hasReturn; }
    
    virtual const QFont &font();
    QFontMetrics metrics(bool firstLine = false) const;

    bool isFixedWidthFont() const;

    void setText(DOM::DOMStringImpl *text);

    virtual SelectionState selectionState() const {return m_selectionState;}
    virtual void setSelectionState(SelectionState s) {m_selectionState = s; }
    virtual void cursorPos(int offset, int &_x, int &_y, int &height);
    virtual bool absolutePosition(int &/*xPos*/, int &/*yPos*/, bool f = false);
    void posOfChar(int ch, int &x, int &y);

    virtual short marginLeft() const { return style()->marginLeft().minWidth(0); }
    virtual short marginRight() const { return style()->marginRight().minWidth(0); }

    virtual void repaint();
protected:
    TextSlave * findTextSlave( int offset, int &pos );
    TextSlaveArray m_lines;

    DOM::DOMStringImpl *str;

    int m_contentHeight;
    short m_minWidth;
    short m_maxWidth;

    SelectionState m_selectionState : 3 ;
    bool hasFirstLine : 1;
    bool m_hasReturn : 1;
private:
    QFontMetrics *fm;
    // the one for the first line
};


};
#endif
