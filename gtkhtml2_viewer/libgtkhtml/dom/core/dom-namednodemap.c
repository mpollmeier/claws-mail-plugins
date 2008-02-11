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

#include <string.h>

#include "dom-namednodemap.h"
#include "dom-node.h"

/**
 * dom_NamedNodeMap_getNamedItem:
 * @map: a DomNamedNodeMap
 * @name: The nodeName of a node to retrieve.
 * 
 * Retrieves a node specified by its name.
 * 
 * Return value: a Node of any type with the specified nodeName or NULL if none is found.
 **/
DomNode *
dom_NamedNodeMap_getNamedItem (DomNamedNodeMap *map, const DomString *name)
{
	xmlNode *attr = map->attr;

	while (attr) {
		if (attr->type == map->type && strcmp (attr->name, name) == 0)
			return dom_Node_mkref (attr);
		
		attr = attr->next;
	}

	return NULL;
}

/**
 * dom_NamedNodeMap_setNamedItem:
 * @map: a DomNamedNodeMap
 * @arg: A node to store in this map.
 * @exc: Return location for an exception.
 * 
 * Adds a node using its nodeName attribute. If a node with that name is already present in this map, it is replaced by the new one.
 *
 * Return value: If the new node replaces an existing node the replaced node is returned, otherwise NULL is returned.
 **/
DomNode *
dom_NamedNodeMap_setNamedItem (DomNamedNodeMap *map, DomNode *arg, DomException *exc)
{
	xmlNode *attr = map->attr;

	if (map->readonly) {
		DOM_SET_EXCEPTION (DOM_NO_MODIFICATION_ALLOWED_ERR);
		
		return NULL;
	}
	else if (attr->doc != arg->xmlnode->doc) {
		DOM_SET_EXCEPTION (DOM_WRONG_DOCUMENT_ERR);

		return NULL;
	}
	else if (arg->xmlnode->parent != NULL) {
		DOM_SET_EXCEPTION (DOM_INUSE_ATTRIBUTE_ERR);

		return NULL;
	}
	
	while (attr) {
		if (attr->type == map->type && strcmp (attr->name, arg->xmlnode->name) == 0) {
			return dom_Node_mkref (xmlReplaceNode (attr, arg->xmlnode));
		}

		attr = attr->next;
	}

	return NULL;
}

/**
 * dom_NamedNodeMap_removeNamedItem:
 * @map: a DomNamedNodeMap
 * @name: The nodeName of the node to remove.
 * @exc: Return location for an exception.
 * 
 * Removes a node specified by name.
 * 
 * Return value: The node removed from this map if a node with such a name exists.
 **/
DomNode *
dom_NamedNodeMap_removeNamedItem (DomNamedNodeMap *map, const DomString *name, DomException *exc)
{
	xmlNode *attr = map->attr;

	if (map->readonly) {
		DOM_SET_EXCEPTION (DOM_NO_MODIFICATION_ALLOWED_ERR);
		
		return NULL;
	}

	while (attr) {
		if (attr->type == map->type && strcmp (attr->name, name) == 0) {
			xmlUnlinkNode (attr);

			return dom_Node_mkref ((xmlNode *)attr);
		}

		attr = attr->next;
	}

	DOM_SET_EXCEPTION (DOM_NOT_FOUND_ERR);

	return NULL;
}

/**
 * dom_NamedNodeMap__get_length:
 * @map: a DomNamedNodeMap
 *
 * Returns the number of nodes in the map. The range of valid child nodes is 0 to length-1.
 * 
 * Return value: The number of nodes in the map.
 **/
gulong
dom_NamedNodeMap__get_length (DomNamedNodeMap *map)
{
	xmlNode *attr = map->attr;
	gulong len = 0;

	while (attr) {
		if (attr->type == map->type)
			len++;
		
		attr = attr->next;
	}
	
	return len;

}

/**
 * dom_NamedNodeMap__get_item:
 * @map: a DomNamedNodeMap.
 * @index: Index into the node map.
 * 
 * Returns the indexth item in the map or NULL if the index value specified is invalid.
 * 
 * Return value: The node at the indexth position in the node map.
 **/
DomNode *
dom_NamedNodeMap__get_item (DomNamedNodeMap *map, gulong index)
{
	xmlNode *attr = map->attr;
	gulong i = 0;

	for (i = 0; i < index; i++) {
		if (!attr)
			return NULL;

		while (attr->type != map->type)
			attr = attr->next;
		
		attr = attr->next;
	}

	return dom_Node_mkref (attr);

}

static void
dom_named_node_map_class_init (DomNamedNodeMapClass *klass)
{
}

static void
dom_named_node_map_init (DomNamedNodeMap *map)
{
}

GType
dom_named_node_map_get_type (void)
{
	static GType dom_named_node_map_type = 0;

	if (!dom_named_node_map_type) {
		static const GTypeInfo dom_named_node_map_info = {
			sizeof (DomNamedNodeMapClass),
			NULL, /* base_init */
			NULL, /* base_finalize */
			(GClassInitFunc) dom_named_node_map_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (DomNamedNodeMap),
			16,   /* n_preallocs */
			(GInstanceInitFunc) dom_named_node_map_init,
		};

		dom_named_node_map_type = g_type_register_static (G_TYPE_OBJECT, "DomNamedNodeMap", &dom_named_node_map_info, 0);
	}

	return dom_named_node_map_type;
}
