/*
 *  This file is part of the KDE libraries
 *  Copyright (C) 2000 Harri Porten (porten@kde.org)
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <qptrdict.h>

#include <kjs/operations.h>
#include <khtmlview.h>
#include <dom_string.h>
#include <dom_xml.h>
#include <xml/dom_nodeimpl.h>
#include <rendering/render_object.h>
#include <htmltags.h>

#include "kjs_text.h"
#include "kjs_dom.h"
#include "kjs_html.h"
#include "kjs_css.h"
#include "dom/dom_exception.h"

using namespace KJS;

QPtrDict<DOMNode> nodes(1021);
QPtrDict<DOMNamedNodeMap> namedNodeMaps;
QPtrDict<DOMNodeList> nodeLists;
QPtrDict<DOMDOMImplementation> domImplementations;

// -------------------------------------------------------------------------

const TypeInfo DOMNode::info = { "Node", HostType, 0, 0, 0 };

DOMNode::~DOMNode()
{
  nodes.remove(node.handle());
}

Boolean DOMNode::toBoolean() const
{
    return Boolean(!node.isNull());
}

bool DOMNode::hasProperty(const UString &p, bool) const
{
    KJSO n = getDOMNode(node);
    KJSO r = n.get(p);
    return !r.isA(UndefinedType);
}

KJSO DOMNode::tryGet(const UString &p) const
{
  KJSO result;
  khtml::RenderObject *rend = node.handle() ? node.handle()->renderer() : 0L;

  if (p == "nodeName")
    result = getString(node.nodeName());
  else if (p == "nodeValue")
    result = getString(node.nodeValue());
  else if (p == "nodeType")
    result = Number((unsigned int)node.nodeType());
  else if (p == "parentNode")
    result = getDOMNode(node.parentNode());
  else if (p == "childNodes")
    result = getDOMNodeList(node.childNodes());
  else if (p == "firstChild")
    result = getDOMNode(node.firstChild());
  else if (p == "lastChild")
    result = getDOMNode(node.lastChild());
  else if (p == "previousSibling")
    result = getDOMNode(node.previousSibling());
  else if (p == "nextSibling")
    result = getDOMNode(node.nextSibling());
  else if (p == "attributes")
    result = getDOMNamedNodeMap(node.attributes());
//  else if (p == "namespaceURI") // new for DOM2 - not yet in khtml
//    result = getString(node.namespaceURI());
//  else if (p == "prefix") // new for DOM2 - not yet in khtml
//    result = getString(node.prefix());
//  else if (p == "localName") // new for DOM2 - not yet in khtml
//    result = getString(node.localName());
  else if (p == "ownerDocument")
    result = getDOMNode(node.ownerDocument());
  // methods
  else if (p == "insertBefore")
    result = new DOMNodeFunc(node, DOMNodeFunc::InsertBefore);
  else if (p == "replaceChild")
    result = new DOMNodeFunc(node, DOMNodeFunc::ReplaceChild);
  else if (p == "removeChild")
    result = new DOMNodeFunc(node, DOMNodeFunc::RemoveChild);
  else if (p == "appendChild")
    result = new DOMNodeFunc(node, DOMNodeFunc::AppendChild);
  else if (p == "hasChildNodes")
    result = new DOMNodeFunc(node, DOMNodeFunc::HasChildNodes);
  else if (p == "cloneNode")
    result = new DOMNodeFunc(node, DOMNodeFunc::CloneNode);
//  else if (p == "normalize") // moved here from Element in DOM2
//    result = new DOMNodeFunc(node, DOMNodeFunc::Normalize);
//  else if (p == "supports") // new for DOM2 - not yet in khtml
//    result = new DOMNodeFunc(node, DOMNodeFunc::Supports);
  // no DOM standard, found in IE only
  else if (p == "offsetLeft")
    result = rend ? static_cast<KJSO>(Number(rend->xPos())) : KJSO(Undefined());
  else if (p == "offsetTop")
    result = rend ? static_cast<KJSO>(Number(rend->yPos())) : KJSO(Undefined());
  else if (p == "offsetParent")
    result = getDOMNode(node.parentNode()); // not necessarily correct
  else if (p == "clientWidth")
      result = rend ? static_cast<KJSO>(Number(rend->contentWidth())) : KJSO(Undefined());
  else if (p == "clientHeight")
      result = rend ? static_cast<KJSO>(Number(rend->contentHeight())) : KJSO(Undefined());
  else if (p == "scrollLeft")
      result = rend ? static_cast<KJSO>(Number(-rend->xPos() + node.ownerDocument().view()->contentsX())) : KJSO(Undefined());
  else if (p == "scrollTop")
      result = rend ? static_cast<KJSO>(Number(-rend->yPos() + node.ownerDocument().view()->contentsY())) : KJSO(Undefined());
  else
    result = Imp::get(p);

  return result;
}

void DOMNode::tryPut(const UString &p, const KJSO& v)
{
  if (p == "nodeValue") {
    node.setNodeValue(v.toString().value().string());
  }
//  else if (p == "prefix") { // new for DOM2 - not yet in khtml
//    node.setPrefix(v.toString().value().string());
//  }
  else
    Imp::put(p, v);
}

KJSO DOMNode::toPrimitive(Type /*preferred*/) const
{
  if (node.isNull())
    return Null();

  return toString();
}

String DOMNode::toString() const
{
  if (node.isNull())
    return String("null");
  const char *s = "DOMNode"; // fallback

  static const struct ElementName {
	int id;
	const char *name;
  } elementNames[] = {
      { ID_A,		"HTMLAnchorElement" },
      { ID_BODY, 	"HTMLBodyElement" },
      { ID_OBJECT,	"HTMLObjectElement" },
      { ID_OPTION,	"Option" },
      // ### other >90 elements from htmltags.h. and put this into khtml.
      { -1, 		0 }
  };

  if (node.nodeType() == DOM::Node::ELEMENT_NODE) {
    int id = static_cast<DOM::Element>(node).elementId();
    const ElementName *eln = elementNames;
    while (eln->id != -1) {
      if (eln->id == id) {
	s = eln->name;
	break;
      }
      eln++;
    }
  } else {
      s = typeInfo()->name;
  }

  return String("[object " + UString(s) + "]");
}

Completion DOMNodeFunc::tryExecute(const List &args)
{
  KJSO result;

  if (id == HasChildNodes)
    result = Boolean(node.hasChildNodes());
  else if (id == CloneNode) {
    Boolean b = args[0].toBoolean();
    result = getDOMNode(node.cloneNode(b.value()));
  } else {
    DOM::Node n1 = toNode(args[0]);
    if (id == AppendChild) {
      result = getDOMNode(node.appendChild(n1));
    } else if (id == RemoveChild) {
      result = getDOMNode(node.removeChild(n1));
    } else {
      DOM::Node n2 = toNode(args[1]);
      if (id == InsertBefore)
	result = getDOMNode(node.insertBefore(n1, n2));
      else
	result = getDOMNode(node.replaceChild(n1, n2));
    }
  }
  return Completion(Normal, result);
}

// -------------------------------------------------------------------------

const TypeInfo DOMNodeList::info = { "NodeList", HostType, 0, 0, 0 };

DOMNodeList::~DOMNodeList()
{
  nodeLists.remove(list.handle());
}

KJSO DOMNodeList::tryGet(const UString &p) const
{
  KJSO result;

  if (p == "length")
    result = Number(list.length());
  else if (p == "item")
    result = new DOMNodeListFunc(list, DOMNodeListFunc::Item);
  else {
    // array index ?
    bool ok;
    long unsigned int idx = p.toULong(&ok);
    if (ok)
      result = getDOMNode(list.item(idx));
    else
      result = HostImp::get(p);
  }

  return result;
}

Completion DOMNodeListFunc::tryExecute(const List &args)
{
  KJSO result;

  if (id == Item)
    result = getDOMNode(list.item(args[0].toNumber().intValue()));
  return Completion(Normal, result);
}

// -------------------------------------------------------------------------

const TypeInfo DOMAttr::info = { "Attr", HostType, &DOMNode::info, 0, 0 };

KJSO DOMAttr::tryGet(const UString &p) const
{
  KJSO result;
  if (p == "name") {
    result = getString(static_cast<DOM::Attr>(node).name()); }
  else if (p == "specified")
    result = Boolean(static_cast<DOM::Attr>(node).specified());
  else if (p == "value")
    result = getString(static_cast<DOM::Attr>(node).value());
//  else if (p == "ownerElement") // new for DOM2 - not yet in khtml
//    rseult = getDOMNode(static_cast<DOM::Attr>(node).ownerElement());
  else
    result = DOMNode::tryGet(p);

  return result;
}

void DOMAttr::tryPut(const UString &p, const KJSO& v)
{
  if (p == "value")
    static_cast<DOM::Attr>(node).setValue(v.toString().value().string());
  else
    DOMNode::tryPut(p,v);
}

// -------------------------------------------------------------------------

const TypeInfo DOMDocument::info = { "Document", HostType,
				     &DOMNode::info, 0, 0 };

KJSO DOMDocument::tryGet(const UString &p) const
{
  DOM::Document doc = static_cast<DOM::Document>(node);

  if (p == "doctype")
    return getDOMNode(doc.doctype());
  if (p == "implementation")
    return getDOMDOMImplementation(doc.implementation());
  else if (p == "documentElement")
    return getDOMNode(doc.documentElement());
  // methods
  else if (p == "createElement")
    return new DOMDocFunction(doc, DOMDocFunction::CreateElement);
  else if (p == "createDocumentFragment")
    return new DOMDocFunction(doc, DOMDocFunction::CreateDocumentFragment);
  else if (p == "createTextNode")
    return new DOMDocFunction(doc, DOMDocFunction::CreateTextNode);
  else if (p == "createComment")
    return new DOMDocFunction(doc, DOMDocFunction::CreateComment);
  else if (p == "createCDATASection")
    return new DOMDocFunction(doc, DOMDocFunction::CreateCDATASection);
  else if (p == "createProcessingInstruction")
    return new DOMDocFunction(doc, DOMDocFunction::CreateProcessingInstruction);
  else if (p == "createAttribute")
    return new DOMDocFunction(doc, DOMDocFunction::CreateAttribute);
  else if (p == "createEntityReference")
    return new DOMDocFunction(doc, DOMDocFunction::CreateEntityReference);
  else if (p == "getElementsByTagName")
    return new DOMDocFunction(doc, DOMDocFunction::GetElementsByTagName);
//  else if (p == "importNode") // new for DOM2 - not yet in khtml
//    return new DOMDocFunction(doc, DOMDocFunction::ImportNode);
  else if (p == "createElementNS") // new for DOM2 - not yet in khtml
    return new DOMDocFunction(doc, DOMDocFunction::CreateElementNS);
  else if (p == "createAttributeNS") // new for DOM2 - not yet in khtml
    return new DOMDocFunction(doc, DOMDocFunction::CreateAttributeNS);
/*  else if (p == "getElementsByTagNameNS") // new for DOM2 - not yet in khtml
    return new DOMDocFunction(doc, DOMDocFunction::GetElementsByTagNameNS);
  else if (p == "getElementById") // new for DOM2 - not yet in khtml
    return new DOMDocFunction(doc, DOMDocFunction::GetElementById);*/
  // look in base class (Document)

  return DOMNode::tryGet(p);
}

DOMDocFunction::DOMDocFunction(DOM::Document d, int i)
  : doc(d), id(i)
{
}

#include <html_document.h>

Completion DOMDocFunction::tryExecute(const List &args)
{
  KJSO result;
  String str = args[0].toString();
  DOM::DOMString s = str.value().string();

  switch(id) {
  case CreateElement:
    result = getDOMNode(doc.createElement(s));
    break;
  case CreateDocumentFragment:
    result = getDOMNode(doc.createDocumentFragment());
    break;
  case CreateTextNode:
    result = getDOMNode(doc.createTextNode(s));
    break;
  case CreateComment:
    result = getDOMNode(doc.createComment(s));
    break;
  case CreateCDATASection:
    result = getDOMNode(doc.createCDATASection(s));  /* TODO: okay ? */
    break;
  case CreateProcessingInstruction:
    result = getDOMNode(doc.createProcessingInstruction(args[0].toString().value().string(),
                                                                 args[1].toString().value().string()));
    break;
  case CreateAttribute:
    result = getDOMNode(doc.createAttribute(s));
    break;
  case CreateEntityReference:
    result = getDOMNode(doc.createEntityReference(args[0].toString().value().string()));
    break;
  case GetElementsByTagName:
    result = getDOMNodeList(doc.getElementsByTagName(s));
    break;
    /* TODO */
//  case ImportNode: // new for DOM2 - not yet in khtml
  case CreateElementNS: // new for DOM2
    result = getDOMNode(doc.createElementNS(args[0].toString().value().string(),args[1].toString().value().string()));
    break;
  case CreateAttributeNS: // new for DOM2
    result = getDOMNode(doc.createAttributeNS(args[0].toString().value().string(),args[1].toString().value().string()));
    break;
/*  case GetElementsByTagNameNS: // new for DOM2 - not yet in khtml
  case GetElementById: // new for DOM2 - not yet in khtml*/
  default:
    result = Undefined();
  }

  return Completion(Normal, result);
}

// -------------------------------------------------------------------------

const TypeInfo DOMElement::info = { "Element", HostType,
				    &DOMNode::info, 0, 0 };

KJSO DOMElement::tryGet(const UString &p) const
{
  DOM::Element element = static_cast<DOM::Element>(node);

  if (p == "tagName")
    return getString(element.tagName());
  else if (p == "getAttribute")
    return new DOMElementFunction(element, DOMElementFunction::GetAttribute);
  else if (p == "setAttribute")
    return new DOMElementFunction(element, DOMElementFunction::SetAttribute);
  else if (p == "removeAttribute")
    return new DOMElementFunction(element, DOMElementFunction::RemoveAttribute);
  else if (p == "getAttributeNode")
    return new DOMElementFunction(element, DOMElementFunction::GetAttributeNode);
  else if (p == "setAttributeNode")
    return new DOMElementFunction(element, DOMElementFunction::SetAttributeNode);
  else if (p == "removeAttributeNode")
    return new DOMElementFunction(element, DOMElementFunction::RemoveAttributeNode);
  else if (p == "getElementsByTagName")
    return new DOMElementFunction(element, DOMElementFunction::GetElementsByTagName);
  else if (p == "normalize") // this is moved to Node in DOM2
    return new DOMElementFunction(element, DOMElementFunction::Normalize);
  else if (p == "style")
//    result = KJSO(new Style(node));
    return getDOMCSSStyleDeclaration(element.style());
//    return new DOMCSSStyleDeclaration(element.style()));
/*  else if (p == "getAttributeNS") // new for DOM2 - not yet in khtml
    return new DOMElementFunction(element, DOMElementFunction::GetAttributeNS);
  else if (p == "setAttributeNS") // new for DOM2 - not yet in khtml
    return new DOMElementFunction(element, DOMElementFunction::SetAttributeNS);
  else if (p == "removeAttributeNS") // new for DOM2 - not yet in khtml
    return new DOMElementFunction(element, DOMElementFunction::RemoveAttributeNS);
  else if (p == "getAttributeNodeNS") // new for DOM2 - not yet in khtml
    return new DOMElementFunction(element, DOMElementFunction::GetAttributeNodeNS);
  else if (p == "setAttributeNodeNS") // new for DOM2 - not yet in khtml
    return new DOMElementFunction(element, DOMElementFunction::SetAttributeNodeNS);
  else if (p == "getElementsByTagNameNS") // new for DOM2 - not yet in khtml
    return new DOMElementFunction(element, DOMElementFunction::GetElementsByTagNameNS);
  else if (p == "hasAttribute") // new for DOM2 - not yet in khtml
    return new DOMElementFunction(element, DOMElementFunction::HasAttribute);
  else if (p == "hasAttributeNS") // new for DOM2 - not yet in khtml
    return new DOMElementFunction(element, DOMElementFunction::HasAttributeNS);*/
  else
    return DOMNode::tryGet(p);
}


DOMElementFunction::DOMElementFunction(DOM::Element e, int i)
  : element(e), id(i)
{
}

Completion DOMElementFunction::tryExecute(const List &args)
{
  KJSO result;

  switch(id) {
    case GetAttribute:
      result = String(element.getAttribute(args[0].toString().value().string()));
      break;
    case SetAttribute:
      element.setAttribute(args[0].toString().value().string(),args[1].toString().value().string());
      result = Undefined();
      break;
    case RemoveAttribute:
      element.removeAttribute(args[0].toString().value().string());
      result = Undefined();
      break;
    case GetAttributeNode:
      result = getDOMNode(element.getAttributeNode(args[0].toString().value().string()));
      break;
    case SetAttributeNode:
      result = getDOMNode(element.setAttributeNode((new DOMNode(KJS::toNode(args[0])))->toNode()));
      break;
    case RemoveAttributeNode:
      result = getDOMNode(element.removeAttributeNode((new DOMNode(KJS::toNode(args[0])))->toNode()));
      break;
    case GetElementsByTagName:
      result = getDOMNodeList(element.getElementsByTagName(args[0].toString().value().string()));
      break;
    case Normalize: {  // this is moved to Node in DOM2
        element.normalize();
        result = Undefined();
      }
      break;
/*    case GetAttributeNS: // new for DOM2 - not yet in khtml
    case SetAttributeNS: // new for DOM2 - not yet in khtml
    case RemoveAttributeNS: // new for DOM2 - not yet in khtml
    case GetAttributeNodeNS: // new for DOM2 - not yet in khtml
    case SetAttributeNodeNS: // new for DOM2 - not yet in khtml
    case GetElementsByTagNameNS: // new for DOM2 - not yet in khtml
    case HasAttribute: // new for DOM2 - not yet in khtml
    case HasAttributeNS: // new for DOM2 - not yet in khtml*/
  default:
    result = Undefined();
  }

  return Completion(Normal, result);
}

// -------------------------------------------------------------------------

const TypeInfo DOMDOMImplementation::info = { "DOMImplementation", HostType, 0, 0, 0 };

DOMDOMImplementation::~DOMDOMImplementation()
{
  domImplementations.remove(implementation.handle());
}

KJSO DOMDOMImplementation::tryGet(const UString &p) const
{
  if (p == "hasFeature")
    return new DOMDOMImplementationFunction(implementation, DOMDOMImplementationFunction::HasFeature);
//  else if (p == "createDocumentType") // new for DOM2 - not yet in khtml
//    return new DOMDOMImplementationFunction(implementation, DOMDOMImplementationFunction::CreateDocumentType);
//  else if (p == "createDocument") // new for DOM2 - not yet in khtml
//    return new DOMDOMImplementationFunction(implementation, DOMDOMImplementationFunction::CreateDocument);
  else
    return HostImp::get(p);
}

DOMDOMImplementationFunction::DOMDOMImplementationFunction(DOM::DOMImplementation impl, int i)
  : implementation(impl), id(i)
{
}

Completion DOMDOMImplementationFunction::tryExecute(const List &args)
{
  KJSO result;

  switch(id) {
    case HasFeature:
      result = Boolean(implementation.hasFeature(args[0].toString().value().string(),args[1].toString().value().string()));
      break;
/*    case CreateDocumentType: // new for DOM2 - not yet in khtml
    case CreateDocument: // new for DOM2 - not yet in khtml*/
    default:
      result = Undefined();
  }

  return Completion(Normal, result);
}

// -------------------------------------------------------------------------

const TypeInfo DOMDocumentType::info = { "DocumentType", HostType, &DOMNode::info, 0, 0 };

KJSO DOMDocumentType::tryGet(const UString &p) const
{
  DOM::DocumentType type = static_cast<DOM::DocumentType>(node);

  if (p == "name")
    return getString(type.name());
  else if (p == "entities")
    return getDOMNamedNodeMap(type.entities());
  else if (p == "notations")
    return getDOMNamedNodeMap(type.notations());
//  else if (p == "publicId") // new for DOM2 - not yet in khtml
//    return getString(type.publicId());
//  else if (p == "systemId") // new for DOM2 - not yet in khtml
//    return getString(type.systemId());
//  else if (p == "internalSubset") // new for DOM2 - not yet in khtml
//    return getString(type.internalSubset());
  else
    return DOMNode::tryGet(p);
}

// -------------------------------------------------------------------------

const TypeInfo DOMNamedNodeMap::info = { "NamedNodeMap", HostType, 0, 0, 0 };

DOMNamedNodeMap::~DOMNamedNodeMap()
{
  namedNodeMaps.remove(map.handle());
}

KJSO DOMNamedNodeMap::tryGet(const UString &p) const
{
  KJSO result;

  if (p == "length")
    return Number(map.length());
  else if (p == "getNamedItem")
    result = new DOMNamedNodeMapFunction(map, DOMNamedNodeMapFunction::GetNamedItem);
  else if (p == "setNamedItem")
    result = new DOMNamedNodeMapFunction(map, DOMNamedNodeMapFunction::SetNamedItem);
  else if (p == "removeNamedItem")
    result = new DOMNamedNodeMapFunction(map, DOMNamedNodeMapFunction::RemoveNamedItem);
  else if (p == "item")
    result = new DOMNamedNodeMapFunction(map, DOMNamedNodeMapFunction::Item);
//  else if (p == "getNamedItemNS") // new for DOM2 - not yet in khtml
//    result = new DOMNamedNodeMapFunction(map, DOMNamedNodeMapFunction::GetNamedItemNS);
//  else if (p == "setNamedItemNS") // new for DOM2 - not yet in khtml
//    result = new DOMNamedNodeMapFunction(map, DOMNamedNodeMapFunction::SetNamedItemNS);
//  else if (p == "removeNamedItemNS") // new for DOM2 - not yet in khtml
//    result = new DOMNamedNodeMapFunction(map, DOMNamedNodeMapFunction::RemoveNamedItemNS);
  else
    result = Undefined();

  // array index ?
  bool ok;
  long unsigned int idx = p.toULong(&ok);
  if (ok)
    result = getDOMNode(map.item(idx));

  return result;
}

DOMNamedNodeMapFunction::DOMNamedNodeMapFunction(DOM::NamedNodeMap m, int i)
  : map(m), id(i)
{
}

Completion DOMNamedNodeMapFunction::tryExecute(const List &args)
{
  KJSO result;

  switch(id) {
    case GetNamedItem:
      result = getDOMNode(map.getNamedItem(args[0].toString().value().string()));
      break;
    case SetNamedItem:
      result = getDOMNode(map.setNamedItem((new DOMNode(KJS::toNode(args[0])))->toNode()));
      break;
    case RemoveNamedItem:
      result = getDOMNode(map.removeNamedItem(args[0].toString().value().string()));
      break;
    case Item:
      result = getDOMNode(map.item(args[0].toNumber().intValue()));
      break;
/*    case GetNamedItemNS: // new for DOM2 - not yet in khtml
    case SetNamedItemNS: // new for DOM2 - not yet in khtml
    case RemoveNamedItemNS: // new for DOM2 - not yet in khtml*/
    default:
      result = Undefined();
  }

  return Completion(Normal, result);
}

// -------------------------------------------------------------------------

const TypeInfo DOMProcessingInstruction::info = { "ProcessingInstruction", HostType, &DOMNode::info, 0, 0 };

KJSO DOMProcessingInstruction::tryGet(const UString &p) const
{
  if (p == "target")
    return getString(static_cast<DOM::ProcessingInstruction>(node).target());
  else if (p == "data")
    return getString(static_cast<DOM::ProcessingInstruction>(node).data());
  else
    return DOMNode::tryGet(p);
}

void DOMProcessingInstruction::tryPut(const UString &p, const KJSO& v)
{
  if (p == "data")
    static_cast<DOM::ProcessingInstruction>(node).setData(v.toString().value().string());
  else
    DOMNode::tryPut(p,v);
}

// -------------------------------------------------------------------------

const TypeInfo DOMNotation::info = { "Notation", HostType, &DOMNode::info, 0, 0 };

KJSO DOMNotation::tryGet(const UString &p) const
{
  if (p == "publicId")
    return getString(static_cast<DOM::Notation>(node).publicId());
  else if (p == "systemId")
    return getString(static_cast<DOM::Notation>(node).systemId());
  else
    return DOMNode::tryGet(p);
}

// -------------------------------------------------------------------------

const TypeInfo DOMEntity::info = { "Entity", HostType, &DOMNode::info, 0, 0 };

KJSO DOMEntity::tryGet(const UString &p) const
{
  if (p == "publicId")
    return getString(static_cast<DOM::Entity>(node).publicId());
  else if (p == "systemId")
    return getString(static_cast<DOM::Entity>(node).systemId());
  else if (p == "notationName")
    return getString(static_cast<DOM::Entity>(node).notationName());
  else
    return DOMNode::tryGet(p);
}

// -------------------------------------------------------------------------

KJSO KJS::getDOMNode(DOM::Node n)
{
  DOMNode *ret = 0;
  if (n.isNull())
    return Null();
  else if ((ret = nodes[n.handle()]))
    return ret;

  switch (n.nodeType()) {
    case DOM::Node::ELEMENT_NODE:
      if (static_cast<DOM::Element>(n).isHTMLElement())
        ret = new HTMLElement(static_cast<DOM::HTMLElement>(n));
      else
        ret = new DOMElement(static_cast<DOM::Element>(n));
      break;
    case DOM::Node::ATTRIBUTE_NODE:
      ret = new DOMAttr(static_cast<DOM::Attr>(n));
      break;
    case DOM::Node::TEXT_NODE:
    case DOM::Node::CDATA_SECTION_NODE:
      ret = new DOMText(static_cast<DOM::Text>(n));
      break;
    case DOM::Node::ENTITY_REFERENCE_NODE:
      ret = new DOMNode(n);
      break;
    case DOM::Node::ENTITY_NODE:
      ret = new DOMEntity(static_cast<DOM::Entity>(n));
      break;
    case DOM::Node::PROCESSING_INSTRUCTION_NODE:
      ret = new DOMProcessingInstruction(static_cast<DOM::ProcessingInstruction>(n));
      break;
    case DOM::Node::COMMENT_NODE:
      ret = new DOMCharacterData(static_cast<DOM::CharacterData>(n));
      break;
    case DOM::Node::DOCUMENT_NODE:
      if (static_cast<DOM::Document>(n).isHTMLDocument())
        ret = new HTMLDocument(static_cast<DOM::HTMLDocument>(n));
      else
        ret = new DOMDocument(static_cast<DOM::Document>(n));
      break;
    case DOM::Node::DOCUMENT_TYPE_NODE:
      ret = new DOMDocumentType(static_cast<DOM::DocumentType>(n));
      break;
    case DOM::Node::DOCUMENT_FRAGMENT_NODE:
      ret = new DOMNode(n);
      break;
    case DOM::Node::NOTATION_NODE:
      ret = new DOMNotation(static_cast<DOM::Notation>(n));
      break;
    default:
      ret = new DOMNode(n);
  }
  nodes.insert(n.handle(),ret);

  return ret;
}

KJSO KJS::getDOMNamedNodeMap(DOM::NamedNodeMap m)
{
  DOMNamedNodeMap *ret;
  if (m.isNull())
    return Null();
  else if ((ret = namedNodeMaps[m.handle()]))
    return ret;
  else {
    ret = new DOMNamedNodeMap(m);
    namedNodeMaps.insert(m.handle(),ret);
    return ret;
  }
}

KJSO KJS::getDOMNodeList(DOM::NodeList l)
{
  DOMNodeList *ret;
  if (l.isNull())
    return Null();
  else if ((ret = nodeLists[l.handle()]))
    return ret;
  else {
    ret = new DOMNodeList(l);
    nodeLists.insert(l.handle(),ret);
    return ret;
  }
}

KJSO KJS::getDOMDOMImplementation(DOM::DOMImplementation i)
{
  DOMDOMImplementation *ret;
  if (i.isNull())
    return Null();
  else if ((ret = domImplementations[i.handle()]))
    return ret;
  else {
    ret = new DOMDOMImplementation(i);
    domImplementations.insert(i.handle(),ret);
    return ret;
  }
}

// -------------------------------------------------------------------------

const TypeInfo NodePrototype::info = { "NodePrototype", HostType, 0, 0, 0 };

KJSO NodePrototype::tryGet(const UString &p) const
{
  if (p == "ELEMENT_NODE")
    return Number((unsigned int)DOM::Node::ELEMENT_NODE);
  if (p == "ATTRIBUTE_NODE")
    return Number((unsigned int)DOM::Node::ATTRIBUTE_NODE);
  if (p == "TEXT_NODE")
    return Number((unsigned int)DOM::Node::TEXT_NODE);
  if (p == "CDATA_SECTION_NODE")
    return Number((unsigned int)DOM::Node::CDATA_SECTION_NODE);
  if (p == "ENTITY_REFERENCE_NODE")
    return Number((unsigned int)DOM::Node::ENTITY_REFERENCE_NODE);
  if (p == "ENTITY_NODE")
    return Number((unsigned int)DOM::Node::ENTITY_NODE);
  if (p == "PROCESSING_INSTRUCTION_NODE")
    return Number((unsigned int)DOM::Node::PROCESSING_INSTRUCTION_NODE);
  if (p == "COMMENT_NODE")
    return Number((unsigned int)DOM::Node::COMMENT_NODE);
  if (p == "DOCUMENT_NODE")
    return Number((unsigned int)DOM::Node::DOCUMENT_NODE);
  if (p == "DOCUMENT_TYPE_NODE")
    return Number((unsigned int)DOM::Node::DOCUMENT_TYPE_NODE);
  if (p == "DOCUMENT_FRAGMENT_NODE")
    return Number((unsigned int)DOM::Node::DOCUMENT_FRAGMENT_NODE);
  if (p == "NOTATION_NODE")
    return Number((unsigned int)DOM::Node::NOTATION_NODE);

  return KJSO();
}

KJSO KJS::getNodePrototype()
{
    KJSO proto = Global::current().get("[[node.prototype]]");
    if (proto.isDefined())
        return proto;
    else
    {
        Object nodeProto( new NodePrototype );
        Global::current().put("[[node.prototype]]", nodeProto);
        return nodeProto;
    }
}
