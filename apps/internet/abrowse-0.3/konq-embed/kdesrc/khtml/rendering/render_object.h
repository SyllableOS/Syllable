/**
 * This file is part of the html renderer for KDE.
 *
 * Copyright (C) 2000 Lars Knoll (knoll@kde.org)
 *           (C) 2000 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Dirk Mueller (mueller@kde.org)
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
#ifndef render_object_h
#define render_object_h

#include <qcolor.h>
#include <qsize.h>
#include <qrect.h>

#include "xml/dom_nodeimpl.h"
#include "misc/khtmllayout.h"
#include "misc/loader_client.h"

#include "render_style.h"

class QPainter;
class CSSStyle;
class KHTMLView;

namespace DOM {
    class DOMString;
    class NodeImpl;
};

namespace khtml {

    class RenderStyle;
    class RenderTable;
    class CachedObject;
    class RenderRoot;

/**
 * Base Class for all rendering tree objects.
 */
class RenderObject : public CachedObjectClient
{
public:

    RenderObject();
    virtual ~RenderObject();

    RenderObject *parent() const { return m_parent; }

    RenderObject *previousSibling() const { return m_previous; }
    RenderObject *nextSibling() const { return m_next; }

    RenderObject *firstChild() const { return m_first; }
    RenderObject *lastChild() const { return m_last; }

    // RenderObject tree manipulation
    //////////////////////////////////////////
    virtual void addChild(RenderObject *newChild, RenderObject *beforeChild = 0);
    virtual void removeChild(RenderObject *oldChild);

    // raw tree manipulation
    virtual RenderObject* removeChildNode(RenderObject* child);
    void appendChildNode(RenderObject* child);
    void insertChildNode(RenderObject* child, RenderObject* before);
    //////////////////////////////////////////

private:
    //////////////////////////////////////////
    // Helper functions. Dangerous to use!
    void setPreviousSibling(RenderObject *previous) { m_previous = previous; }
    void setNextSibling(RenderObject *next) { m_next = next; }
    void setParent(RenderObject *parent) { m_parent = parent; }
    void setFirstChild(RenderObject *first) { m_first = first; }
    void setLastChild(RenderObject *last) { m_last = last; }
    //////////////////////////////////////////

public:
    virtual const char *renderName() const { return "RenderObject"; }
    virtual void printTree(int indent=0) const;

    static RenderObject *createObject(DOM::NodeImpl *node);

    // some helper functions...
    /**
     * is an Element that should be floated in the textstream
     */
    virtual bool childrenInline() const { return false; }
    virtual bool isRendered() const { return false; }
    virtual bool isFlow() const { return false; }

    virtual bool isListItem() const { return false; }
    virtual bool isRoot() const { return false; }
    virtual bool isBR() const { return false; }
    virtual bool isHtml() const { return false; }
    virtual bool isTableCell() const { return false; }
    virtual bool isTableRow() const { return false; }
    virtual bool isTableSection() const { return false; }
    virtual bool isTable() const { return false; }
    virtual bool isWidget() const { return false; }
    virtual bool isBody() const { return false; }

    /**
     * a block box that holds inline content or vice versa.
     * All inline children of a block level element box become anonymous
     * boxes as soon as they have any block level siblings.
     */
    bool isAnonymousBox() const { return m_isAnonymous; }
    void setIsAnonymousBox(bool b) { m_isAnonymous = b; }

    bool isFloating() const { return m_floating; }
    bool isPositioned() const { return m_positioned; } // absolute or fixed positioning
    bool isRelPositioned() const { return m_relPositioned; } // relative positioning
    bool isText() const  { return m_isText; }   // inherits RenderText
    bool isInline() const { return m_inline; }  // inline object
    bool isReplaced() const { return m_replaced; } // a "replaced" element (see CSS)
    bool hasSpecialObjects() const { return m_printSpecial; }
    bool isVisible() const  { return m_visible; }
    bool layouted() const   { return m_layouted; }
    bool parsing() const    { return m_parsing;     }
    bool minMaxKnown() const{ return m_minMaxKnown; }
    bool containsPositioned() const { return m_containsPositioned; }
    bool containsWidget() const { return m_containsWidget; }

    RenderRoot *root() const { return m_root; }

    /**
     * returns the object containing this one. can be different from parent for
     * positioned elements
     */
    RenderObject *container() const;

    void setContainsPositioned(bool p);
    void setLayouted(bool b=true) { m_layouted = b; }
    void setParsing(bool b=true) { m_parsing = b; }
    void setMinMaxKnown(bool b=true) { m_minMaxKnown = b; }
    void setPositioned(bool b=true)  { m_positioned = b;  }
    void setRelPositioned(bool b=true) { m_relPositioned = b; }
    void setFloating(bool b=true) { m_floating = b; }
    void setInline(bool b=true) { m_inline = b; }
    void setSpecialObjects(bool b=true) { m_printSpecial = b; }
    void setVisible(bool b=true) { m_visible = b; }
    void setRenderText() { m_isText = true; }
    void setReplaced(bool b=true) { m_replaced = b; }
    void setContainsWidget(bool b=true) { m_containsWidget = b; }

    virtual short baselineOffset() const { return 0; }
    virtual short verticalPositionHint() const { return 0; }

    /**
     * Print the object and it's children, but only if it fits in the
     * rectangle given by x,y,w,h. (tx|ty) is parents position.
     */
    virtual void print( QPainter *p, int x, int y, int w, int h, int tx, int ty);

    /**
     * assumes (_tx/_ty) point to the upper left corner of the object
     * prints only this object without calling print for the children.
     */
    virtual void printObject( QPainter */*p*/, int /*x*/, int /*y*/,
                        int /*w*/, int /*h*/, int /*tx*/, int /*ty*/) {}

    /**
     * prints the Box decoration borders
     */
    void printBorder(QPainter *p, int _tx, int _ty, int w, int h, const RenderStyle* style, bool begin=true, bool end=true);

    /**
     * prints the Box outline borders
     */
    void printOutline(QPainter *p, int _tx, int _ty, int w, int h, const RenderStyle* style);


    /**
     * This function calculates the minimum & maximum width that the object
     * can be set to.
     *
     * when the Element calls setMinMaxKnown(true), calcMinMaxWidth() will
     * be no longer called.
     *
     * when a element has a fixed size, m_minWidth and m_maxWidth should be
     * set to the same value. This has the special meaning that m_width,
     * contains the actual value.
     *
     * ### assumes calcMinMaxWidth has already been called for all children.
     */
    virtual void calcMinMaxWidth() { }

    /**
     * Calculates the actual width of the object (only for non inline
     * objects)
     */
    virtual void calcWidth() {}

    /**
     * This function should cause the Element to calculate its
     * width and height and the layout of it's content
     *
     * when the Element calls setLayouted(true), layout() is no
     * longer called during relayouts, as long as there is no
     * style sheet change. When that occurs, isLayouted will be
     * set to false and the Element receives layout() calls
     * again.
     */
    virtual void layout() = 0;

    /**
     * this function gets called when a child changed it's geometry
     * (because an image got loaded or some changes in the DOM...)
     */
    virtual void updateSize();

    /**
     * this function gets called when a child changed it's height
     * (because an image got loaded or some changes in the DOM...)
     */
    virtual void updateHeight() {}

    /**
     * This function gets called as soon as the parser leaves the element
     *
     */
    virtual void close() { setParsing(false); }

    /**
     * set the style of the object. This _has_ to be called after
     * the objects constructor to set the correct style. Also used for
     * dhtml to change the objects current style.
     *
     * If changing the style dynamically, you might need to call
     * updateSize() after applying the style change to force a
     * relayout/repaint
     */
    virtual void setStyle(RenderStyle *style);


    /**
     * returns the containing block level element for this element.
     * needed to compute margins and paddings
     *
     * objects with positioning set to absolute and fixed have to be added
     * to this objects rendering list
     *
     * function must not be called before the Element has been added
     * to the Renderingtree.
     */
    RenderObject *containingBlock() const;

    /**
     * return the size of the containing block.
     * Needed for layout calculations, see CSS2 specs, 10.1
     */
    virtual QSize containingBlockSize() const;

    /**
     * returns the width of the block the current Element is in. useful
     * for relative width/height calculations.
     *
     * must not be called before the Element has been added to the
     * Renderingtree.
     */
    virtual short containingBlockWidth() const;

    /**
     * returns the height of the block the current Element is in. useful
     * for relative width/height calculations.
     *
     * must not be called before the Element has been added to the
     * Renderingtree.
     */
    virtual int containingBlockHeight() const;

    /**
     * the original size of the content area
     * (see CSS2 visual formatting details)
     */
    virtual QSize contentSize() const;

    /**
     * the original width of the content area
     */
    virtual short contentWidth() const { return 0; }

    /**
     * the original height of the content area
     */
    virtual int contentHeight() const { return 0; }

    /**
     * A helper value for calculating the layout
     * extent of a replaced element. for images, it is the physical
     * width of the image.
     * Note that this value can differ from the layouted size as
     * retrieved by @ref contentWidth(), i.e. if the image is scaled.
     *
     * for inlined elements, the value is undefined, and 0 is returned.
     */
    virtual short intrinsicWidth() const { return 0; }

    /**
     * A helper value for calculating the layout
     * extent of a replaced element. for images, it is the physical
     * height of the image.
     * Note that this value can differ from the layouted size as
     * retrieved by @ref contentHeight(), i.e. if the image is scaled.
     *
     * for inlined elements, the value is undefined, and 0 is returned.
     */
    virtual int intrinsicHeight() const { return 0; }


    /**
     * the offset of the contents relative to the box borders
     * (basically border+padding)
     */
    virtual QSize contentOffset() const;
    // the size of the content + padding
    virtual QSize paddingSize() const;
    // the size of the box (including padding and border)
    virtual QSize size() const;


    // relative to parent node
    virtual void setPos( int /*xPos*/, int /*yPos*/ ) { }
    virtual void setSize( int /*width*/, int /*height*/ ) { }
    virtual void setWidth( int /*width*/ ) { }
    virtual void setHeight( int /*height*/ ) { }

    /**
     * Get the X-Position of this object relative to its parent
     */
    virtual int xPos() const { return 0; }

    /**
     * Get the Y-Position of this object relative to its parent
     */
    virtual int yPos() const { return 0; }

    virtual bool absolutePosition(int &/*xPos*/, int &/*yPos*/, bool fixed = false);

    // width and height are without margins but include paddings and borders
    virtual short width() const { return 0; }
    virtual int height() const { return 0; }

    // these are just for convinience
    virtual short marginTop() const { return 0; }
    virtual short marginBottom() const { return 0; }
    virtual short marginLeft() const { return 0; }
    virtual short marginRight() const { return 0; }

    int paddingTop() const;
    int paddingBottom() const;
    int paddingLeft() const;
    int paddingRight() const;

    int borderTop() const { return m_style->borderTopWidth(); }
    int borderBottom() const { return m_style->borderBottomWidth(); }
    int borderLeft() const { return m_style->borderLeftWidth(); }
    int borderRight() const { return m_style->borderRightWidth(); }

    virtual short minWidth() const { return 0; }
    virtual short maxWidth() const { return 0; }

    virtual RenderStyle* style() const { return m_style; }

    enum BorderSide {
        BSTop, BSBottom, BSLeft, BSRight
    };
    void drawBorder(QPainter *p, int x1, int y1, int x2, int y2, int width, BorderSide s,
                    QColor c, const QColor& textcolor, EBorderStyle style, bool sb1, bool sb2,
                    int adjbw1, int adjbw2, bool invalidisInvert = false);

    virtual void setTable(RenderTable*) {};

    /*
     * force a full repaint of the rendering tree
     */
    virtual void repaint() { if(m_parent) m_parent->repaint(); }
    virtual void repaintRectangle(int x, int y, int w, int h);

    virtual unsigned int length() const { return 1; }

    virtual bool isHidden() const { return isFloating() || isPositioned(); }
    /*
     * Special objects are objects that should be floated
     * but draw themselves (i.e. have content)
     */
    bool isSpecial() const { return (isFloating() || isPositioned()); };
    virtual bool containsSpecial() { return false; }
    virtual bool hasOverhangingFloats() { return false; }

    // for discussion of bidiHeight see CSS2 spec
    virtual int bidiHeight() const { return 0; }
    // helper function for layoutInlineChildren to position the
    // children RenderObjects
    virtual void position(int, int, int, int, int, bool, bool) {}

    enum SelectionState {
        SelectionNone,
        SelectionStart,
        SelectionInside,
        SelectionEnd,
        SelectionBoth
    };

    virtual SelectionState selectionState() const { return SelectionNone;}
    virtual void setSelectionState(SelectionState) {}

    virtual void cursorPos(int /*offset*/, int &/*_x*/, int &/*_y*/, int &/*height*/);

    /**
     * absolute lowest position (highest y-value) the object covers
     */
    virtual int lowestPosition() const {return 0;}
    /**
     * absolute rightmost position (highest x-value) the object covers
     */
    virtual int rightmostPosition() const {return 0;}

    CachedImage *backgroundImage() const { return m_bgImage; }

protected:
    virtual void selectionStartEnd(int& spos, int& epos);

    /**
     * helper function for RenderObject::print().
     * Draws the box decorations:
     * - DOC: (what box decorations are there?)
     * -
     * -
     * the coordinates are meant relative to the contents area.
     */
    virtual void printBoxDecorations(QPainter* /*p*/, int /*_x*/, int /*_y*/,
                                     int /*_w*/, int /*_h*/, int /*_tx*/, int /*_ty*/) {}

    virtual QRect viewRect() const;

private:
    RenderStyle *m_style;
    RenderObject *m_parent;
    RenderObject *m_previous;
    RenderObject *m_next;
    RenderObject *m_first;
    RenderObject *m_last;

    friend class RenderRoot;
    RenderRoot *m_root;

    CachedImage *m_bgImage;

    bool m_layouted       : 1;
    bool m_parsing        : 1;
    bool m_minMaxKnown    : 1;
    bool m_floating       : 1;

    bool m_positioned     : 1;
    bool m_containsPositioned     : 1;
    bool m_relPositioned  : 1;
    bool m_printSpecial   : 1; // if the box has something special to print (background, border, etc)
    bool m_isAnonymous    : 1;
    bool m_visible        : 1;
    bool m_isText         : 1;
    bool m_inline         : 1;
    bool m_replaced       : 1;
    bool m_containsWidget : 1;
    bool m_containsOverhangingFloats : 1;
};


    enum VerticalPositionHint {
        PositionTop = -1,
        PositionBottom = -2
    };



}; //namespace
#endif
