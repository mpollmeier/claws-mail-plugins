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

#include "dom-attr.h"
#include "dom-element.h"

/**
 * dom_Attr__get_name:
 * @attr: a DomAttr.
 * 
 * Returns the name of this attribute.
 * 
 * Return value: The name of this attribute. This value must be freed.
 **/
DomString *
dom_Attr__get_name (DomAttr *attr)
{
	return g_strdup (DOM_NODE (attr)->xmlnode->name);
}

/**
 * dom_Attr__get_value:
 * @attr: a DomAttr.
 * 
 * Returns the value of this attribute as a string.
 * 
 * Return value: The value of this attribute. This value must be freed.
 **/
DomString *
dom_Attr__get_value (DomAttr *attr)
{
	DomString *result;
	xmlNode *node = DOM_NODE (attr)->xmlnode;
	
	result = xmlNodeListGetString(node->parent->doc, node->children, 1);

	if (result == NULL)
		return g_strdup ("");

	return result;
}

/**
 * dom_Attr__set_value:
 * @attr: a DomAttr
 * @value: the value to be set.
 * @exc: return location for an exception.
 * 
 * Sets the value of the attribute.
 **/
void
dom_Attr__set_value (DomAttr *attr, const DomString *value, DomException *exc)
{
	xmlSetProp (DOM_NODE (attr)->xmlnode->parent,
		    DOM_NODE (attr)->xmlnode->name,
		    value);
}

/**
 * dom_Attr__get_ownerElement:
 * @attr: a DomAttr
 * 
 * Returns the element node this attribute is attached to or NULL if this attribute isn't used.
 * 
 * Return value: The element node that this attribute is attached to.
 **/
DomElement *
dom_Attr__get_ownerElement (DomAttr *attr)
{
	return DOM_ELEMENT (dom_Node_mkref (DOM_NODE (attr)->xmlnode->parent));
}

/**
 * dom_Attr_get_specified:
 * @attr: a DomAttr.
 * 
 * If this attribute was explicitly given a value in the original document, this is TRUE; otherwise, it is FALSE. 
 * 
 * Return value: If the attribute was given a value in the original document.
 **/
DomBoolean
dom_Attr_get_specified (DomAttr *attr)
{
	/* FIXME: This has to do a real check */
	return TRUE;
}

static void
dom_attr_class_init (DomAttrClass *klass)
{
}

static void
dom_attr_init (DomAttr *attr)
{
}

GType
dom_attr_get_type (void)
{
	static GType dom_attr_type = 0;

	if (!dom_attr_type) {
		static const GTypeInfo dom_attr_info = {
			sizeof (DomAttrClass),
			NULL, /* base_init */
			NULL, /* base_finalize */
			(GClassInitFunc) dom_attr_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (DomAttr),
			16,   /* n_preallocs */
			(GInstanceInitFunc) dom_attr_init,
		};

		dom_attr_type = g_type_register_static (DOM_TYPE_NODE, "DomAttr", &dom_attr_info, 0);
	}

	return dom_attr_type;
}
