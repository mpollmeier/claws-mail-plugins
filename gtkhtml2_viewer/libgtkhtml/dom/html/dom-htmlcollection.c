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

#include "dom-htmlcollection.h"

static GObjectClass *parent_class = NULL;

/**
 * dom_html_collection__get_length:
 * @collection: a DomHTMLCollection
 * 
 * Returns the number of nodes in the collection. The range of valid child nodes is 0 to length-1.
 * 
 * Return value: The number of nodes in the collection.
 **/
gulong
dom_HTMLCollection__get_length (DomHTMLCollection *collection)
{
	return collection->length (collection);
}

DomNode *
dom_HTMLCollection__get_item (DomHTMLCollection *collection, gulong index)
{
	return collection->item (collection, index);
}

DomNode *
dom_HTMLCollection__get_namedItem (DomHTMLCollection *collection, const DomString *name)
{
	return collection->namedItem (collection, name);
}

static void
dom_html_collection_finalize (GObject *object)
{
	DomHTMLCollection *collection = DOM_HTML_COLLECTION (object);

	if (collection->node)
		g_object_unref (collection->node);
	
	parent_class->finalize (object);
}

static void
dom_html_collection_class_init (DomHTMLCollectionClass *klass)
{
	GObjectClass *object_class = (GObjectClass *)klass;

	object_class->finalize = dom_html_collection_finalize;

	parent_class = g_type_class_peek_parent (klass);
}

static void
dom_html_collection_init (DomHTMLCollection *collection)
{
}

GType
dom_html_collection_get_type (void)
{
	static GType dom_html_collection_type = 0;

	if (!dom_html_collection_type) {
		static const GTypeInfo dom_html_collection_info = {
			sizeof (DomHTMLCollectionClass),
			NULL, /* base_init */
			NULL, /* base_finalize */
			(GClassInitFunc) dom_html_collection_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (DomHTMLCollection),
			16,   /* n_preallocs */
			(GInstanceInitFunc) dom_html_collection_init,
		};

		dom_html_collection_type = g_type_register_static (G_TYPE_OBJECT, "DomHTMLCollection", &dom_html_collection_info, 0);
	}

	return dom_html_collection_type;
}
