/**
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
#include "dtd.h"
#include "htmlhashes.h"

using namespace DOM;

#include <kdebug.h>

static const ushort tag_list_0[] = {
    ID_TEXT,
    ID_TT,
    ID_I,
    ID_B,
    ID_U,
    ID_S,
    ID_STRIKE,
    ID_BIG,
    ID_SMALL,
    ID_EM,
    ID_STRONG,
    ID_DFN,
    ID_CODE,
    ID_SAMP,
    ID_KBD,
    ID_VAR,
    ID_CITE,
    ID_ABBR,
    ID_ACRONYM,
    ID_A,
    ID_IMG,
    ID_APPLET,
    ID_OBJECT,
    ID_EMBED,
    ID_FONT,
    ID_BASEFONT,
    ID_BR,
    ID_SCRIPT,
    ID_MAP,
    ID_Q,
    ID_SUB,
    ID_SUP,
    ID_SPAN,
    ID_BDO,
    ID_IFRAME,
    ID_INPUT,
    ID_SELECT,
    ID_TEXTAREA,
    ID_LABEL,
    ID_BUTTON,
    ID_COMMENT,
    0
};

static const ushort tag_list_1[] = {
    ID_P,
    ID_H1,
    ID_H2,
    ID_H3,
    ID_H4,
    ID_H5,
    ID_H6,
    ID_UL,
    ID_OL,
    ID_DIR,
    ID_MENU,
    ID_PRE,
    ID_DL,
    ID_DIV,
    ID_LAYER,
    ID_CENTER,
    ID_NOSCRIPT,
    ID_NOFRAMES,
    ID_BLOCKQUOTE,
    ID_FORM,
    ID_ISINDEX,
    ID_HR,
    ID_TABLE,
    ID_FIELDSET,
    ID_ADDRESS,
    ID_TEXT,
    ID_TT,
    ID_I,
    ID_B,
    ID_U,
    ID_S,
    ID_STRIKE,
    ID_BIG,
    ID_SMALL,
    ID_EM,
    ID_STRONG,
    ID_DFN,
    ID_CODE,
    ID_SAMP,
    ID_KBD,
    ID_VAR,
    ID_CITE,
    ID_ABBR,
    ID_ACRONYM,
    ID_A,
    ID_IMG,
    ID_APPLET,
    ID_OBJECT,
    ID_EMBED,
    ID_FONT,
    ID_BASEFONT,
    ID_BR,
    ID_SCRIPT,
    ID_MAP,
    ID_Q,
    ID_SUB,
    ID_SUP,
    ID_SPAN,
    ID_BDO,
    ID_IFRAME,
    ID_INPUT,
    ID_SELECT,
    ID_TEXTAREA,
    ID_LABEL,
    ID_BUTTON,
    ID_COMMENT,
    ID_LI,
    ID_LISTING,
    0
};

static const ushort tag_list_2[] = {
    ID_INS,
    ID_DEL,
    ID_COMMENT,
    0
};

static const ushort tag_list_3[] = {
    ID_P,
    ID_H1,
    ID_H2,
    ID_H3,
    ID_H4,
    ID_H5,
    ID_H6,
    ID_UL,
    ID_OL,
    ID_DIR,
    ID_MENU,
    ID_PRE,
    ID_DL,
    ID_DIV,
    ID_LAYER,
    ID_CENTER,
    ID_NOSCRIPT,
    ID_NOFRAMES,
    ID_BLOCKQUOTE,
    ID_FORM,
    ID_ISINDEX,
    ID_HR,
    ID_TABLE,
    ID_FIELDSET,
    ID_ADDRESS,
    ID_COMMENT,
    ID_LI,
    ID_LISTING,
    0
};

static const ushort tag_list_4[] = {
    ID_PARAM,
    ID_P,
    ID_H1,
    ID_H2,
    ID_H3,
    ID_H4,
    ID_H5,
    ID_H6,
    ID_UL,
    ID_OL,
    ID_DIR,
    ID_MENU,
    ID_PRE,
    ID_DL,
    ID_DIV,
    ID_LAYER,
    ID_CENTER,
    ID_NOSCRIPT,
    ID_NOFRAMES,
    ID_BLOCKQUOTE,
    ID_FORM,
    ID_ISINDEX,
    ID_HR,
    ID_TABLE,
    ID_FIELDSET,
    ID_ADDRESS,
    ID_TEXT,
    ID_TT,
    ID_I,
    ID_B,
    ID_U,
    ID_S,
    ID_STRIKE,
    ID_BIG,
    ID_SMALL,
    ID_EM,
    ID_STRONG,
    ID_DFN,
    ID_CODE,
    ID_SAMP,
    ID_KBD,
    ID_VAR,
    ID_CITE,
    ID_ABBR,
    ID_ACRONYM,
    ID_A,
    ID_IMG,
    ID_APPLET,
    ID_OBJECT,
    ID_EMBED,
    ID_FONT,
    ID_BASEFONT,
    ID_BR,
    ID_SCRIPT,
    ID_MAP,
    ID_Q,
    ID_SUB,
    ID_SUP,
    ID_SPAN,
    ID_BDO,
    ID_IFRAME,
    ID_INPUT,
    ID_SELECT,
    ID_TEXTAREA,
    ID_LABEL,
    ID_BUTTON,
    ID_COMMENT,
    ID_LI,
    ID_LISTING,
    0
};

static const ushort tag_list_6[] = {
    ID_DT,
    ID_DD,
    ID_COMMENT,
    0
};

static const ushort tag_list_7[] = {
    ID_OPTGROUP,
    ID_OPTION,
    ID_COMMENT,
    ID_SCRIPT,
    0
};

static const ushort tag_list_9[] = {
    ID_TH,
    ID_TD,
    ID_COMMENT,
    0
};

static const ushort tag_list_10[] = {
    ID_FRAMESET,
    ID_FRAME,
    ID_COMMENT,
    0
};

static const ushort tag_list_11[] = {
    ID_SCRIPT,
    ID_STYLE,
    ID_META,
    ID_LINK,
    ID_OBJECT,
    ID_EMBED,
    ID_COMMENT,
    0
};

bool check_array(ushort child, const ushort *tagList)
{
    int i = 0;
    while(tagList[i] != 0)
    {
        if(tagList[i] == child) return true;
    i++;
    }
    return false;
}


bool DOM::checkChild(ushort tagID, ushort childID)
{
    //kdDebug( 6030 ) << "checkChild: " << tagID << "/" << childID << endl;

    // ### allow comments inside ANY node that can contain children

    if (tagID >= 1000 || childID >= 1000)
	return true; // one or both of the elements in an XML element; just allow for now

    switch(tagID)
    {
    case ID_TT:
    case ID_I:
    case ID_B:
    case ID_U:
    case ID_S:
    case ID_STRIKE:
    case ID_BIG:
    case ID_SMALL:
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
    case ID_SUB:
    case ID_SUP:
    case ID_SPAN:
    case ID_BDO:
//    case ID_FONT:
    case ID_P:
    case ID_H1:
    case ID_H2:
    case ID_H3:
    case ID_H4:
    case ID_H5:
    case ID_H6:
    case ID_Q:
    case ID_LEGEND:
    case ID_CAPTION:
	// _0 *
	return check_array(childID, tag_list_0);
    case ID_BASEFONT:
    case ID_BR:
    case ID_AREA:
    case ID_LINK:
    case ID_IMG:
    case ID_PARAM:
    case ID_HR:
    case ID_INPUT:
    case ID_COL:
    case ID_FRAME:
    case ID_ISINDEX:
    case ID_BASE:
    case ID_META:
    case ID_COMMENT:
	// BASEFONT: EMPTY
	return false;
    case ID_BODY:
	// BODY: _1 * + _2
	if( check_array(childID, tag_list_1) ) return true;
	return check_array(childID, tag_list_2);
    case ID_ADDRESS:
	// ADDRESS: ( _0 | P ) *
	if( check_array(childID, tag_list_0) ) return true;
	return (childID == ID_P);
  // it is not this simple -ak
  // or maybe it is -ak
    case ID_FONT:
        // special handling for FONT: (_0 | 3)
        return check_array(childID, tag_list_3) ||
            check_array(childID, tag_list_0);

    case ID_LI:
    case ID_DT:
    case ID_DIV:
    case ID_LAYER:
    case ID_CENTER:
    case ID_BLOCKQUOTE:
    case ID_INS:
    case ID_DEL:
    case ID_DD:
    case ID_TH:
    case ID_TD:
    case ID_IFRAME:
    case ID_NOFRAMES:
    case ID_NOSCRIPT:
	// DIV: _1 *
	return check_array(childID, tag_list_1);
    case ID_A:
	// A: _0 * - A
	if( check_array(childID, tag_list_0) ) return true;
        if ( childID == ID_TABLE ) return false; // no table in anchor
	if( check_array(childID, tag_list_3) ) return true;
	if( childID == ID_ADDRESS ) return true;
	return false;
    case ID_MAP:
	// MAP: ( _3 + | AREA + )
	if( check_array(childID, tag_list_3) ) return true;
	return (childID == ID_AREA);
    case ID_OBJECT:
    case ID_EMBED:
    case ID_APPLET:
	// OBJECT: _4 *
	return check_array(childID, tag_list_4);
    case ID_PRE:
	// PRE: _0 * - _5
	return check_array(childID, tag_list_1);
    case ID_DL:
	// DL: _6 +
	return check_array(childID, tag_list_6);
    case ID_OL:
    case ID_UL:
	// OL: LI +
	if(childID == ID_LI) return true;
	return false;
    case ID_DIR:
    case ID_MENU:
	// (DIR|MENU): LI + - _3
	if(childID == ID_LI) return true;
	return false;
    case ID_FORM:
	// FORM: _1 * - FORM
	return check_array(childID, tag_list_1);
    case ID_LABEL:
	// LABEL: _0 * - LABEL
	return check_array(childID, tag_list_0);
    case ID_SELECT:
	// SELECT: _7 +
	return check_array(childID, tag_list_7);
    case ID_OPTGROUP:
	// OPTGROUP: OPTION +
	if(childID == ID_OPTION) return true;
	return false;
    case ID_OPTION:
    case ID_TEXTAREA:
    case ID_TITLE:
    case ID_STYLE:
    case ID_SCRIPT:
	// OPTION: TEXT
	if(childID == ID_TEXT) return true;
	return false;
    case ID_FIELDSET:
	// FIELDSET: ( TEXT , LEGEND , _1 * )
	if(childID == ID_TEXT) return true;
	if(childID == ID_LEGEND) return true;
	return check_array(childID, tag_list_1);
    case ID_BUTTON:
	// BUTTON: _1 * - _8
	return check_array(childID, tag_list_1);
    case ID_TABLE:
	// TABLE: ( CAPTION ? , ( COL * | COLGROUP * ) , THEAD ? , TFOOT ? , TBODY + )
	switch(childID)
	{
	case ID_CAPTION:
	case ID_COL:
	case ID_COLGROUP:
	case ID_THEAD:
	case ID_TFOOT:
	case ID_TBODY:
	    return true;
	default:
	    return false;
	}
    case ID_THEAD:
    case ID_TFOOT:
    case ID_TBODY:
	// THEAD: TR +
	if(childID == ID_TR) return true;
	return false;
    case ID_COLGROUP:
	// COLGROUP: COL *
	if(childID == ID_COL) return true;
	return false;
    case ID_TR:
	// TR: _9 +
	return check_array(childID, tag_list_9);
	return false;
    case ID_FRAMESET:
	// FRAMESET: ( _10 + & NOFRAMES ? )
	return check_array(childID, tag_list_10);
	return (childID == ID_NOFRAMES);
    case ID_HEAD:
	// HEAD: ( TITLE & ISINDEX ? & BASE ? ) + _11
	if(childID == ID_TITLE || childID == ID_ISINDEX || childID == ID_BASE)
	    return true;
	return check_array(childID, tag_list_11);
    case ID_HTML:
	// HTML: ( HEAD , COMMENT, ( BODY | ( FRAMESET & NOFRAMES ? ) ) )
	switch(childID)
	{
	case ID_HEAD:
	case ID_COMMENT:
	case ID_BODY:
	case ID_FRAMESET:
	case ID_NOFRAMES:
	case ID_SCRIPT:
	    return true;
	default:
	    return false;
	}
    default:
	kdDebug( 6030 ) << "unhandled tag in dtd.cpp:checkChild(): tagID=" << tagID << "!" << endl;
	return false;
    }
}

void DOM::addForbidden(int tagId, ushort *forbiddenTags)
{
    switch(tagId)
    {
    case ID_A:
	// we allow nested anchors. The innermost one wil be taken...
	//forbiddenTags[ID_A]++;
	break;
    case ID_PRE:
	//forbiddenTags[ID_IMG]++;
	forbiddenTags[ID_OBJECT]++;
	forbiddenTags[ID_EMBED]++;
	forbiddenTags[ID_APPLET]++;
	// why forbid them. We can deal with them in PRE
	//forbiddenTags[ID_BIG]++;
	//forbiddenTags[ID_SMALL]++;
	//forbiddenTags[ID_SUB]++;
	//forbiddenTags[ID_SUP]++;
	forbiddenTags[ID_BASEFONT]++;
	break;
    case ID_DIR:
    case ID_MENU:
	forbiddenTags[ID_P]++;
	forbiddenTags[ID_H1]++;
	forbiddenTags[ID_H2]++;
	forbiddenTags[ID_H3]++;
	forbiddenTags[ID_H4]++;
	forbiddenTags[ID_H5]++;
	forbiddenTags[ID_H6]++;
	forbiddenTags[ID_UL]++;
	forbiddenTags[ID_OL]++;
	forbiddenTags[ID_DIR]++;
	forbiddenTags[ID_MENU]++;
	forbiddenTags[ID_PRE]++;
	forbiddenTags[ID_DL]++;
	forbiddenTags[ID_DIV]++;
	forbiddenTags[ID_CENTER]++;
	forbiddenTags[ID_NOSCRIPT]++;
	forbiddenTags[ID_NOFRAMES]++;
	forbiddenTags[ID_BLOCKQUOTE]++;
	forbiddenTags[ID_FORM]++;
	forbiddenTags[ID_ISINDEX]++;
	forbiddenTags[ID_HR]++;
	forbiddenTags[ID_TABLE]++;
	forbiddenTags[ID_FIELDSET]++;
	forbiddenTags[ID_ADDRESS]++;
	break;
    case ID_FORM:
	// the parser deals with them in another way. helps supporting some broken html
	//forbiddenTags[ID_FORM]++;
	break;
    case ID_LABEL:
	forbiddenTags[ID_LABEL]++;
	break;
    case ID_BUTTON:
	forbiddenTags[ID_A]++;
	forbiddenTags[ID_INPUT]++;
	forbiddenTags[ID_SELECT]++;
	forbiddenTags[ID_TEXTAREA]++;
	forbiddenTags[ID_LABEL]++;
	forbiddenTags[ID_BUTTON]++;
	forbiddenTags[ID_FORM]++;
	forbiddenTags[ID_ISINDEX]++;
	forbiddenTags[ID_FIELDSET]++;
	forbiddenTags[ID_IFRAME]++;
	break;
    default:
	break;
    }
}

void DOM::removeForbidden(int tagId, ushort *forbiddenTags)
{
    switch(tagId)
    {
    case ID_A:
	//forbiddenTags[ID_A]--;
	break;
    case ID_PRE:
	//forbiddenTags[ID_IMG]--;
	forbiddenTags[ID_OBJECT]--;
	forbiddenTags[ID_EMBED]--;
	forbiddenTags[ID_APPLET]--;
	//forbiddenTags[ID_BIG]--;
	//forbiddenTags[ID_SMALL]--;
	//forbiddenTags[ID_SUB]--;
	//forbiddenTags[ID_SUP]--;
	forbiddenTags[ID_BASEFONT]--;
	break;
    case ID_DIR:
    case ID_MENU:
	forbiddenTags[ID_P]--;
	forbiddenTags[ID_H1]--;
	forbiddenTags[ID_H2]--;
	forbiddenTags[ID_H3]--;
	forbiddenTags[ID_H4]--;
	forbiddenTags[ID_H5]--;
	forbiddenTags[ID_H6]--;
	forbiddenTags[ID_UL]--;
	forbiddenTags[ID_OL]--;
	forbiddenTags[ID_DIR]--;
	forbiddenTags[ID_MENU]--;
	forbiddenTags[ID_PRE]--;
	forbiddenTags[ID_DL]--;
	forbiddenTags[ID_DIV]--;
	forbiddenTags[ID_CENTER]--;
	forbiddenTags[ID_NOSCRIPT]--;
	forbiddenTags[ID_NOFRAMES]--;
	forbiddenTags[ID_BLOCKQUOTE]--;
	forbiddenTags[ID_FORM]--;
	forbiddenTags[ID_ISINDEX]--;
	forbiddenTags[ID_HR]--;
	forbiddenTags[ID_TABLE]--;
	forbiddenTags[ID_FIELDSET]--;
	forbiddenTags[ID_ADDRESS]--;
	break;
    case ID_FORM:
	//forbiddenTags[ID_FORM]--;
	break;
    case ID_LABEL:
	forbiddenTags[ID_LABEL]--;
	break;
    case ID_BUTTON:
	forbiddenTags[ID_A]--;
	forbiddenTags[ID_INPUT]--;
	forbiddenTags[ID_SELECT]--;
	forbiddenTags[ID_TEXTAREA]--;
	forbiddenTags[ID_LABEL]--;
	forbiddenTags[ID_BUTTON]--;
	forbiddenTags[ID_FORM]--;
	forbiddenTags[ID_ISINDEX]--;
	forbiddenTags[ID_FIELDSET]--;
	forbiddenTags[ID_IFRAME]--;
	break;
    default:
	break;
    }
}





struct attr_priv {
    attr_priv() { id = len = 0, val = 0; }
    attr_priv(ushort i, const QChar *v, ushort l)
    { id =i; len = l, val = v; }
    ushort id;
    const QChar *val;
    ushort len;
};

DOMString find_attr(ushort id, const attr_priv *attrs)
{
    int i = 0;
    while(attrs[i].id != 0)
    {
        if(attrs[i].id == id)
            return DOMString(attrs[i].val, attrs[i].len);
        i++;
    }
    return DOMString();
}

static const QChar value_1_0 [] = { 'N','O','N','E' };

attr_priv attr_list_1[] = {
    attr_priv(ATTR_CLEAR, value_1_0, 4)
};

static const QChar value_2_0 [] = { 'R','E','C','T' };

attr_priv attr_list_2[] = {
    attr_priv(ATTR_SHAPE, value_2_0, 4)
};

static const QChar value_3_0 [] = { 'R','E','C','T' };

attr_priv attr_list_3[] = {
    attr_priv(ATTR_SHAPE, value_3_0, 4)
};

static const QChar value_4_0 [] = { 'D','A','T','A' };

attr_priv attr_list_4[] = {
    attr_priv(ATTR_VALUETYPE, value_4_0, 4)
};

static const QChar value_5_0 [] = { 'G','E','T' };
static const QChar value_5_1 [] = { 'A','P','P','L','I','C','A','T','I','O','N','/','X','-','W','W','W','-','F','O','R','M','-','U','R','L','E','N','C','O','D','E','D' };

attr_priv attr_list_5[] = {
    attr_priv(ATTR_METHOD, value_5_0, 3),
    attr_priv(ATTR_ENCTYPE, value_5_1, 33)
};

static const QChar value_6_0 [] = { 'T','E','X','T' };

attr_priv attr_list_6[] = {
    attr_priv(ATTR_TYPE, value_6_0, 4)
};

static const QChar value_7_0 [] = { 'S','U','B','M','I','T' };

attr_priv attr_list_7[] = {
    attr_priv(ATTR_TYPE, value_7_0, 6)
};

static const QChar value_8_0 [] = { '1' };

attr_priv attr_list_8[] = {
    attr_priv(ATTR_SPAN, value_8_0, 1)
};

static const QChar value_9_0 [] = { '1' };

attr_priv attr_list_9[] = {
    attr_priv(ATTR_SPAN, value_9_0, 1)
};

static const QChar value_10_0 [] = { '1' };
static const QChar value_10_1 [] = { '1' };

attr_priv attr_list_10[] = {
    attr_priv(ATTR_ROWSPAN, value_10_0, 1),
    attr_priv(ATTR_COLSPAN, value_10_1, 1)
};

static const QChar value_11_0 [] = { '1' };
static const QChar value_11_1 [] = { 'A','U','T','O' };

attr_priv attr_list_11[] = {
    attr_priv(ATTR_FRAMEBORDER, value_11_0, 1),
    attr_priv(ATTR_SCROLLING, value_11_1, 4)
};

static const QChar value_12_0 [] = { '1' };
static const QChar value_12_1 [] = { 'A','U','T','O' };

attr_priv attr_list_12[] = {
    attr_priv(ATTR_FRAMEBORDER, value_12_0, 1),
    attr_priv(ATTR_SCROLLING, value_12_1, 4)
};

static const QChar value_13_0 [] = { '-','/','/','W','3','C','/','/','D','T','D' };
static const QChar value_13_1 [] = { 'T','R','A','N','S','I','T','I','O','N','A','L','/','/','E','N' };

attr_priv attr_list_13[] = {
    attr_priv(ATTR_VERSION, value_13_0, 11),
    attr_priv(ATTR_HTML, value_13_1, 16)
};


DOMString DOM::findDefAttrNone(ushort)
{
    return DOMString();
};
DOMString DOM::findDefAttrBR(ushort id)
{
    return find_attr(id, attr_list_1);
}
DOMString DOM::findDefAttrA(ushort id)
{
    return find_attr(id, attr_list_2);
}
DOMString DOM::findDefAttrAREA(ushort id)
{
    return find_attr(id, attr_list_3);
}
DOMString DOM::findDefAttrPARAM(ushort id)
{
    return find_attr(id, attr_list_4);
}
DOMString DOM::findDefAttrFORM(ushort id)
{
    return find_attr(id, attr_list_5);
}
DOMString DOM::findDefAttrINPUT(ushort id)
{
    return find_attr(id, attr_list_6);
}
DOMString DOM::findDefAttrBUTTON(ushort id)
{
    return find_attr(id, attr_list_7);
}
DOMString DOM::findDefAttrCOLGROUP(ushort id)
{
    return find_attr(id, attr_list_8);
}
DOMString DOM::findDefAttrCOL(ushort id)
{
    return find_attr(id, attr_list_9);
}
DOMString DOM::findDefAttrTH(ushort id)
{
    return find_attr(id, attr_list_10);
}
DOMString DOM::findDefAttrFRAME(ushort id)
{
    return find_attr(id, attr_list_11);
}
DOMString DOM::findDefAttrIFRAME(ushort id)
{
    return find_attr(id, attr_list_12);
}
DOMString DOM::findDefAttrHTML(ushort id)
{
    return find_attr(id, attr_list_13);
}
