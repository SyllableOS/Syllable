/**
 * This file is part of the DOM implementation for KDE.
 *
 * (C) 1999 Lars Knoll (knoll@kde.org)
 * (C) 2000 Gunnstein Lye (gunnstein@netcom.no)
 * (C) 2000 Frederik Holljen (frederik.holljen@hig.no)
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

#ifndef _DOM2_RangeImpl_h_
#define _DOM2_RangeImpl_h_

#include "dom2_range.h"

namespace DOM {

class RangeImpl : public DomShared
{
    friend class DocumentImpl;
public:
    RangeImpl();
    RangeImpl(DocumentImpl *rootContainer);
    RangeImpl(const Document rootContainer);
    RangeImpl(const RangeImpl &other);
    RangeImpl(const Node sc, const long so, const Node ec, const long eo);

    RangeImpl & operator = (const RangeImpl &other);

    ~RangeImpl();

    // ### remove the get from these methods (i.e. getStartContainer() -> startContainer())
    Node getStartContainer() const;
    long getStartOffset() const;
    Node getEndContainer() const;
    long getEndOffset() const;
    bool getCollapsed() const;
    Node getCommonAncestorContainer();
    void setStart ( const Node &refNode, long offset );
    void setEnd ( const Node &refNode, long offset );
    void collapse ( bool toStart );
    short compareBoundaryPoints ( Range::CompareHow how, const Range &sourceRange );
    short compareBoundaryPoints ( Node containerA, long offsetA, Node containerB, long offsetB );
    bool boundaryPointsValid (  );
    void deleteContents (  );
    DocumentFragment extractContents (  );
    DocumentFragment cloneContents (  );
    void insertNode ( const Node &newNode );
    DOMString toString (  );
    DOMString toHTML (  );
    void detach (  );
    bool isDetached() const;
    DocumentFragment masterTraverse(bool contentExtract);

protected:
    Document ownerDocument;
    Node startContainer;
    unsigned long startOffset;
    Node endContainer;
    unsigned long endOffset;
    Node commonAncestorContainer;
    bool collapsed;
    bool detached;
};

}; // namespace

#endif

