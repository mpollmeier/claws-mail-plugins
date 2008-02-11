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

#include "dom-element.h"
#include "dom-attr.h"

/**
 * dom_Element__get_tagName:
 * @element: a DomElement
 * 
 * Returns the name of the elemnent.
 * 
 * Return value: The name of the element. This value must be freed.
 **/
DomString *
dom_Element__get_tagName (DomElement *element)
{
	return g_strdup (DOM_NODE (element)->xmlnode->name);
}

/**
 * dom_Element_getAttribute:
 * @element: a DomElement
 * @name: the name of the attribute to retrieve
 * 
 * Retrieves an attribute value by name.
 * 
 * Return value: The attribute value or an empty string if that attribute does not have a value. This value must be freed.
 **/
DomString *
dom_Element_getAttribute (DomElement *element, const DomString *name)
{
	return xmlGetProp (DOM_NODE (element)->xmlnode, name);
}

/**
 * dom_Element_setAttribute: 
 * @element: a DomElement
 * @name: the name of the attribute to set.
 * @value: the value to set.
 * 
 * Add a new attribute value to an element. If the value already exists the old value will be replaced by the new one.
 **/
void
dom_Element_setAttribute (DomElement *element, DomString *name, const DomString *value)
{
	xmlSetProp (DOM_NODE (element)->xmlnode, name, value);
}


/**
 * dom_Element_hasAttribute:
 * @element: a DomElement
 * @name: The name of the attribute to look for
 * 
 * Returns TRUE if an attribute with the specified name exists.
 *
 * Return value: TRUE if an attribute with the specified name exists, FALSE otherwise
 **/
DomBoolean
dom_Element_hasAttribute (DomElement *element, const DomString *name)
{
	if (!element)
		return FALSE;
	return (xmlHasProp (DOM_NODE (element)->xmlnode, name) != NULL);
}

/**
 * dom_Element_removeAttribute:
 * @element: a DomElement
 * @name: the name of the attribute to remove
 * 
 * Removes an attribute by name.
 **/
void
dom_Element_removeAttribute (DomElement *element, const DomString *name)
{
	xmlRemoveProp (xmlHasProp (DOM_NODE (element)->xmlnode, name));
}

DomAttr *
dom_Element_getAttributeNode (DomElement *element, const DomString *name)
{
	xmlAttr *node;

	node = DOM_NODE (element)->xmlnode->properties;

	while (node) {
		if (strcmp (node->name, name) == 0)
			return DOM_ATTR (dom_Node_mkref ((xmlNode *)node));
		
		node = node->next;
	}

	return NULL;
}

gboolean
dom_element_is_focusable (DomElement *element)
{
	return DOM_ELEMENT_GET_CLASS (element)->is_focusable (element);
}

static gboolean
dom_element_real_is_focusable (DomElement *element)
{
	return FALSE;
}

static void
dom_element_class_init (DomElementClass *klass)
{
	klass->is_focusable = dom_element_real_is_focusable;
}

static void
dom_element_init (DomElement *element)
{
	element->tabindex = 0;
}

GType
dom_element_get_type (void)
{
	static GType dom_element_type = 0;

	if (!dom_element_type) {
		static const GTypeInfo dom_element_info = {
			sizeof (DomElementClass),
			NULL, /* base_init */
			NULL, /* base_finalize */
			(GClassInitFunc) dom_element_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (DomElement),
			16,   /* n_preallocs */
			(GInstanceInitFunc) dom_element_init,
		};

		dom_element_type = g_type_register_static (DOM_TYPE_NODE, "DomElement", &dom_element_info, 0);
	}

	return dom_element_type;
}
