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

#include "dom-documenttype.h"

/**
 * dom_DocumentType__get_name:
 * @dtd: a DomDocumentType
 * 
 * The name of the DTD, the name immediately following the DOCTYPE keyword.
 * 
 * Return value: The name of the DTD. The value must be freed.
 **/
DomString *
dom_DocumentType__get_name (DomDocumentType *dtd)
{
	return g_strdup (DOM_NODE (dtd)->xmlnode->name);
}

/**
 * dom_DocumentType__get_publicId:
 * @dtd: a DomDocumentType
 * 
 * Returns the public identifier of the external subset.
 * 
 * Return value: The public identifier of the external subset. This value must be freed.
 **/
DomString *
dom_DocumentType__get_publicId (DomDocumentType *dtd)
{
	return g_strdup (((xmlDtd *)DOM_NODE (dtd)->xmlnode)->ExternalID);
}

/**
 * dom_DocumentType__get_systemId:
 * @dtd: 
 * 
 * Returns the system identifier of the external subset.
 * 
 * Return value: The system identifier of the external subset. This value must be freed.
 **/
DomString *
dom_DocumentType__get_systemId (DomDocumentType *dtd)
{
	return g_strdup (((xmlDtd *)DOM_NODE (dtd)->xmlnode)->SystemID);
}

DomNamedNodeMap *
dom_DocumentType__get_entities (DomDocumentType *dtd)
{
	DomNamedNodeMap *map;

	map = g_object_new (DOM_TYPE_NAMED_NODE_MAP, NULL);
	
	map->attr = DOM_NODE (dtd)->xmlnode->children;
	map->readonly = TRUE;
	map->type = XML_ENTITY_DECL;
	
	return map;
}

static void
dom_document_type_class_init (DomDocumentTypeClass *klass)
{
}

static void
dom_document_type_init (DomDocumentType *doc)
{
}

GType
dom_document_type_get_type (void)
{
	static GType dom_document_type_type = 0;

	if (!dom_document_type_type) {
		static const GTypeInfo dom_document_type_info = {
			sizeof (DomDocumentTypeClass),
			NULL, /* base_init */
			NULL, /* base_finalize */
			(GClassInitFunc) dom_document_type_class_init,
			NULL, /* class_finalize */
			NULL, /* class_type */
			sizeof (DomDocumentType),
			16,   /* n_preallocs */
			(GInstanceInitFunc) dom_document_type_init,
		};

		dom_document_type_type = g_type_register_static (DOM_TYPE_NODE, "DomDocumentType", &dom_document_type_info, 0);
	}

	return dom_document_type_type;
}
