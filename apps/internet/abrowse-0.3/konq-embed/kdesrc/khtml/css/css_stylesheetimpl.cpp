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

//#define CSS_STYLESHEET_DEBUG

#include "css_stylesheetimpl.h"

#include "css_stylesheet.h"
#include "css_rule.h"
#include "css_ruleimpl.h"
#include "css_valueimpl.h"
#include "cssparser.h"

#include "dom_string.h"
#include "dom_exception.h"
#include "dom_nodeimpl.h"
#include "html_documentimpl.h"
#include "misc/loader.h"

#include <kdebug.h>


using namespace DOM;
using namespace khtml;
// --------------------------------------------------------------------------------

StyleSheetImpl::StyleSheetImpl(StyleSheetImpl *parentSheet, DOMString href)
    : StyleListImpl(parentSheet)
{
    m_disabled = false;
    m_media = 0;
    m_parentNode = 0;
    m_strHref = href;
}


StyleSheetImpl::StyleSheetImpl(DOM::NodeImpl *parentNode, DOMString href)
    : StyleListImpl()
{
    m_parentNode = parentNode;
    m_disabled = false;
    m_media = 0;
    m_strHref = href;
}

StyleSheetImpl::StyleSheetImpl(StyleBaseImpl *owner, DOMString href)
    : StyleListImpl(owner)
{
    m_disabled = false;
    m_media = 0;
    m_parentNode = 0;
    m_strHref = href;
}

StyleSheetImpl::~StyleSheetImpl()
{
    if(m_media) m_media->deref();
}

bool StyleSheetImpl::deleteMe()
{
    if(!m_parent && _ref <= 0) return true;
    return false;
}

bool StyleSheetImpl::disabled() const
{
    return m_disabled;
}

void StyleSheetImpl::setDisabled( bool disabled )
{
    m_disabled = disabled;
}

NodeImpl *StyleSheetImpl::ownerNode() const
{
    return m_parentNode;
}

StyleSheetImpl *StyleSheetImpl::parentStyleSheet() const
{
    if( !m_parent ) return 0;
    if( m_parent->isStyleSheet() ) return static_cast<StyleSheetImpl *>(m_parent);
    return 0;
}

DOMString StyleSheetImpl::href() const
{
    return m_strHref;
}

DOMString StyleSheetImpl::title() const
{
    return m_strTitle;
}

MediaListImpl *StyleSheetImpl::media() const
{
    return m_media;
}

// -----------------------------------------------------------------------


CSSStyleSheetImpl::CSSStyleSheetImpl(CSSStyleSheetImpl *parentSheet, DOMString href)
    : StyleSheetImpl(parentSheet, href)
{
    m_lstChildren = new QList<StyleBaseImpl>;
    m_doc = 0;
}

CSSStyleSheetImpl::CSSStyleSheetImpl(DOM::NodeImpl *parentNode, DOMString href)
    : StyleSheetImpl(parentNode, href)
{
    m_lstChildren = new QList<StyleBaseImpl>;
    m_doc = parentNode->nodeType() == Node::DOCUMENT_NODE ? static_cast<DocumentImpl*>(parentNode) : m_doc = parentNode->ownerDocument();
}

CSSStyleSheetImpl::CSSStyleSheetImpl(CSSRuleImpl *ownerRule, DOMString href)
    : StyleSheetImpl(ownerRule, href)
{
    m_lstChildren = new QList<StyleBaseImpl>;
    m_doc = 0;
}

CSSStyleSheetImpl::CSSStyleSheetImpl(DOM::NodeImpl *parentNode, CSSStyleSheetImpl *orig)
    : StyleSheetImpl(parentNode, orig->m_strHref)
{
    m_lstChildren = new QList<StyleBaseImpl>;
    StyleBaseImpl *rule;
    for ( rule = orig->m_lstChildren->first(); rule != 0; rule = orig->m_lstChildren->next() )
    {
	m_lstChildren->append(rule);
	rule->setParent(this);
    }
    m_doc = parentNode->nodeType() == Node::DOCUMENT_NODE ? static_cast<DocumentImpl*>(parentNode) : m_doc = parentNode->ownerDocument();
}

CSSStyleSheetImpl::CSSStyleSheetImpl(CSSRuleImpl *ownerRule, CSSStyleSheetImpl *orig)
    : StyleSheetImpl(ownerRule, orig->m_strHref)
{
    m_lstChildren = new QList<StyleBaseImpl>;
    StyleBaseImpl *rule;
    for ( rule = orig->m_lstChildren->first(); rule != 0; rule = orig->m_lstChildren->next() )
    {
	m_lstChildren->append(rule);
	rule->setParent(this);
    }
    m_doc  = 0;
}

CSSStyleSheetImpl::~CSSStyleSheetImpl()
{
    // m_lstChildren is deleted in StyleListImpl
}

CSSRuleImpl *CSSStyleSheetImpl::ownerRule() const
{
    if( !m_parent ) return 0;
    if( m_parent->isRule() ) return static_cast<CSSRuleImpl *>(m_parent);
    return 0;
}

CSSRuleList CSSStyleSheetImpl::cssRules()
{
    return this;
}

unsigned long CSSStyleSheetImpl::insertRule( const DOMString &rule, unsigned long index, int &exceptioncode )
{
    exceptioncode = 0;
    if(index > m_lstChildren->count()) {
	exceptioncode = DOMException::INDEX_SIZE_ERR;
	return 0;
    }
    const QChar *curP = rule.unicode();
    const QChar *endP = rule.unicode()+rule.length();
    CSSRuleImpl *r = parseRule(curP, endP);

    if(!r) {
	exceptioncode = CSSException::SYNTAX_ERR + CSSException::_EXCEPTION_OFFSET;
	return 0;
    }
    // ###
    // HIERARCHY_REQUEST_ERR: Raised if the rule cannot be inserted at the specified index e.g. if an
    //@import rule is inserted after a standard rule set or other at-rule.
    m_lstChildren->insert(index, r);
    return index;
}

void CSSStyleSheetImpl::deleteRule( unsigned long index, int &exceptioncode )
{
    exceptioncode = 0;
    StyleBaseImpl *b = m_lstChildren->take(index);
    if(!b) {
	exceptioncode = DOMException::INDEX_SIZE_ERR;
	return;
    }
    b->deref();
}

bool CSSStyleSheetImpl::parseString(const DOMString &string, bool strict)
{
    strictParsing = strict;
    QString preprocessed = preprocess(string.string());

#ifdef CSS_STYLESHEET_DEBUG
    kdDebug( 6080 ) << "parsing sheet, len=" << string.length() << ", sheet is " << string.string() << endl;
#endif

    const QChar *curP = preprocessed.unicode();
    const QChar *endP = preprocessed.unicode()+preprocessed.length() - 1;

#ifdef CSS_STYLESHEET_DEBUG
    kdDebug( 6080 ) << "preprocessed sheet, len=" << preprocessed.length() << ", sheet is " << preprocessed << endl;
#endif

    while (curP && (curP < endP))
    {
        CSSRuleImpl *rule = parseRule(curP, endP);
        if(rule)
	{
	   m_lstChildren->append(rule);
	   rule->setParent(this);
	}
    }
    return true;
}

bool CSSStyleSheetImpl::isLoading()
{
    StyleBaseImpl *rule;
    for ( rule = m_lstChildren->first(); rule != 0; rule = m_lstChildren->next() )
    {
	if(rule->isImportRule())
	{
	    CSSImportRuleImpl *import = static_cast<CSSImportRuleImpl *>(rule);
#ifdef CSS_STYLESHEET_DEBUG
	    kdDebug( 6080 ) << "found import" << endl;
#endif
	    if(import->isLoading())
	    {
#ifdef CSS_STYLESHEET_DEBUG
		kdDebug( 6080 ) << "--> not loaded" << endl;
#endif
		return true;
	    }
	}
    }
    return false;
}

void CSSStyleSheetImpl::checkLoaded()
{
    if(isLoading()) return;
    if(m_parent) m_parent->checkLoaded();
    if(m_parentNode) m_parentNode->sheetLoaded();
}

void CSSStyleSheetImpl::setNonCSSHints()
{
    StyleBaseImpl *rule = m_lstChildren->first();
    while(rule) {
	if(rule->isStyleRule()) {
	    static_cast<CSSStyleRuleImpl *>(rule)->setNonCSSHints();
	}
	rule = m_lstChildren->next();
    }
}

khtml::DocLoader *CSSStyleSheetImpl::docLoader()
{
    if ( !m_doc ) // doc is 0 for the user- and default-sheet!
        return 0;

    // ### remove? (clients just use sheet->doc()->docLoader())
    return m_doc->docLoader();
}

// ---------------------------------------------------------------------------------------------

StyleSheetListImpl::StyleSheetListImpl(StyleSheetImpl *parentSheet)
    : StyleListImpl(parentSheet)
{
}

StyleSheetListImpl::~StyleSheetListImpl()
{
}

unsigned long StyleSheetListImpl::length() const
{
    //###
    return 0;
}

StyleSheetImpl *StyleSheetListImpl::item( unsigned long /*index*/ )
{
    //###
    return 0;
}


// --------------------------------------------------------------------------------------------

MediaListImpl::MediaListImpl(CSSStyleSheetImpl *parentSheet)
    : StyleBaseImpl(parentSheet)
{
    m_lstMedia.setAutoDelete(true);
}

MediaListImpl::MediaListImpl(CSSRuleImpl *parentRule)
    : StyleBaseImpl(parentRule)
{
    m_lstMedia.setAutoDelete(true);
}

MediaListImpl::~MediaListImpl()
{
}

CSSStyleSheetImpl *MediaListImpl::parentStyleSheet() const
{
    if( m_parent->isCSSStyleSheet() ) return static_cast<CSSStyleSheetImpl *>(m_parent);
    return 0;
}

CSSRuleImpl *MediaListImpl::parentRule() const
{
    if( m_parent->isRule() ) return static_cast<CSSRuleImpl *>(m_parent);
    return 0;
}

unsigned long MediaListImpl::length() const
{
    return m_lstMedia.count();
}

DOMString MediaListImpl::item( unsigned long index )
{
    return *m_lstMedia.at(index);
}

void MediaListImpl::del( const DOMString &oldMedium )
{
    int i;
    for(i = 0; i < (int)m_lstMedia.count(); i++)
	if( *(m_lstMedia.at(i)) == oldMedium )
	{
	    m_lstMedia.remove(i);
	    return;
	}
}

void MediaListImpl::append( const DOMString &newMedium )
{
    DOMString *str = new DOMString(newMedium);
    m_lstMedia.append(str);
}

DOM::DOMString MediaListImpl::cssText() const
{
    // ###
    return 0;
}

void MediaListImpl::setCssText(DOM::DOMString /*str*/)
{
    // ###
}

