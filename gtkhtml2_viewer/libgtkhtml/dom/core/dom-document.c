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

#include <libxml/tree.h>
#include <string.h>

#include "libgtkhtml/dom/traversal/dom-documenttraversal.h"
#include "dom-document.h"
#include "dom-element.h"
#include "dom-documenttype.h"
#include "dom-text.h"
#include "dom-comment.h"


/**
 * dom_Document_importNode:
 * @doc: a DomDocument
 * @importedNode: The node to import.
 * @deep: Whether the subtree under the specified node should be imported.
 * @exc: Return location for an exception.
 * 
 * Imports a node from another document to this document. This method creates a new copy of the source node.
 * 
 * Return value: The imported node.
 **/
DomNode *
dom_Document_importNode (DomDocument *doc, DomNode *importedNode, DomBoolean deep, DomException *exc)
{
	xmlNode *result;
	
	switch (importedNode->xmlnode->type) {
	case XML_ELEMENT_NODE:
	case XML_TEXT_NODE:
		/* Copy the node */
		result = xmlDocCopyNode (importedNode->xmlnode, (xmlDoc *)DOM_NODE (doc)->xmlnode, deep);
		break;
	default:
		DOM_SET_EXCEPTION (DOM_NOT_SUPPORTED_ERR);
		return NULL;
	}

	return dom_Node_mkref (result);
}

/**
 * dom_Document__get_documentElement: 
 * @doc: a DomDocument
 * 
 * Allows direct access to the root node of the document.
 * 
 * Return value: The root node of the document.
 **/
DomElement *
dom_Document__get_documentElement (DomDocument *doc)
{
	g_return_val_if_fail (doc != NULL, NULL);
	g_return_val_if_fail (DOM_IS_DOCUMENT (doc), NULL);
	
	return DOM_ELEMENT (dom_Node_mkref (xmlDocGetRootElement ((xmlDoc *)(DOM_NODE (doc)->xmlnode))));
}

/**
 * dom_Document_createElement: 
 * @doc: a DomDocument
 * @tagName: The name of the element 
 * 
 * Creates an element of the type specified.
 * 
 * Return value: The newly created element
 **/
DomElement *
dom_Document_createElement (DomDocument *doc, const DomString *tagName)
{
	return DOM_ELEMENT (dom_Node_mkref (xmlNewDocNode (((xmlDoc *)DOM_NODE (doc)->xmlnode), NULL, tagName, NULL)));
}


/**
 * dom_Document_createTextNode:
 * @doc: a DomDocument
 * @data: The data for the node.
 * 
 * Creates a text node given the specified string.
 * 
 * Return value: The new Text object.
 **/
DomText *
dom_Document_createTextNode (DomDocument *doc, const DomString *data)
{
	return DOM_TEXT (dom_Node_mkref (xmlNewDocTextLen ((xmlDoc *)DOM_NODE (doc)->xmlnode, data, strlen (data))));
}

/**
 * dom_Document_createComment:
 * @doc: a DomDocument
 * @data: The data for the node.
 * 
 * Creates a comment node given the specified string.
 * 
 * Return value: The new comment object.
 **/
DomComment *
dom_Document_createComment (DomDocument *doc, const DomString *data)
{
	return DOM_COMMENT (dom_Node_mkref (xmlNewDocComment ((xmlDoc *)DOM_NODE (doc)->xmlnode, data)));
}

DomDocumentType *
dom_Document__get_doctype (DomDocument *doc)
{
	return DOM_DOCUMENT_TYPE (dom_Node_mkref ((xmlNode *)((xmlDoc *)DOM_NODE (doc)->xmlnode)->intSubset));
}

static DomNodeIterator *
dom_Document_createNodeIterator (DomDocumentTraversal *traversal, DomNode *root, gulong whatToShow, DomNodeFilter *filter, gboolean entityReferenceExpansion, DomException *exc)
{
	DomNodeIterator *iterator;
	DomDocument *document = DOM_DOCUMENT (traversal);
	
	if (root == NULL) {
		DOM_SET_EXCEPTION (DOM_NOT_SUPPORTED_ERR);
		return NULL;
	}

	iterator = g_object_new (DOM_TYPE_NODE_ITERATOR, NULL);
	iterator->document = document;
	iterator->root = root;
	iterator->whatToShow = whatToShow;
	iterator->filter = filter;
	iterator->expandEntityReferences = entityReferenceExpansion;

	document->iterators  = g_slist_append (document->iterators, iterator);
	
	return iterator;
}

static void
dom_document_traversal_init (DomDocumentTraversalIface *iface)
{
	iface->createNodeIterator = dom_Document_createNodeIterator;
}

static void
dom_document_class_init (DomDocumentClass *klass)
{
}

static void
dom_document_init (DomDocument *doc)
{
}

GType
dom_document_get_type (void)
{
	static GType dom_document_type = 0;

	if (!dom_document_type) {
		static const GTypeInfo dom_document_info = {
			sizeof (DomDocumentClass),
			NULL, /* base_init */
			NULL, /* base_finalize */
			(GClassInitFunc) dom_document_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (DomDocument),
			16,   /* n_preallocs */
			(GInstanceInitFunc) dom_document_init,
		};

		static const GInterfaceInfo dom_document_traversal_info = {
			(GInterfaceInitFunc) dom_document_traversal_init,
			NULL,
			NULL
		};


		dom_document_type = g_type_register_static (DOM_TYPE_NODE, "DomDocument", &dom_document_info, 0);
		g_type_add_interface_static (dom_document_type,
					     DOM_TYPE_DOCUMENT_TRAVERSAL,
					     &dom_document_traversal_info);

	}

	return dom_document_type;
}
