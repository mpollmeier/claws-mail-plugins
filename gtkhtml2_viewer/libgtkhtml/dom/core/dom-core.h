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

#ifndef __DOM_CORE_H__
#define __DOM_CORE_H__

#define DOM_NODE(x) ((DomNode *)x)
#define DOM_COMMENT(x) ((DomComment *)x)
#define DOM_EVENT_TARGET(x) ((DomEventTarget *)x)
#define DOM_EVENT_LISTENER(x) ((DomEventListener *)x)

typedef struct _DomDOMImplementation DomDOMImplementation;
typedef struct _DomNode DomNode;
typedef struct _DomComment DomComment;
typedef struct _DomCDATASection DomCDATASection;
typedef struct _DomEntityReference DomEntityReference;
typedef struct _DomNodeList DomNodeList;
typedef struct _DomNamedNodeMap DomNamedNodeMap;
typedef struct _DomCharacterData DomCharacterData;
typedef struct _DomDocument DomDocument;
typedef struct _DomDocumentFragment DomDocumentFragment;
typedef struct _DomDocumentType DomDocumentType;
typedef struct _DomText DomText;
typedef struct _DomAttr DomAttr;
typedef struct _DomElement DomElement;
typedef struct _DomProcessingInstruction DomProcessingInstruction;

typedef struct _DomDOMImplementationVtab DomDOMImplementationVtab;
typedef struct _DomCommentVtab DomCommentVtab;
typedef struct _DomElementVtab DomElementVtab;
typedef struct _DomCDATASectionVtab DomCDATASectionVtab;
typedef struct _DomNodeVtab DomNodeVtab;
typedef struct _DomNodeListVtab DomNodeListVtab;
typedef struct _DomCharacterDataVtab DomCharacterDataVtab;
typedef struct _DomNamedNodeMapVtab DomNamedNodeMapVtab;
typedef struct _DomDocumentVtab DomDocumentVtab;
typedef struct _DomDocumentFragmentVtab DomDocumentFragmentVtab;
typedef struct _DomDocumentTypeVtab DomDocumentTypeVtab;
typedef struct _DomTextVtab DomTextVtab;
typedef struct _DomAttrVtab DomAttrVtab;
typedef struct _DomProcessingInstructionVtab DomProcessingInstructionVtab;
typedef struct _DomEntityReferenceVtab DomEntityReferenceVtab;

#include <libxml/tree.h>
#include "dom/dom-string.h"
#include "dom/events/dom-events.h"
#include "dom/dom-exception.h"


struct _DomDOMImplementation {
	const DomDOMImplementationVtab *vtab;
};

struct _DomNode {
	const DomNodeVtab *vtab;
};

struct _DomCDATASection {
	const DomCDATASectionVtab *vtab;
};

struct _DomEntityReference {
	const DomEntityReferenceVtab *vtab;
};

struct _DomComment {
	const DomCommentVtab *vtab;
};

struct _DomNodeList {
	const DomNodeListVtab *vtab;
};

struct _DomNamedNodeMap {
	const DomNamedNodeMapVtab *vtab;
};

struct _DomCharacterData {
	const DomCharacterDataVtab *vtab;
};

struct _DomDocument {
	const DomDocumentVtab *vtab;
};

struct _DomDocumentFragment {
	const DomDocumentFragmentVtab *vtab;
};

struct _DomDocumentType {
	const DomDocumentTypeVtab *vtab;
};

struct _DomElement {
  const DomElementVtab *vtab;
};

struct _DomText {
	const DomTextVtab *vtab;
};

struct _DomAttr {
	const DomAttrVtab *vtab;
};

struct _DomProcessingInstruction {
	const DomProcessingInstructionVtab *vtab;
};

struct _DomNodeVtab {
	void              (*ref)                 (DomNode *self, DomException *exc);
	void              (*unref)               (DomNode *self, DomException *exc);
	void *            (*queryInterface)      (DomNode *self, DomString *str, DomException *exc);

	DomString *       (*get_nodeName)        (DomNode *self, DomException *exc);
	DomString *       (*get_nodeValue)       (DomNode *self, DomException *exc);
	void              (*set_nodeValue)       (DomNode *self, DomString *nodeValue, DomException *exc);
	gushort           (*get_nodeType)        (DomNode *self, DomException *exc);
	DomNode *         (*get_parentNode)      (DomNode *self, DomException *exc);
	DomNodeList *     (*get_childNodes)      (DomNode *self, DomException *exc);
	DomNode *         (*get_firstChild)      (DomNode *self, DomException *exc);
	DomNode *         (*get_lastChild)       (DomNode *self, DomException *exc);
	DomNode *         (*get_previousSibling) (DomNode *self, DomException *exc);
	DomNode *         (*get_nextSibling)     (DomNode *self, DomException *exc);
	DomNamedNodeMap * (*get_attributes)      (DomNode *self, DomException *exc);
	DomDocument *     (*get_ownerDocument)   (DomNode *self, DomException *exc);
	DomNode *         (*insertBefore)        (DomNode *self, DomNode *newChild, 
						  DomNode *refChild, DomException *exc);
	DomNode *         (*replaceChild)        (DomNode *self, DomNode *newChild, 
						  DomNode *oldChild, DomException *exc);
	DomNode *         (*removeChild)         (DomNode *self, DomNode *oldChild, 
						  DomException *exc);
	DomNode *         (*appendChild)         (DomNode *self, DomNode *newChild, 
						  DomException *exc);
	DomBoolean        (*hasChildNodes)       (DomNode *self, DomException *exc);
	DomNode *         (*cloneNode)           (DomNode *self, DomBoolean deep,
						  DomException *exc);
	void              (*normalize)           (DomNode *self, DomException *exc);
	DomBoolean        (*isSupported)         (DomNode *self, DomString *feature, 
						  DomString *version, DomException *exc);
	DomString *       (*get_namespaceURI)    (DomNode *self, DomException *exc);
	DomString *       (*get_prefix)          (DomNode *self, DomException *exc);
	void              (*set_prefix)          (DomNode *self, DomString *prefix, 
						  DomException *exc);
	DomString *       (*get_localName)       (DomNode *self, DomException *exc);
	DomBoolean        (*hasAttributes)       (DomNode *self, DomException *exc);

	/* From the EventTarget interface */
	void              (*addEventListener)    (DomEventTarget *self, DomString *type, DomEventListener *listener, DomBoolean useCapture, DomException *exc);
        void              (*removeEventListener) (DomEventTarget *self, DomString *type, DomEventListener *listener, DomBoolean useCapture, DomException *exc);
	DomBoolean        (*dispatchEvent) (DomEventTarget *self, DomEvent *evt, DomException *exc);
};

struct _DomNodeListVtab {
	void (*ref) (DomNodeList *self, DomException *exc);
	void (*unref) (DomNodeList *self, DomException *exc);
	void * (*queryInterface) (DomNodeList *self, DomString *str, DomException *exc);

	DomNode *(*item) (DomNodeList *self, gulong index, DomException *exc);
	gulong (*get_length) (DomNodeList *self, DomException *exc);
	
};

struct _DomNamedNodeMapVtab {
	void (*ref) (DomNamedNodeMap *self, DomException *exc);
	void (*unref) (DomNamedNodeMap *self, DomException *exc);
	void * (*queryInterface) (DomNamedNodeMap *self, const char *interface, DomException *exc);
	DomNode *(*getNamedItem) (DomNamedNodeMap *self, DomString *name, DomException *exc);
	DomNode *(*setNamedItem) (DomNamedNodeMap *self, DomNode *arg, DomException *exc);
	DomNode *(*removeNamedItem) (DomNamedNodeMap *self, DomString *name, DomException *exc);
	DomNode *(*item) (DomNamedNodeMap *self, gulong index, DomException *exc);
	gulong (*get_length) (DomNamedNodeMap *self, DomException *exc);
};

struct _DomDocumentVtab {
	DomNodeVtab super;
	
	DomDocumentType *(*get_doctype) (DomDocument *self, DomException *exc);
	DomDOMImplementation *(*get_implementation) (DomDocument *self, DomException *exc);
	DomElement *(*get_documentElement) (DomDocument *self, DomException *exc);
	DomElement *(*createElement) (DomDocument *self, DomString *tagName, DomException *exc);
	DomDocumentFragment *(*createDocumentFragment) (DomDocument *self, DomException *exc);
	DomText *(*createTextNode) (DomDocument *self, DomString *data, DomException *exc);
	DomComment *(*createComment) (DomDocument *self, DomString *data, DomException *exc);
	DomCDATASection *(*createCDATASection) (DomDocument *self, DomString *data, DomException *exc);
	DomProcessingInstruction *(*createProcessingInstruction) (DomDocument *self, DomString *target, DomString *data, DomException *exc);
	DomAttr *(*createAttribute) (DomDocument *self, DomString *name, DomException *exc);
	DomEntityReference *(*createEntityReference) (DomDocument *self, DomString *name, DomException *exc);
	DomNodeList *(*getElementsByTagName) (DomDocument *self, DomString *tagname, DomException *exc);
	DomNode *(*importNode) (DomDocument *self, DomNode *importedNode, DomBoolean deep, DomException *exc);
	DomElement *(*createElementNS) (DomDocument *self, DomString *namespaceURI, DomString *qualifiedName, DomException *exc);
	DomAttr *(*createAttributeNS) (DomDocument *self, DomString *namespaceURI, DomString *qualifiedName, DomException *exc);
	DomNodeList *(*getElementsByTagNameNS) (DomDocument *self, DomString *namespaceURI, DomString *localName, DomException *exc);
	DomElement *(*getElementById) (DomDocument *self, DomString *elementId, DomException *exc);

	/* From the DocumentEvent interface */
	DomEvent *(*createEvent) (DomDocument *self, DomString *eventType, DomException *exc);
};

struct _DomDocumentFragmentVtab {
	DomNodeVtab super;
};

struct _DomEntityReferenceVtab {
	DomNodeVtab super;
};

struct _DomDocumentTypeVtab {
	DomNodeVtab super;
	DomString *(*get_name) (DomDocumentType *self, DomException *exc);
	DomNamedNodeMap *(*get_entities) (DomDocumentType *self, DomException *exc);
	DomNamedNodeMap *(*get_notations) (DomDocumentType *self, DomException *exc);
	DomString *(*get_publicId) (DomDocumentType *self, DomException *exc);
	DomString *(*get_systemId) (DomDocumentType *self, DomException *exc);
	DomString *(*get_internalSubset) (DomDocumentType *self, DomException *exc);
};

struct _DomElementVtab {
	DomNodeVtab super;
	DomString *(*get_tagName) (DomElement *self, DomException *exc);
	DomString *(*getAttribute) (DomElement *self, DomString *name, DomException *exc);
	void (*setAttribute) (DomElement *self, DomString *name, DomString *value, DomException *exc);
	void (*removeAttribute) (DomElement *self, DomString *name, DomException *exc);
	DomAttr *(*getAttributeNode) (DomElement *self, DomString *name, DomException *exc);
	DomAttr *(*setAttributeNode) (DomElement *self, DomAttr *newAttr, DomException *exc);
	DomAttr *(*removeAttributeNode) (DomElement *self, DomAttr *oldAttr, DomException *exc);
	DomNodeList *(*getElementsByTagName) (DomElement *self, DomString *name, DomException *exc);
	DomString *(*getAttributeNS) (DomElement *self, DomString *namespaceURI, DomString *localName, DomException *exc);
	void (*setAttributeNS) (DomElement *self, DomString *namespaceURI, DomString *qualifiedName, DomString *value, DomException *exc);
	void (*removeAttributeNS) (DomElement *self, DomString *namespaceURI, DomString *localName, DomException *exc);
	DomAttr *(*getAttributeNodeNS) (DomElement *self, DomString *namespaceURI, DomString *localName, DomException *exc);
	DomAttr *(*setAttributeNodeNS) (DomElement *self, DomAttr *newAttr, DomException *exc);
	DomNodeList *(*getElementsByTagNameNS) (DomElement *self, DomString *namespaceURI, DomString *localName, DomException *exc);
	DomBoolean (*hasAttribute) (DomElement *self, DomString *name, DomException *exc);
	DomBoolean (*hasAttributeNS) (DomElement *self, DomString *namespaceURI, DomString *localName, DomException *exc);
};


struct _DomCharacterDataVtab {
	DomNodeVtab super;

	DomString *(*get_data) (DomCharacterData *self, DomException *exc);
	void (*set_data) (DomCharacterData *self, DomString *data, DomException *exc);
	gulong (*get_length) (DomCharacterData *self, DomException *exc);
	DomString *(*substringData) (DomCharacterData *self, gulong offset, gulong count, DomException *exc);
	void (*appendData) (DomCharacterData *self, DomString *arg, DomException *exc);
	void (*insertData) (DomCharacterData *self, gulong offset, DomString *arg, DomException *exc);
	void (*deleteData) (DomCharacterData *self, gulong offset, gulong count, DomException *exc);
	void (*replaceData) (DomCharacterData *self, gulong offset, gulong count, DomString *arg, DomException *exc);
};

struct _DomTextVtab {
	DomCharacterDataVtab super;
	
	DomText *(*splitText) (DomText *self, gulong offset, DomException *exc);
};

struct _DomCDATASectionVtab {
	DomTextVtab super;
};


struct _DomCommentVtab {
	DomCharacterDataVtab super;
};

struct _DomAttrVtab {
	DomNodeVtab super;

	DomString *(*get_name) (DomAttr *self, DomException *exc);
	DomBoolean (*get_specified) (DomAttr *self, DomException *exc);
	DomString *(*get_value) (DomAttr *self, DomException *exc);
	void (*set_value) (DomAttr *self, DomString *value, DomException *exc);

	DomElement * (*get_element) (DomAttr *self, DomException *exc);
};

struct _DomDOMImplementationVtab {
  void (*ref) (DomDOMImplementation *self, DomException *exc);
  void (*unref) (DomDOMImplementation *self, DomException *exc);
  void * (*queryInterface) (DomDOMImplementation *self, const gchar *interface, DomException *exc);
  DomBoolean (*hasFeature) (DomDOMImplementation *self, DomString *feature, DomString *version, DomException *exc);
  DomDocumentType *(*createDocumentType) (DomDOMImplementation *self, DomString *qualifiedName, DomString *publicId, DomString *systemId, DomException *exc);
  DomDocument *(*createDocument) (DomDOMImplementation *self, DomString *namespaceURI, DomString *qualifiedName, DomDocumentType *doctype, DomException *exc);
};

struct _DomProcessingInstructionVtab {
	DomNodeVtab super;
	DomString *(*get_target) (DomProcessingInstruction *self, DomException *exc);
	DomString *(*get_data) (DomProcessingInstruction *self, DomException *exc);
	void (*set_data) (DomProcessingInstruction *self, DomString *data, DomException *exc);
};

void dom_NamedNodeMap__ref (DomNamedNodeMap *self, DomException *exc);
void dom_NamedNodeMap__unref (DomNamedNodeMap *self, DomException *exc);
void * dom_NamedNodeMap__queryInterface (DomNamedNodeMap *self, const gchar *interface, DomException *exc);
DomNode *dom_NamedNodeMap__getNamedItem (DomNamedNodeMap *self, DomString *name, DomException *exc);
DomNode *dom_NamedNodeMap__setNamedItem (DomNamedNodeMap *self, DomNode *arg, DomException *exc);
DomNode *dom_NamedNodeMap__removeNamedItem (DomNamedNodeMap *self, DomString *name, DomException *exc);
DomNode *dom_NamedNodeMap__item (DomNamedNodeMap *self, unsigned long index, DomException *exc);
unsigned long dom_NamedNodeMap__get_length (DomNamedNodeMap *self, DomException *exc);
DomNode *dom_NamedNodeMap__getNamedItemNS (DomNamedNodeMap *self, DomString *namespaceURI, DomString *localName, DomException *exc);
DomNode *dom_NamedNodeMap__setNamedItemNS (DomNamedNodeMap *self, DomNode *arg, DomException *exc);
DomNode *dom_NamedNodeMap__removeNamedItemNS (DomNamedNodeMap *self, DomString *namespaceURI, DomString *localName, DomException *exc);

void dom_Node__ref (DomNode *self, DomException *exc);
void dom_Node__unref (DomNode *self, DomException *exc);
void *dom_Node__queryInterface (DomNode *self, DomString *str, DomException *exc);
DomString *dom_Node__get_nodeName (DomNode *self, DomException *exc);
DomNode *dom_Node__removeChild (DomNode *self, DomNode *oldChild, DomException *exc);
DomDocument *dom_Node__get_ownerDocument (DomNode *self, DomException *exc);
DomNode *dom_Node__appendChild (DomNode *self, DomNode *newChild, DomException *exc);
DomNode *dom_Node__removeChild (DomNode *self, DomNode *oldChild, DomException *exc);
DomNode *dom_Node__get_firstChild (DomNode *self, DomException *exc);
void dom_Node__set_nodeValue (DomNode *self, DomString *nodeValue, DomException *exc);



void dom_CharacterData__ref (DomCharacterData *self, DomException *exc);

DomBoolean dom_DOMImplementation__hasFeature (DomDOMImplementation *self, DomString *feature, DomString *version, DomException *exc);
DomDocumentType *dom_DOMImplementation__createDocumentType (DomDOMImplementation *self, DomString *qualifiedName, DomString *publicId, DomString *systemId, DomException *exc);
DomDocument *dom_DOMImplementation__createDocument (DomDOMImplementation *self, DomString *namespaceURI, DomString *qualifiedName, DomDocumentType *doctype, DomException *exc);

DomString *dom_Element__get_tagName (DomElement *self, DomException *exc);
DomString *dom_Element__getAttribute (DomElement *self, DomString *name, DomException *exc);
void dom_Element__setAttribute (DomElement *self, DomString *name, DomString *value, DomException *exc);
void dom_Element__removeAttribute (DomElement *self, DomString *name, DomException *exc);
DomAttr *dom_Element__getAttributeNode (DomElement *self, DomString *name, DomException *exc);
DomAttr *dom_Element__setAttributeNode (DomElement *self, DomAttr *newAttr, DomException *exc);
DomAttr *dom_Element__removeAttributeNode (DomElement *self, DomAttr *oldAttr, DomException *exc);
DomNodeList *dom_Element__getElementsByTagName (DomElement *self, DomString *name, DomException *exc);
DomString *dom_Element__getAttributeNS (DomElement *self, DomString *namespaceURI, DomString *localName, DomException *exc);
void dom_Element__setAttributeNS (DomElement *self, DomString *namespaceURI, DomString *qualifiedName, DomString *value, DomException *exc);
void dom_Element__removeAttributeNS (DomElement *self, DomString *namespaceURI, DomString *localName, DomException *exc);
DomAttr *dom_Element__getAttributeNodeNS (DomElement *self, DomString *namespaceURI, DomString *localName, DomException *exc);
DomAttr *dom_Element__setAttributeNodeNS (DomElement *self, DomAttr *newAttr, DomException *exc);
DomNodeList *dom_Element__getElementsByTagNameNS (DomElement *self, DomString *namespaceURI, DomString *localName, DomException *exc);
DomBoolean dom_Element__hasAttribute (DomElement *self, DomString *name, DomException *exc);
DomBoolean dom_Element__hasAttributeNS (DomElement *self, DomString *namespaceURI, DomString *localName, DomException *exc);

DomDocumentType *dom_Document__get_doctype (DomDocument *self, DomException *exc);
DomDOMImplementation *dom_Document__get_implementation (DomDocument *self, DomException *exc);
DomElement *dom_Document__get_documentElement (DomDocument *self, DomException *exc);
DomElement *dom_Document__createElement (DomDocument *self, DomString *tagName, DomException *exc);
DomDocumentFragment *dom_Document__createDocumentFragment (DomDocument *self, DomException *exc);
DomText *dom_Document__createTextNode (DomDocument *self, DomString *data, DomException *exc);
DomComment *dom_Document__createComment (DomDocument *self, DomString *data, DomException *exc);
DomCDATASection *dom_Document__createCDATASection (DomDocument *self, DomString *data, DomException *exc);
DomProcessingInstruction *dom_Document__createProcessingInstruction (DomDocument *self, DomString *target, DomString *data, DomException *exc);
DomAttr *dom_Document__createAttribute (DomDocument *self, DomString *name, DomException *exc);
DomEntityReference *dom_Document__createEntityReference (DomDocument *self, DomString *name, DomException *exc);
DomNodeList *dom_Document__getElementsByTagName (DomDocument *self, DomString *tagname, DomException *exc);
DomNode *dom_Document__importNode (DomDocument *self, DomNode *importedNode, DomBoolean deep, DomException *exc);
DomElement *dom_Document__createElementNS (DomDocument *self, DomString *namespaceURI, DomString *qualifiedName, DomException *exc);
DomAttr *dom_Document__createAttributeNS (DomDocument *self, DomString *namespaceURI, DomString *qualifiedName, DomException *exc);
DomNodeList *dom_Document__getElementsByTagNameNS (DomDocument *self, DomString *namespaceURI, DomString *localName, DomException *exc);
DomElement *dom_Document__getElementById (DomDocument *self, DomString *elementId, DomException *exc);

#endif /* __DOM_CORE_H__ */



