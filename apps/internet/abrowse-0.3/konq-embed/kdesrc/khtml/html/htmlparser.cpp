/*
    This file is part of the KDE libraries

   Copyright (C) 1997 Martin Jones (mjones@kde.org)
              (C) 1997 Torben Weis (weis@kde.org)
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
// KDE HTML Widget -- HTML Parser
//#define PARSER_DEBUG
//#define COMMENTS_IN_DOM

#include "htmlparser.h"

#include "dom_nodeimpl.h"
#include "dom_exception.h"
#include "html_baseimpl.h"
#include "html_blockimpl.h"
#include "html_documentimpl.h"
#include "html_elementimpl.h"
#include "html_formimpl.h"
#include "html_headimpl.h"
#include "html_imageimpl.h"
#include "html_inlineimpl.h"
#include "html_listimpl.h"
#include "html_miscimpl.h"
#include "html_tableimpl.h"
#include "html_objectimpl.h"
#include "dom_textimpl.h"

#include "htmlhashes.h"
#include "htmltoken.h"
#include "khtmlview.h"
#include "khtml_part.h"
#include "cssproperties.h"

#include "rendering/render_object.h"

#include <kdebug.h>
#include <klocale.h>

using namespace DOM;
using namespace khtml;

//----------------------------------------------------------------------------

// ### FIXME: move this list to dtd.cpp
//
// priority of tags. Closing tags of higher priority close tags of lower
// priority.
// Update this list, whenever you change htmltags.*
//
// 0 elements with forbidden close tag and text. They don't get pushed
//   to the stack.
// 1 inline elements
// 2 form elements
// 3 regular block level elements
// 4 lists (OL UL DIR MENU)
// 5 TD TH
// 6 TR
// 7 tbody thead tfoot caption  object
// 8 table
// 9 body frameset
// 10 html
const unsigned short tagPriority[] = {
    0, // 0
    1, // ID_A == 1
    1, // ID_ABBR
    1, // ID_ACRONYM
    3, // ID_ADDRESS
    1, // ID_APPLET
    0, // ID_AREA
    1, // ID_B
    0, // ID_BASE
    0, // ID_BASEFONT
    1, // ID_BDO
    1, // ID_BIG
    3, // ID_BLOCKQUOTE
    9, // ID_BODY
    0, // ID_BR
    1, // ID_BUTTON
    1, // ID_CAPTION
    3, // ID_CENTER
    1, // ID_CITE
    1, // ID_CODE
    0, // ID_COL
    1, // ID_COLGROUP
    3, // ID_DD
    1, // ID_DEL
    1, // ID_DFN
    4, // ID_DIR
    3, // ID_DIV
    4, // ID_DL
    3, // ID_DT
    1, // ID_EM
    0, // ID_EMBED
    3, // ID_FIELDSET
    1, // ID_FONT
    3, // ID_FORM
    0, // ID_FRAME
    9, // ID_FRAMESET
    3, // ID_H1
    3, // ID_H2
    3, // ID_H3
    3, // ID_H4
    3, // ID_H5
    3, // ID_H6
    9, // ID_HEAD
    0, // ID_HR
    10, // ID_HTML
    1, // ID_I
    1, // ID_IFRAME
    0, // ID_IMG
    0, // ID_INPUT
    1, // ID_INS
    0, // ID_ISINDEX
    1, // ID_KBD
    1, // ID_LABEL
    1, // ID_LAYER
    1, // ID_LEGEND
    3, // ID_LI
    0, // ID_LINK
    1, // ID_LISTING
    1, // ID_MAP
    4, // ID_MENU
    0, // ID_META
    9, // ID_NOEMBED
    9, // ID_NOFRAMES
    3, // ID_NOLAYER
    3, // ID_NOSCRIPT
    7, // ID_OBJECT
    4, // ID_OL
    1, // ID_OPTGROUP
    2, // ID_OPTION
    3, // ID_P
    0, // ID_PARAM
    1, // ID_PLAIN
    1, // ID_PRE
    1, // ID_Q
    1, // ID_S
    1, // ID_SAMP
    1, // ID_SCRIPT
    2, // ID_SELECT
    1, // ID_SMALL
    1, // ID_SPAN
    1, // ID_STRIKE
    1, // ID_STRONG
    1, // ID_STYLE
    1, // ID_SUB
    1, // ID_SUP
    8, // ID_TABLE
    7, // ID_TBODY
    5, // ID_TD
    1, // ID_TEXTAREA
    7, // ID_TFOOT
    5, // ID_TH
    7, // ID_THEAD
    1, // ID_TITLE
    6, // ID_TR
    1, // ID_TT
    1, // ID_U
    4, // ID_UL
    1, // ID_VAR
    0, // ID_TEXT
};


/**
 * @internal
 */
class HTMLStackElem
{
public:
    HTMLStackElem( int _id,
                   int _level,
                   DOM::NodeImpl *_node,
                   HTMLStackElem * _next
        )
        :
        id(_id),
        level(_level),
        node(_node),
        next(_next)
        { }

    int       id;
    int       level;
    NodeImpl *node;
    HTMLStackElem *next;
};

/**
 * @internal
 *
 * The parser parses tokenized input into the document, building up the
 * document tree. If the document is wellformed, parsing it is
 * straightforward.
 * Unfortunately, people can't write wellformed HTML documents, so the parser
 * has to be tolerant about errors.
 *
 * We have to take care of the following error conditions:
 * 1. The element being added is explicitly forbidden inside some outer tag.
 *    In this case we should close all tags up to the one, which forbids
 *    the element, and add it afterwards.
 * 2. We are not allowed to add the element directly. It could be, that
 *    the person writing the document forgot some tag inbetween (or that the
 *    tag inbetween is optional...) This could be the case with the following
 *    tags: HTML HEAD BODY TBODY TR TD LI (did I forget any?)
 * 3. We wan't to add a block element inside to an inline element. Close all
 *    inline elements up to the next higher block element.
 * 4. If this doesn't help close elements, until we are allowed to add the
 *    element or ignore the tag.
 *
 */
KHTMLParser::KHTMLParser( KHTMLView *_parent, HTMLDocumentImpl *doc)
{
    //kdDebug( 6035 ) << "parser constructor" << endl;
#ifdef SPEED_DEBUG
    qt.start();
#endif

    HTMLWidget    = _parent;
    document      = doc;

    blockStack = 0;

    // ID_CLOSE_TAG == Num of tags
    forbiddenTag = new ushort[ID_CLOSE_TAG+1];

    reset();
}

KHTMLParser::KHTMLParser( DOM::DocumentFragmentImpl *i, HTMLDocumentImpl *doc )
{
    HTMLWidget = 0;
    document = doc;
    forbiddenTag = new ushort[ID_CLOSE_TAG+1];

    blockStack = 0;

    reset();
    current = i;
    inBody = true;
}

KHTMLParser::~KHTMLParser()
{
#ifdef SPEED_DEBUG
    kdDebug( ) << "TIME: parsing time was = " << qt.elapsed() << endl;
#endif

    freeBlock();

    delete [] forbiddenTag;
    delete isindex;
}

void KHTMLParser::reset()
{
    current = document;

    freeBlock();

    // before parsing no tags are forbidden...
    memset(forbiddenTag, 0, (ID_CLOSE_TAG+1)*sizeof(ushort));

    inBody = false;
    noRealBody = true;
    haveFrameSet = false;
    _inline = false;

    form = 0;
    map = 0;
    head = 0;
    end = false;
    isindex = 0;
    flat = false;

    discard_until = 0;
}

void KHTMLParser::parseToken(Token *t)
{
    if (t->id > 2*ID_CLOSE_TAG)
    {
      kdDebug( 6035 ) << "Unknown tag!! tagID = " << t->id << endl;
      return;
    }
    if(discard_until)
    {
        if(t->id == discard_until)
            discard_until = 0;
        return;
    }

#ifdef PARSER_DEBUG
    kdDebug( 6035 ) << "\n\n==> parser: processing token " << t->id << " current = " << current->id() << endl;
    kdDebug(6035) << "inline=" << _inline << " inBody=" << inBody << endl;
#endif

    // holy shit. apparently some sites use </br> instead of <br>
    // be compatible with IE and NS
    if(document->parseMode() != DocumentImpl::Strict && t->id == ID_BR+ID_CLOSE_TAG)
        t->id -= ID_CLOSE_TAG;

    if(t->id > ID_CLOSE_TAG)
    {
        processCloseTag(t);
        return;
    }

    // ignore spaces, if we're not inside a paragraph or other inline code
    if( t->id == ID_TEXT ) {
#ifdef PARSER_DEBUG
	kdDebug(6035) << "length="<< t->text.length() << "text='" << t->text.string() << "'" << endl;
#endif
	if (!_inline  || !inBody || current->id() == ID_OPTION)  {
	    if(t->text.length() == 1 && t->text[0].latin1() == ' ')
		return;
	} else if ( inBody ) {
	    noRealBody = false;
	}
    }


    NodeImpl *n = getElement(t);
    // just to be sure, and to catch currently unimplemented stuff
    if(!n)
        return;

    // set attributes
    if(n->isElementNode())
    {
        ElementImpl *e = static_cast<ElementImpl *>(n);
        e->setAttribute(t->attrs);

        // take care of optional close tags
        if(e->endTag() == DOM::OPTIONAL)
            popBlock(t->id);
    }

    // if this tag is forbidden inside the current context, pop
    // blocks until we are allowed to add it...
    while(forbiddenTag[t->id]) popOneBlock();

    if ( !insertNode(n) ) {
        // we couldn't insert the node...
#ifdef PARSER_DEBUG
        kdDebug( 6035 ) << "insertNode failed current=" << current->id() << ", new=" << n->id() << "!" << endl;
#endif
        if (map == n)
        {
#ifdef PARSER_DEBUG
            kdDebug( 6035 ) << "  --> resetting map!" << endl;
#endif
            map = 0;
        }
        if (form == n)
        {
#ifdef PARSER_DEBUG
            kdDebug( 6035 ) << "   --> resetting form!" << endl;
#endif
            form = 0;
        }
        delete n;
    }
}

bool KHTMLParser::insertNode(NodeImpl *n)
{
    int id = n->id();

    // let's be stupid and just try to insert it.
    // this should work if the document is wellformed
#ifdef PARSER_DEBUG
    NodeImpl *tmp = current;
#endif
    NodeImpl *newNode = current->addChild(n);
    if ( newNode ) {
#ifdef PARSER_DEBUG
        kdDebug( 6035 ) << "added " << n->nodeName().string() << " to " << tmp->nodeName().string() << ", new current=" << newNode->nodeName().string() << endl;
#endif
        // don't push elements without end tag on the stack
        if(tagPriority[id] != 0 && !flat)
        {
            pushBlock(id, tagPriority[id]);
            current = newNode;
#ifndef SPEED_DEBUG
            if(!n->attached() && HTMLWidget )  n->attach(HTMLWidget);
#endif
            if(current->isInline()) _inline = true;
        }
        else {
#ifndef SPEED_DEBUG
            if(!n->attached() && HTMLWidget)  n->attach(HTMLWidget);
#endif
	    flat = false;
	}

#ifndef SPEED_DEBUG
        if(tagPriority[id] == 0 && n->renderer()) {
            n->renderer()->calcMinMaxWidth();
            if (n->id() == ID_EMBED) n->renderer()->close();
        }
#endif
        return true;
    } else {
#ifdef PARSER_DEBUG
        kdDebug( 6035 ) << "ADDING NODE FAILED!!!! current = " << current->nodeName().string() << ", new = " << n->nodeName().string() << endl;
#endif
        // error handling...
        HTMLElementImpl *e;
        bool handled = false;


        // things that appear in tables before <tr> or <td> must be
        // moved before the table. lets test this first...
        if(current->id() == ID_TABLE)
        {
            // scripts, styles, etc are handled later, for now...
            if(id!=ID_TD && id!=ID_TR && id!=ID_TH && id!=ID_SCRIPT &&
                    id!=ID_STYLE && id!=ID_MAP && id!=ID_FORM)
            {
                NodeImpl *node = current;
                NodeImpl *parent = node->parentNode();

                if (id == ID_TABLE) // <table><table> switches order, but
                                    // but also closes the first table. weird...
                {
                    popBlock(ID_TABLE);
                }

                int exceptioncode = 0;
                parent->insertBefore(n, node, exceptioncode );
                if ( exceptioncode ) {
#ifdef PARSER_DEBUG
                    kdDebug( 6035 ) << "adding element before table failed!!!!" << endl;
#endif
                    return false;
                }

                if(tagPriority[id] != 0 && id != ID_FORM && id != ID_INPUT ) {
                    pushBlock(id, tagPriority[id]);
                    current = n;
                }
#ifndef SPEED_DEBUG
                if(!n->attached() && HTMLWidget)  n->attach(HTMLWidget);
                if(tagPriority[id] == 0 && n->renderer())
                    n->renderer()->close();
#endif
                return true;
            }
        }

        // switch according to the element to insert
        switch(id)
        {
        case ID_COMMENT:
            break;
        case ID_HEAD:
            // ### alllow not having <HTML> in at all, as per HTML spec
            if (!current->isDocumentNode() && !current->id() == ID_HTML )
                return false;
            break;
            // We can deal with a base, meta and link element in the body, by just adding the element to head.
        case ID_META:
        case ID_LINK:
        case ID_BASE:
            if( !head )
                createHead();
            if( head ) {
                head->addChild(n);
#ifndef SPEED_DEBUG
                if(!n->attached() && HTMLWidget)
                    n->attach(HTMLWidget);
#endif
                return true;
            }
            break;
        case ID_HTML:
            if (!current->isDocumentNode())
                return false;
            break;
        case ID_TITLE:
        case ID_STYLE:
            if ( !head )
                createHead();
            if ( head ) {

                DOM::NodeImpl *newNode = head->addChild(n);
                if ( newNode ) {
                    pushBlock(id, tagPriority[id]);
                    current = newNode;
#ifndef SPEED_DEBUG
                    if(!n->attached() && HTMLWidget)
                        n->attach(HTMLWidget);
#endif
                } else {
#ifdef PARSER_DEBUG
                    kdDebug( 6035 ) << "adding style before to body failed!!!!" << endl;
#endif
                    discard_until = ID_STYLE + ID_CLOSE_TAG;
                    return false;
                }
                return true;
            } else if(inBody) {
                discard_until = ID_STYLE + ID_CLOSE_TAG;
                return false;
            }
            break;
            // SCRIPT and OBJECT are allowd in the body.
        case ID_BODY:
            if(inBody && document->body()) {
                // we have another <BODY> element.... apply attributes to existing one
                // make sure we don't overwrite already existing attributes
                // some sites use <body bgcolor=rightcolor>...<body bgcolor=wrongcolor>
                NamedAttrMapImpl *map = static_cast<NamedAttrMapImpl*>(n->attributes());
                NamedAttrMapImpl *bodymap = static_cast<NamedAttrMapImpl*>(document->body()->attributes());
                unsigned long attrNo;
                int exceptioncode;
                bool changed = false;
                for (attrNo = 0; map && attrNo < map->length(); attrNo++)
                    if(!bodymap->getNamedItem(static_cast<AttrImpl*>(map->item(attrNo))->name())) {
                        document->body()->setAttributeNode(static_cast<AttrImpl*>(map->item(attrNo)->cloneNode(false,exceptioncode)), exceptioncode);
                        changed = true;
                    }
                if ( changed )
                    document->body()->applyChanges();
		noRealBody = false;
            } else if ( current->isDocumentNode() )
                break;
            return false;
            break;
        case ID_LI:
            e = new HTMLUListElementImpl(document);
            e->addCSSProperty(CSS_PROP_MARGIN_LEFT, DOMString("0pt") );
            e->addCSSProperty(CSS_PROP_LIST_STYLE_POSITION, DOMString("inside") );
            // only try to insert <LI> when <UL> succeeds.
            // helps for <html><head><ul><li></html>
            if(insertNode(e)) {
                insertNode(n);
                return true;
            }
            break;

            // the following is a hack to move non rendered elements
            // outside of tables.
            // needed for broken constructs like <table><form ...><tr>....
        case ID_INPUT:
        {
            ElementImpl *e = static_cast<ElementImpl *>(n);
            DOMString type = e->getAttribute(ATTR_TYPE);

            if ( strcasecmp( type, "hidden" ) != 0 )
                break;
            // Fall through!
        }
        case ID_MAP:
        case ID_FORM:
        case ID_SCRIPT:
        {
#ifdef PARSER_DEBUG
            kdDebug( 6035 ) << "--> badly placed element!!!" << endl;
#endif
            NodeImpl *node = current;
            // in case the form is opened inside the table (<table><form ...><tr> or similar)
            // we need to move it outside the table.
            if(node->id() == ID_TR)
                node = node->parentNode();
            if(node->id() == ID_THEAD)
                node = node->parentNode();
            else if(node->id() == ID_TBODY)
                node = node->parentNode();
            else if(node->id() == ID_TFOOT)
                node = node->parentNode();

            if(node->id() == ID_TABLE)
            {
                NodeImpl *parent = node->parentNode();
                //kdDebug( 6035 ) << "trying to add form to " << parent->id() <<", " << parent->nodeName().string() <<  endl;
                int exceptioncode = 0;
                parent->insertBefore(n, node, exceptioncode );
                if ( exceptioncode ) {
#ifdef PARSER_DEBUG
                    kdDebug( 6035 ) << "adding form before of table failed!!!!" << endl;
#endif
                    return false;
                }
                if(tagPriority[id] != 0 && id != ID_FORM && id != ID_INPUT ) {
                    pushBlock(id, tagPriority[id]);
                    current = n;
                }
#ifndef SPEED_DEBUG
                if(!n->attached() && HTMLWidget)  n->attach(HTMLWidget);
                if(tagPriority[id] == 0 && n->renderer())
                    n->renderer()->close();
#endif
                return true;
            }
            break;
        }
        case ID_DD:
        case ID_DT:
            e = new HTMLDListElementImpl(document);
            insertNode(e);
            insertNode(n);
            return true;
            break;
        case ID_AREA:
        {
            if(map)
            {
                map->addChild(n);
#ifndef SPEED_DEBUG
                if(!n->attached() && HTMLWidget)  n->attach(HTMLWidget);
#endif
                handled = true;
            }
            else
                return false;
            return true;
        }
        case ID_TEXT:
            // ignore text inside the following elements.
            switch(current->id())
            {
            case ID_SELECT:
                return false;
            default:
                break;
           }
           break;
        default:
            break;
        }

        // switch on the currently active element
        switch(current->id())
        {
        case ID_HTML:
            switch(id)
            {
                // ### check, there's only one HTML and one BODY tag!
            case ID_SCRIPT:
            case ID_STYLE:
            case ID_META:
            case ID_LINK:
            case ID_OBJECT:
            case ID_EMBED:
            case ID_TITLE:
            case ID_ISINDEX:
            case ID_BASE:
                // ### what about <script> tags between head and body????
                if(!head) {
                    head = new HTMLHeadElementImpl(document);
                    e = head;
                    insertNode(e);
                    handled = true;
                }
                break;
            case ID_FRAME:
                if( haveFrameSet ) break;
                e = new HTMLFrameSetElementImpl(document);
                inBody = true;
                haveFrameSet = true;
                insertNode(e);
                handled = true;
                break;
            default:
                e = new HTMLBodyElementImpl(document);
                startBody();
                insertNode(e);
                document->createSelector();
                handled = true;
                break;
            }
            break;
        case ID_HEAD:
            // we can get here only if the element is not allowed in head.
            if (id == ID_HTML)
                return false;
            else {
                // This means the body starts here...
                popBlock(ID_HEAD);
                e = new HTMLBodyElementImpl(document);
                startBody();
                insertNode(e);
                document->createSelector();
                handled = true;
            }
            break;
        case ID_BODY:

            break;
        case ID_TABLE:
            switch(id)
            {
            case ID_COL:
            case ID_COLGROUP:
            case ID_P:
                break;
            case ID_TEXT:
            {
                TextImpl *t = static_cast<TextImpl *>(n);
                DOMStringImpl *i = t->string();
                unsigned int pos = 0;
                while(pos < i->l && ( *(i->s+pos) == QChar(' ') ||
                                      *(i->s+pos) == QChar(0xa0))) pos++;
                if(pos == i->l)
                {
                    break;
                }
            }
            case ID_COMMENT:
                break;
            // Fall through!
            default:
                e = new HTMLTableSectionElementImpl(document, ID_TBODY);
                insertNode(e);
                handled = true;
                break;
            }
            break;
        case ID_THEAD:
        case ID_TFOOT:
        case ID_TBODY:
            switch(id)
            {
            case ID_COMMENT:
                case ID_FONT:
            case ID_COL:
            case ID_COLGROUP:
            case ID_P:
                break;
            default:
                e = new HTMLTableRowElementImpl(document);
                insertNode(e);
                handled = true;
                break;
            }
            break;
        case ID_TR:
            switch(id)
            {
            case ID_COMMENT:
            case ID_FONT:
            case ID_COL:
            case ID_COLGROUP:
            case ID_P:
                break;
            case ID_TEXT:
            {
                TextImpl *t = static_cast<TextImpl *>(n);
                DOMStringImpl *i = t->string();
                unsigned int pos = 0;
                while(pos < i->l && ( *(i->s+pos) == QChar(' ') ||
                                      *(i->s+pos) == QChar(0xa0))) pos++;
                if(pos == i->l)
                {
                    break;
                }
            }
            // Fall through!
            default:
                e = new HTMLTableCellElementImpl(document, ID_TD);
                insertNode(e);
                handled = true;
                break;
            }
            break;
	    case ID_OBJECT:
		discard_until = id + ID_CLOSE_TAG;
		return false;
	    case ID_UL:
        case ID_OL:
        case ID_DIR:
        case ID_MENU:
            e = new HTMLLIElementImpl(document);
            e->addCSSProperty(CSS_PROP_LIST_STYLE_TYPE, DOMString("none") );
            insertNode(e);
            handled = true;
            break;
            case ID_DL:
                popBlock(ID_DL);
                handled = true;
                break;
            case ID_DT:
                popBlock(ID_DT);
                handled = true;
                break;
        case ID_SELECT:
            if( n->isInline() )
                return false;
            break;
        case ID_P:
        case ID_H1:
        case ID_H2:
        case ID_H3:
        case ID_H4:
        case ID_H5:
        case ID_H6:
            if(!n->isInline())
            {
                popBlock(current->id());
                handled = true;
            }
            break;
        case ID_OPTION:
            if (id == ID_OPTGROUP)
            {
                popBlock(ID_OPTION);
                handled = true;
            }
            else if(id == ID_SELECT)
            {
		// IE treats a nested select as </select>. Let's do the same
		popBlock( ID_SELECT );
                break;
            }
            break;
            // head elements in the body should be ignored.
        case ID_ADDRESS:
            popBlock(ID_ADDRESS);
            handled = true;
            break;
        case ID_COLGROUP:
            popBlock(ID_COLGROUP);
            handled = true;
            break;
        case ID_FONT:
            popBlock(ID_FONT);
            handled = true;
            break;
        default:
            if(current->isDocumentNode())
            {
                if(current->firstChild() == 0)
                {
                    e = new HTMLHtmlElementImpl(document);
                    insertNode(e);
                    handled = true;
                }
            }
            else if(current->isInline())
            {
                popInlineBlocks();
                handled = true;
            }
        }

        // if we couldn't handle the error, just rethrow the exception...
        if(!handled)
        {
            //kdDebug( 6035 ) << "Exception handler failed in HTMLPArser::insertNode()" << endl;
            return false;
        }

        return insertNode(n);
    }
}


NodeImpl *KHTMLParser::getElement(Token *t)
{
    NodeImpl *n = 0;

    switch(t->id)
    {
    case ID_HTML:
        n = new HTMLHtmlElementImpl(document);
        break;
    case ID_HEAD:
        if(!head && current->id() == ID_HTML) {
            head = new HTMLHeadElementImpl(document);
            n = head;
        }
        break;
    case ID_BODY:
        popBlock(ID_HEAD);
        n = new HTMLBodyElementImpl(document);
        startBody();
	noRealBody = false;
        break;

// head elements
    case ID_BASE:
        n = new HTMLBaseElementImpl(document);
        break;
    case ID_LINK:
        n = new HTMLLinkElementImpl(document);
        break;
    case ID_META:
        n = new HTMLMetaElementImpl(document);
        break;
    case ID_STYLE:
        n = new HTMLStyleElementImpl(document);
        break;
    case ID_TITLE:
        n = new HTMLTitleElementImpl(document);
        break;

// frames
    case ID_FRAME:
        n = new HTMLFrameElementImpl(document);
        break;
    case ID_FRAMESET:
        popBlock(ID_HEAD);
	if ( inBody && noRealBody )
	    removeBody();
        if ( haveFrameSet && current->id() == ID_HTML )
            break;
        n = new HTMLFrameSetElementImpl(document);
        haveFrameSet = true;
        startBody();
	noRealBody = false;
        break;
        // a bit a special case, since the frame is inlined...
    case ID_IFRAME:
        n = new HTMLIFrameElementImpl(document);
        break;

// form elements
    case ID_FORM:
        // close all open forms...
        popBlock(ID_FORM);
        form = new HTMLFormElementImpl(document);
        n = form;
        break;
    case ID_BUTTON:
        n = new HTMLButtonElementImpl(document, form);
        break;
    case ID_FIELDSET:
        n = new HTMLFieldSetElementImpl(document, form);
        break;
    case ID_INPUT:
        n = new HTMLInputElementImpl(document, form);
        break;
    case ID_ISINDEX:
	n = handleIsindex(t);
	if( !inBody ) {
	    isindex = n;
	    n = 0;
	} else
	    flat = true;
        //n = new HTMLIsIndexElementImpl(document, form);
        break;
    case ID_LABEL:
        n = new HTMLLabelElementImpl(document, form);
        break;
    case ID_LEGEND:
        n = new HTMLLegendElementImpl(document, form);
        break;
    case ID_OPTGROUP:
        n = new HTMLOptGroupElementImpl(document, form);
        break;
    case ID_OPTION:
        n = new HTMLOptionElementImpl(document, form);
        break;
    case ID_SELECT:
        n = new HTMLSelectElementImpl(document, form);
        break;
    case ID_TEXTAREA:
        n = new HTMLTextAreaElementImpl(document, form);
        break;

// lists
    case ID_DL:
        n = new HTMLDListElementImpl(document);
        break;
    case ID_DD:
        n = new HTMLGenericElementImpl(document, t->id);
        popBlock(ID_DT);
        popBlock(ID_DD);
        break;
    case ID_DT:
        n = new HTMLGenericElementImpl(document, t->id);
        popBlock(ID_DD);
        popBlock(ID_DT);
        break;
    case ID_UL:
    {
        n = new HTMLUListElementImpl(document);
        break;
    }
    case ID_OL:
    {
        n = new HTMLOListElementImpl(document);
        break;
    }
    case ID_DIR:
        n = new HTMLDirectoryElementImpl(document);
        break;
    case ID_MENU:
        n = new HTMLMenuElementImpl(document);
        break;
    case ID_LI:
    {
        popBlock(ID_LI);
        HTMLElementImpl *e = new HTMLLIElementImpl(document);
        n = e;
        if( current->id() != ID_UL && current->id() != ID_OL )
                e->addCSSProperty(CSS_PROP_LIST_STYLE_POSITION, DOMString("inside") );
        break;
    }
// formatting elements (block)
    case ID_BLOCKQUOTE:
        n = new HTMLBlockquoteElementImpl(document);
        break;
    case ID_DIV:
        n = new HTMLDivElementImpl(document);
        break;
    case ID_LAYER:
        n = new HTMLLayerElementImpl(document);
        break;
    case ID_H1:
    case ID_H2:
    case ID_H3:
    case ID_H4:
    case ID_H5:
    case ID_H6:
        n = new HTMLHeadingElementImpl(document, t->id);
        break;
    case ID_HR:
        n = new HTMLHRElementImpl(document);
        break;
    case ID_P:
        n = new HTMLParagraphElementImpl(document);
        break;
    case ID_PRE:
        n = new HTMLPreElementImpl(document);
        break;

// font stuff
    case ID_BASEFONT:
        n = new HTMLBaseFontElementImpl(document);
        break;
    case ID_FONT:
        n = new HTMLFontElementImpl(document);
        break;

// ins/del
    case ID_DEL:
    case ID_INS:
        n = new HTMLModElementImpl(document, t->id);
        break;

// anchor
    case ID_A:
        n = new HTMLAnchorElementImpl(document);
        break;

// images
    case ID_IMG:
        n = new HTMLImageElementImpl(document);
        break;
    case ID_MAP:
        map = new HTMLMapElementImpl(document);
        n = map;
        break;
    case ID_AREA:
        n = new HTMLAreaElementImpl(document);
        break;

// objects, applets and scripts
    case ID_APPLET:
        n = new HTMLAppletElementImpl(document);
        break;
    case ID_EMBED:
        n = new HTMLEmbedElementImpl(document);
        break;
    case ID_OBJECT:
        n = new HTMLObjectElementImpl(document);
        break;
    case ID_PARAM:
        n = new HTMLParamElementImpl(document);
        break;
    case ID_SCRIPT:
        n = new HTMLScriptElementImpl(document);
        break;

// tables
    case ID_TABLE:
        n = new HTMLTableElementImpl(document);
        break;
    case ID_CAPTION:
        n = new HTMLTableCaptionElementImpl(document);
        break;
    case ID_COLGROUP:
    case ID_COL:
        n = new HTMLTableColElementImpl(document, t->id);
        break;
    case ID_TR:
        popBlock(ID_TR);
        n = new HTMLTableRowElementImpl(document);
        break;
    case ID_TD:
    case ID_TH:
        popBlock(ID_TH);
        popBlock(ID_TD);
        n = new HTMLTableCellElementImpl(document, t->id);
        break;
    case ID_THEAD:
    case ID_TBODY:
    case ID_TFOOT:
        n = new HTMLTableSectionElementImpl(document, t->id);
        break;

// inline elements
    case ID_BR:
        n = new HTMLBRElementImpl(document);
        break;
    case ID_Q:
        n = new HTMLQuoteElementImpl(document);
        break;

// elements with no special representation in the DOM

// block:
    case ID_ADDRESS:
    case ID_CENTER:
        case ID_LISTING:
        n = new HTMLGenericElementImpl(document, t->id);
        break;
// inline
        // %fontstyle
    case ID_TT:
    case ID_U:
    case ID_B:
    case ID_I:
    case ID_S:
    case ID_STRIKE:
    case ID_BIG:
    case ID_SMALL:

        // %phrase
    case ID_EM:
    case ID_STRONG:
    case ID_DFN:
    case ID_CODE:
    case ID_SAMP:
    case ID_KBD:
    case ID_VAR:
    case ID_CITE:
    case ID_ABBR:
    case ID_ACRONYM:

        // %special
    case ID_SUB:
    case ID_SUP:
    case ID_SPAN:
        n = new HTMLGenericElementImpl(document, t->id);
        break;

    case ID_BDO:
        break;

        // these are special, and normally not rendered
    case ID_NOEMBED:
        discard_until = ID_NOEMBED + ID_CLOSE_TAG;
        return 0;
    case ID_NOFRAMES:
        discard_until = ID_NOFRAMES + ID_CLOSE_TAG;
        return 0;
    case ID_NOSCRIPT:
        if(HTMLWidget && HTMLWidget->part()->jScriptEnabled())
            discard_until = ID_NOSCRIPT + ID_CLOSE_TAG;
        return 0;
    case ID_NOLAYER:
        discard_until = ID_NOLAYER + ID_CLOSE_TAG;
        return 0;
        // Waldo's plaintext stuff
    case ID_PLAIN:
        return 0;
        break;
// text
    case ID_TEXT:
        n = new TextImpl(document, t->text);
        if (t->complexText )
            n->setComplexText(true);
        break;
    case ID_COMMENT:
#ifdef COMMENTS_IN_DOM
        n = new CommentImpl(document, t->text);
#endif
        break;
    default:
        kdDebug( 6035 ) << "Unknown tag " << t->id << "!" << endl;
    }
    return n;
}

void KHTMLParser::processCloseTag(Token *t)
{
    // support for really broken html. Can't believe I'm supporting such crap (lars)
    switch(t->id)
    {
    case ID_HTML+ID_CLOSE_TAG:
    case ID_BODY+ID_CLOSE_TAG:
        // we never close the body tag, since some stupid web pages close it before the actual end of the doc.
        // let's rely on the end() call to close things.
        return;
    case ID_FORM+ID_CLOSE_TAG:
        form = 0;
        // this one is to get the right style on the body element
        break;
    case ID_MAP+ID_CLOSE_TAG:
        map = 0;
        break;
    case ID_HEAD+ID_CLOSE_TAG:
        //inBody = true;
        // don't close head neither. the creation of body will do it for us.
        // fixes some sites, that define stylesheets after </head>
        return;
    case ID_TITLE+ID_CLOSE_TAG:
        if ( current->id() == ID_TITLE )
          static_cast<HTMLTitleElementImpl *>(current)->setTitle();
        break;
    default:
        break;
    }

#ifdef PARSER_DEBUG
    kdDebug( 6035 ) << "added the following childs to " << current->nodeName().string() << endl;
    NodeImpl *child = current->firstChild();
    while(child != 0)
    {
        kdDebug( 6035 ) << "    " << child->nodeName().string() << endl;
        child = child->nextSibling();
    }
#endif
    popBlock(t->id-ID_CLOSE_TAG);
#ifdef PARSER_DEBUG
    kdDebug( 6035 ) << "closeTag --> current = " << current->nodeName().string() << endl;
#endif
}


void KHTMLParser::pushBlock(int _id, int _level)
{
    HTMLStackElem *Elem = new HTMLStackElem(_id, _level, current, blockStack);

    blockStack = Elem;
    addForbidden(_id, forbiddenTag);
}

void KHTMLParser::popBlock( int _id )
{
    HTMLStackElem *Elem = blockStack;
    int maxLevel = 0;

    while( Elem && (Elem->id != _id))
    {
        if (maxLevel < Elem->level)
        {
            maxLevel = Elem->level;
        }
        Elem = Elem->next;
    }
    if (!Elem || maxLevel > Elem->level)
        return;

    Elem = blockStack;

    while (Elem)
    {
        if (Elem->id == _id)
        {
            popOneBlock();
            Elem = 0;
        }
        else
        {
            popOneBlock();
            Elem = blockStack;
        }
    }
}

void KHTMLParser::popOneBlock()
{
    HTMLStackElem *Elem = blockStack;

    // we should never get here, but some bad html might cause it.
#ifndef PARSER_DEBUG
    if(!Elem) return;
#else
    kdDebug( 6035 ) << "popping block: " << Elem->id << endl;
#endif

#ifndef SPEED_DEBUG
    if(Elem->node != current)
        if(current->renderer()) current->renderer()->close();
#endif

    removeForbidden(Elem->id, forbiddenTag);

    blockStack = Elem->next;
    // we only set inline to false, if the element we close is a block level element.
    // This helps getting cases as <p><b>bla</b> <b>bla</b> right.
    if(!current->isInline())
        _inline = false;
    current = Elem->node;

    delete Elem;
}

void KHTMLParser::popInlineBlocks()
{
    while(current->isInline() && current->id() != ID_FONT)
        popOneBlock();
}

void KHTMLParser::freeBlock()
{
    while (blockStack)
        popOneBlock();
    blockStack = 0;
}

void KHTMLParser::createHead()
{
    if(head || !document->firstChild())
        return;

    head = new HTMLHeadElementImpl(document);
    HTMLElementImpl *body = document->body();
    int exceptioncode = 0;
    document->firstChild()->insertBefore(head, body, exceptioncode);
    if ( exceptioncode ) {
#ifdef PARSER_DEBUG
        kdDebug( 6035 ) << "creation of head failed!!!!" << endl;
#endif
        delete head;
        head = 0;
    }
}

NodeImpl *KHTMLParser::handleIsindex( Token *t )
{
    NodeImpl *n;
    HTMLFormElementImpl *myform = form;
    if ( !myform ) {
	myform = new HTMLFormElementImpl(document);
	n = myform;
    } else
	n = new HTMLDivElementImpl( document );
    NodeImpl *child = new HTMLHRElementImpl( document );
    n->addChild( child );
    DOMString text = t->attrs.valueForId( ATTR_PROMPT );
    if( text.isNull() )
	text =  i18n("This is a searchable index. Enter search keywords: ");
    child = new TextImpl(document, text);
    n->addChild( child );
    child = new HTMLInputElementImpl(document, myform);
    static_cast<ElementImpl *>(child)->setAttribute(ATTR_TYPE, "khtml_isindex");
    n->addChild( child );
    child = new HTMLHRElementImpl( document );
    n->addChild( child );

    return n;
}

void KHTMLParser::startBody()
{
    if(inBody) return;

    document->createSelector();
    inBody = true;

    if( isindex ) {
	flat = true; // don't decend into this node
	insertNode( isindex );
	isindex = 0;
    }
}

void KHTMLParser::removeBody()
{
    popBlock( ID_BODY );
    document->setBody( 0 );
    inBody = false;
    noRealBody = true;
}
