/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
   Copyright (C) 2000 CodeFactory AB
   Copyright (C) 2000 Jonas Borgstr\366m <jonas@codefactory.se>
   Copyright (C) 2000 Anders Carlsson <andersca@codefactory.se>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "dom-core.h"

/**
 * dom_DOMImplementation__hasFeature: Test if the DOM implementation implements a specific feature
 * @self: The DOM implementation.
 * @feature: The name of the feature to test (case-insensitive).
 * @version: The version number of the feature to test.
 * @exc: Exception.
 * 
 * 
 * 
 * Return value: Returns TRUE if the feature is implemented or FALSE otherwise.
 **/
DomBoolean
dom_DOMImplementation__hasFeature (DomDOMImplementation *self, DomString *feature, DomString *version, DomException *exc)
{
  *exc = 0;
  return self->vtab->hasFeature (self, feature, version, exc);
}

DomDocumentType *
dom_DOMImplementation__createDocumentType (DomDOMImplementation *self, DomString *qualifiedName, DomString *publicId, DomString *systemId, DomException *exc)
{
  *exc = 0;
  return self->vtab->createDocumentType (self, qualifiedName, publicId, systemId, exc);
}

DomDocument *
dom_DOMImplementation__createDocument (DomDOMImplementation *self, DomString *namespaceURI, DomString *qualifiedName, DomDocumentType *doctype, DomException *exc)
{
  *exc = 0;
  return self->vtab->createDocument (self, namespaceURI, qualifiedName, doctype, exc);
}


DomNode *
dom_Node__insertBefore (DomNode *self, DomNode *newChild, DomNode *refChild, DomException *exc)
{
	*exc = 0;

	return self->vtab->insertBefore (self, newChild, refChild, exc);
}

void
dom_Node__set_nodeValue (DomNode *self, DomString *nodeValue, DomException *exc)
{
  *exc = 0;
  self->vtab->set_nodeValue (self, nodeValue, exc);
}

DomNode *
dom_Node__get_firstChild (DomNode *self, DomException *exc)
{
  *exc = 0;
  return self->vtab->get_firstChild (self, exc);
}

void
dom_Node__ref (DomNode *self, DomException *exc)
{
	self->vtab->ref (self, exc);
}

void
dom_Node__unref (DomNode *self, DomException *exc)
{
	self->vtab->unref (self, exc);
}

DomString *
dom_Node__get_nodeName (DomNode *self, DomException *exc)
{
	return self->vtab->get_nodeName (self, exc);
}

DomNode *
dom_Node__appendChild (DomNode *self, DomNode *newChild, DomException *exc)
{
	return self->vtab->appendChild (self, newChild, exc);
}

DomNode *
dom_Node__removeChild (DomNode *self, DomNode *oldChild, DomException *exc)
{
	return self->vtab->removeChild (self, oldChild, exc);
}

DomDocument *
dom_Node__get_ownerDocument (DomNode *self, DomException *exc)
{
	*exc = 0;
	return self->vtab->get_ownerDocument (self, exc);
}

void
dom_CharacterData__ref (DomCharacterData *self, DomException *exc)
{
	self->vtab->super.ref ((DomNode *)self, exc);
}

DomDocumentType *
dom_Document__get_doctype (DomDocument *self, DomException *exc)
{
  *exc = 0;
  return self->vtab->get_doctype (self, exc);
}

DomDOMImplementation *
dom_Document__get_implementation (DomDocument *self, DomException *exc)
{
  *exc = 0;
  return self->vtab->get_implementation (self, exc);
}

DomElement *
dom_Document__get_documentElement (DomDocument *self, DomException *exc)
{
  *exc = 0;
  return self->vtab->get_documentElement (self, exc);
}

DomElement *
dom_Document__createElement (DomDocument *self, DomString *tagName, DomException *exc)
{
  *exc = 0;
  return self->vtab->createElement (self, tagName, exc);
}

DomDocumentFragment *
dom_Document__createDocumentFragment (DomDocument *self, DomException *exc)
{
  *exc = 0;
  return self->vtab->createDocumentFragment (self, exc);
}

DomText *
dom_Document__createTextNode (DomDocument *self, DomString *data, DomException *exc)
{
  *exc = 0;
  return self->vtab->createTextNode (self, data, exc);
}

DomComment *
dom_Document__createComment (DomDocument *self, DomString *data, DomException *exc)
{
  *exc = 0;
  return self->vtab->createComment (self, data, exc);
}

DomCDATASection *
dom_Document__createCDATASection (DomDocument *self, DomString *data, DomException *exc)
{
  *exc = 0;
  return self->vtab->createCDATASection (self, data, exc);
}

DomProcessingInstruction *
dom_Document__createProcessingInstruction (DomDocument *self, DomString *target, DomString *data, DomException *exc)
{
  *exc = 0;
  return self->vtab->createProcessingInstruction (self, target, data, exc);
}

DomAttr *
dom_Document__createAttribute (DomDocument *self, DomString *name, DomException *exc)
{
  *exc = 0;
  return self->vtab->createAttribute (self, name, exc);
}

DomEntityReference *
dom_Document__createEntityReference (DomDocument *self, DomString *name, DomException *exc)
{
  *exc = 0;
  return self->vtab->createEntityReference (self, name, exc);
}

DomNodeList *
dom_Document__getElementsByTagName (DomDocument *self, DomString *tagname, DomException *exc)
{
  *exc = 0;
  return self->vtab->getElementsByTagName (self, tagname, exc);
}

DomNode *
dom_Document__importNode (DomDocument *self, DomNode *importedNode, DomBoolean deep, DomException *exc)
{
  *exc = 0;
  return self->vtab->importNode (self, importedNode, deep, exc);
}

DomElement *
dom_Document__createElementNS (DomDocument *self, DomString *namespaceURI, DomString *qualifiedName, DomException *exc)
{
  *exc = 0;
  return self->vtab->createElementNS (self, namespaceURI, qualifiedName, exc);
}

DomAttr *
dom_Document__createAttributeNS (DomDocument *self, DomString *namespaceURI, DomString *qualifiedName, DomException *exc)
{
  *exc = 0;
  return self->vtab->createAttributeNS (self, namespaceURI, qualifiedName, exc);
}

DomNodeList *
dom_Document__getElementsByTagNameNS (DomDocument *self, DomString *namespaceURI, DomString *localName, DomException *exc)
{
  *exc = 0;
  return self->vtab->getElementsByTagNameNS (self, namespaceURI, localName, exc);
}

DomElement *
dom_Document__getElementById (DomDocument *self, DomString *elementId, DomException *exc)
{
  *exc = 0;
  return self->vtab->getElementById (self, elementId, exc);
}

DomString *
dom_Element__get_tagName (DomElement *self, DomException *exc)
{
  *exc = 0;
  return self->vtab->get_tagName (self, exc);
}

DomString *
dom_Element__getAttribute (DomElement *self, DomString *name, DomException *exc)
{
  *exc = 0;
  return self->vtab->getAttribute (self, name, exc);
}

void
dom_Element__setAttribute (DomElement *self, DomString *name, DomString *value, DomException *exc)
{
  *exc = 0;
  self->vtab->setAttribute (self, name, value, exc);
}

void
dom_Element__removeAttribute (DomElement *self, DomString *name, DomException *exc)
{
  *exc = 0;
  self->vtab->removeAttribute (self, name, exc);
}

DomAttr *
dom_Element__getAttributeNode (DomElement *self, DomString *name, DomException *exc)
{
  *exc = 0;
  return self->vtab->getAttributeNode (self, name, exc);
}

DomAttr *
dom_Element__setAttributeNode (DomElement *self, DomAttr *newAttr, DomException *exc)
{
  *exc = 0;
  return self->vtab->setAttributeNode (self, newAttr, exc);
}

DomAttr *
dom_Element__removeAttributeNode (DomElement *self, DomAttr *oldAttr, DomException *exc)
{
  *exc = 0;
  return self->vtab->removeAttributeNode (self, oldAttr, exc);
}

DomNodeList *
dom_Element__getElementsByTagName (DomElement *self, DomString *name, DomException *exc)
{
  *exc = 0;
  return self->vtab->getElementsByTagName (self, name, exc);
}

DomString *
dom_Element__getAttributeNS (DomElement *self, DomString *namespaceURI, DomString *localName, DomException *exc)
{
  *exc = 0;
  return self->vtab->getAttributeNS (self, namespaceURI, localName, exc);
}

void
dom_Element__setAttributeNS (DomElement *self, DomString *namespaceURI, DomString *qualifiedName, DomString *value, DomException *exc)
{
  *exc = 0;
  self->vtab->setAttributeNS (self, namespaceURI, qualifiedName, value, exc);
}

void
dom_Element__removeAttributeNS (DomElement *self, DomString *namespaceURI, DomString *localName, DomException *exc)
{
  *exc = 0;
  self->vtab->removeAttributeNS (self, namespaceURI, localName, exc);
}

DomAttr *
dom_Element__getAttributeNodeNS (DomElement *self, DomString *namespaceURI, DomString *localName, DomException *exc)
{
  *exc = 0;
  return self->vtab->getAttributeNodeNS (self, namespaceURI, localName, exc);
}

DomAttr *
dom_Element__setAttributeNodeNS (DomElement *self, DomAttr *newAttr, DomException *exc)
{
  *exc = 0;
  return self->vtab->setAttributeNodeNS (self, newAttr, exc);
}

DomNodeList *
dom_Element__getElementsByTagNameNS (DomElement *self, DomString *namespaceURI, DomString *localName, DomException *exc)
{
  *exc = 0;
  return self->vtab->getElementsByTagNameNS (self, namespaceURI, localName, exc);
}

DomBoolean
dom_Element__hasAttribute (DomElement *self, DomString *name, DomException *exc)
{
  *exc = 0;
  return self->vtab->hasAttribute (self, name, exc);
}

DomBoolean
dom_Element__hasAttributeNS (DomElement *self, DomString *namespaceURI, DomString *localName, DomException *exc)
{
  *exc = 0;
  return self->vtab->hasAttributeNS (self, namespaceURI, localName, exc);
}

void
dom_NodeList__ref (DomNodeList *self, DomException *exc)
{
  *exc = 0;
  self->vtab->ref (self, exc);
}

void
dom_NodeList__unref (DomNodeList *self, DomException *exc)
{
  *exc = 0;
  self->vtab->unref (self, exc);
}

void *
dom_NodeList__queryInterface (DomNodeList *self, const gchar *interface, DomException *exc)
{
  *exc = 0;
  return self->vtab->queryInterface (self, interface, exc);
}

DomNode *
dom_NodeList__item (DomNodeList *self, unsigned long index, DomException *exc)
{
  *exc = 0;
  return self->vtab->item (self, index, exc);
}

unsigned long
dom_NodeList__get_length (DomNodeList *self, DomException *exc)
{
  *exc = 0;
  return self->vtab->get_length (self, exc);
}
