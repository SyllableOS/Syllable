/**
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright (C) 2000 Peter Kelly (pmk@post.com)
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


#include "dom/dom_text.h"
#include "dom_xmlimpl.h"
#include "dom_docimpl.h"
#include "dom_stringimpl.h"
#include "dom_exception.h"

using namespace DOM;

EntityImpl::EntityImpl(DocumentImpl *doc) : NodeBaseImpl(doc)
{
    m_publicId = 0;
    m_systemId = 0;
    m_notationName = 0;
    m_name = 0;
}

EntityImpl::EntityImpl(DocumentImpl *doc, DOMString _name) : NodeBaseImpl(doc)
{
    m_publicId = 0;
    m_systemId = 0;
    m_notationName = 0;
    m_name = _name.implementation();
    if (m_name)
	m_name->ref();
}

EntityImpl::EntityImpl(DocumentImpl *doc, DOMString _publicId, DOMString _systemId, DOMString _notationName) : NodeBaseImpl(doc)
{
    m_publicId = _publicId.implementation();
    if (m_publicId)
	m_publicId->ref();
    m_systemId = _systemId.implementation();
    if (m_systemId)
	m_systemId->ref();
    m_notationName = _notationName.implementation();
    if (m_notationName)
	m_notationName->ref();
    m_name = 0;
}


EntityImpl::~EntityImpl()
{
    if (m_publicId)
	m_publicId->deref();
    if (m_systemId)
	m_systemId->deref();
    if (m_notationName)
	m_notationName->deref();
    if (m_name)
 	m_name->deref();
}

const DOMString EntityImpl::nodeName() const
{
    return m_name;
}

unsigned short EntityImpl::nodeType() const
{
    return Node::ENTITY_NODE;
}

DOMString EntityImpl::publicId() const
{
    return m_publicId;
}

DOMString EntityImpl::systemId() const
{
    return m_systemId;
}

DOMString EntityImpl::notationName() const
{
    return m_notationName;
}

// DOM Section 1.1.1
bool EntityImpl::childAllowed( NodeImpl *newChild )
{
    switch (newChild->nodeType()) {
	case Node::ELEMENT_NODE:
	case Node::PROCESSING_INSTRUCTION_NODE:
	case Node::COMMENT_NODE:
	case Node::TEXT_NODE:
	case Node::CDATA_SECTION_NODE:
	case Node::ENTITY_REFERENCE_NODE:
	    return true;
	    break;
	default:
	    return false;
    }
}

NodeImpl *EntityImpl::cloneNode ( bool /*deep*/, int &exceptioncode )
{
    exceptioncode = DOMException::NOT_SUPPORTED_ERR;
    return 0;
}

// -------------------------------------------------------------------------

EntityReferenceImpl::EntityReferenceImpl(DocumentImpl *doc) : NodeBaseImpl(doc)
{
    m_entityName = 0;
}

EntityReferenceImpl::EntityReferenceImpl(DocumentImpl *doc, DOMStringImpl *_entityName) : NodeBaseImpl(doc)
{
    m_entityName = _entityName;
    if (m_entityName)
	m_entityName->ref();
}

EntityReferenceImpl::~EntityReferenceImpl()
{
    if (m_entityName)
	m_entityName->deref();
}

const DOMString EntityReferenceImpl::nodeName() const
{
    return m_entityName;
}

unsigned short EntityReferenceImpl::nodeType() const
{
    return Node::ENTITY_REFERENCE_NODE;
}

// DOM Section 1.1.1
bool EntityReferenceImpl::childAllowed( NodeImpl *newChild )
{
    switch (newChild->nodeType()) {
	case Node::ELEMENT_NODE:
	case Node::PROCESSING_INSTRUCTION_NODE:
	case Node::COMMENT_NODE:
	case Node::TEXT_NODE:
	case Node::CDATA_SECTION_NODE:
	case Node::ENTITY_REFERENCE_NODE:
	    return true;
	    break;
	default:
	    return false;
    }
}

NodeImpl *EntityReferenceImpl::cloneNode ( bool deep, int &exceptioncode )
{
    EntityReferenceImpl *clone = new EntityReferenceImpl(document,m_entityName);
    // ### make sure children are readonly
    // ### since we are a reference, should we clone children anyway (even if not deep?)
    if (deep)
	cloneChildNodes(clone,exceptioncode);
    return clone;
}

// -------------------------------------------------------------------------

NotationImpl::NotationImpl(DocumentImpl *doc) : NodeBaseImpl(doc)
{
    m_publicId = 0;
    m_systemId = 0;
    m_name = 0;
}

NotationImpl::NotationImpl(DocumentImpl *doc, DOMString _name, DOMString _publicId, DOMString _systemId) : NodeBaseImpl(doc)
{
    m_name = _name.implementation();
    if (m_name)
	m_name->ref();
    m_publicId = _publicId.implementation();
    if (m_publicId)
	m_publicId->ref();
    m_systemId = _systemId.implementation();
    if (m_systemId)
	m_systemId->ref();
}

NotationImpl::~NotationImpl()
{
    if (m_name)
	m_name->deref();
    if (m_publicId)
	m_publicId->deref();
    if (m_systemId)
	m_systemId->deref();
}

const DOMString NotationImpl::nodeName() const
{
    return m_name;
}

unsigned short NotationImpl::nodeType() const
{
    return Node::NOTATION_NODE;
}

DOMString NotationImpl::publicId() const
{
    return m_publicId;
}

DOMString NotationImpl::systemId() const
{
    return m_systemId;
}

// DOM Section 1.1.1
bool NotationImpl::childAllowed( NodeImpl */*newChild*/ )
{
    return false;
}

NodeImpl *NotationImpl::cloneNode ( bool /*deep*/, int &exceptioncode )
{
    exceptioncode = DOMException::NOT_SUPPORTED_ERR;
    return 0;
}

// -------------------------------------------------------------------------

ProcessingInstructionImpl::ProcessingInstructionImpl(DocumentImpl *doc) : NodeBaseImpl(doc)
{
    m_target = 0;
    m_data = 0;
}

ProcessingInstructionImpl::ProcessingInstructionImpl(DocumentImpl *doc, DOMString _target, DOMString _data) : NodeBaseImpl(doc)
{
    m_target = _target.implementation();
    if (m_target)
	m_target->ref();
    m_data = _data.implementation();
    if (m_data)
	m_data->ref();
}

ProcessingInstructionImpl::~ProcessingInstructionImpl()
{
    if (m_target)
	m_target->deref();
    if (m_data)
	m_data->deref();	
}

const DOMString ProcessingInstructionImpl::nodeName() const
{
    return m_target;
}

unsigned short ProcessingInstructionImpl::nodeType() const
{
    return Node::PROCESSING_INSTRUCTION_NODE;
}

DOMString ProcessingInstructionImpl::nodeValue() const
{
    return m_data;
}

DOMString ProcessingInstructionImpl::target() const
{
    return m_target;
}

DOMString ProcessingInstructionImpl::data() const
{
    return m_data;
}

void ProcessingInstructionImpl::setData( const DOMString &_data )
{
    if (m_data)
	m_data->deref();
    m_data = _data.implementation();
    if (m_data)
	m_data->ref();
}

// DOM Section 1.1.1
bool ProcessingInstructionImpl::childAllowed( NodeImpl */*newChild*/ )
{
    return false;
}

NodeImpl *ProcessingInstructionImpl::cloneNode ( bool /*deep*/, int &/*exceptioncode*/ )
{
    return new ProcessingInstructionImpl(document,m_target,m_data);
}




