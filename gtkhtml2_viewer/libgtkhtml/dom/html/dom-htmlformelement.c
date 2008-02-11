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
#include "dom/events/dom-event-utils.h"
#include "dom-htmlformelement.h"
#include "dom-htmlinputelement.h"
#include "dom-htmltextareaelement.h"
#include "dom-htmlselectelement.h"
#include "dom-htmloptionelement.h"

DomString *
dom_HTMLFormElement__get_name (DomHTMLFormElement *form)
{
	return dom_Element_getAttribute (DOM_ELEMENT (form), "name");
}

DomString *
dom_HTMLFormElement__get_acceptCharset (DomHTMLFormElement *form)
{
	return dom_Element_getAttribute (DOM_ELEMENT (form), "accept-charset");
}

DomString *
dom_HTMLFormElement__get_action (DomHTMLFormElement *form)
{
	return dom_Element_getAttribute (DOM_ELEMENT (form), "action");
}

DomString *
dom_HTMLFormElement__get_enctype (DomHTMLFormElement *form)
{
	return dom_Element_getAttribute (DOM_ELEMENT (form), "enctype");
}

DomString *
dom_HTMLFormElement__get_method (DomHTMLFormElement *form)
{
	return dom_Element_getAttribute (DOM_ELEMENT (form), "method");
}

DomString *
dom_HTMLFormElement__get_target (DomHTMLFormElement *form)
{
	return dom_Element_getAttribute (DOM_ELEMENT (form), "target");
}

void
dom_HTMLFormElement__set_name (DomHTMLFormElement *form, const DomString *str)
{
	dom_Element_setAttribute (DOM_ELEMENT (form), "name", str);
}

void
dom_HTMLFormElement__set_acceptCharset (DomHTMLFormElement *form, const DomString *str)
{
	dom_Element_setAttribute (DOM_ELEMENT (form), "accept-charset", str);
}

void
dom_HTMLFormElement__set_action (DomHTMLFormElement *form, const DomString *str)
{
	dom_Element_setAttribute (DOM_ELEMENT (form), "action", str);
}

void
dom_HTMLFormElement__set_enctype (DomHTMLFormElement *form, const DomString *str)
{
	dom_Element_setAttribute (DOM_ELEMENT (form), "enctype", str);
}

void
dom_HTMLFormElement__set_method (DomHTMLFormElement *form, const DomString *str)
{
	dom_Element_setAttribute (DOM_ELEMENT (form), "method", str);
}

void
dom_HTMLFormElement__set_target (DomHTMLFormElement *form, const DomString *str)
{
	dom_Element_setAttribute (DOM_ELEMENT (form), "target", str);
}

static gboolean
is_control (DomNode *node)
{
	/* FIXME: add TextArea, Select ... / jb */
	if (DOM_IS_HTML_INPUT_ELEMENT (node))
		return TRUE;
	else if (DOM_IS_HTML_SELECT_ELEMENT (node))
		return TRUE;
	else if (DOM_IS_HTML_OPTION_ELEMENT (node))
		return TRUE;
	else if (DOM_IS_HTML_TEXT_AREA_ELEMENT (node))
		return TRUE;
	else
		return FALSE;
}

static DomNode *
item_helper (xmlNode *n, gulong *index)
{
	DomNode *node = dom_Node_mkref (n);

	if (is_control (node)) {
		if (*index == 0)
			return node;
		else 
			(*index)--;
	}
	else {
		xmlNode *child = n->children;

		while (child) {
			DomNode *res = item_helper (child, index);
			
			if (res)
				return res;

			child = child->next;
		}
	}
	return NULL;
}

static DomNode *
dom_HTMLFormElement__get_elements_item (DomHTMLCollection *collection, gulong index)
{
	return item_helper (collection->node->xmlnode, &index);
}

static gboolean
has_name (DomNode *node, const DomString *name)
{
	if (DOM_IS_HTML_INPUT_ELEMENT (node)) {

		const gchar *str = dom_HTMLInputElement__get_name (DOM_HTML_INPUT_ELEMENT (node));

		if (str && strcasecmp (name, str) == 0)
			return TRUE;
	}
	return FALSE;
}

static DomNode *
namedItem_helper (xmlNode *n, const DomString *name)
{
	DomNode *node = dom_Node_mkref (n);

	if (is_control (node) && has_name (node, name))
		return node;
	else {
		xmlNode *child = n->children;

		while (child) {
			DomNode *res = namedItem_helper (child, name);
			
			if (res)
				return res;

			child = child->next;
		}
	}
	return NULL;
}

static DomNode *
dom_HTMLFormElement__get_elements_namedItem (DomHTMLCollection *collection, const DomString *name)
{
	return namedItem_helper (collection->node->xmlnode, name);
}

static gulong
length_helper (xmlNode *n)
{
	DomNode *node = dom_Node_mkref (n);
	xmlNode *child = n->children;
	gulong num = 0;

	if (is_control (node))
		num++;

	while (child) {
		num += length_helper (child);
		child = child->next;
	}
	return num;
}

static gulong
dom_HTMLFormElement__get_elements_length (DomHTMLCollection *collection)
{
	return length_helper (collection->node->xmlnode);
}

gulong
dom_HTMLFormElement__get_length (DomHTMLFormElement *element)
{
	return length_helper (DOM_NODE (element)->xmlnode);
}

DomHTMLCollection *
dom_HTMLFormElement__get_elements (DomHTMLFormElement *form)
{
	DomHTMLCollection *collection = g_object_new (DOM_TYPE_HTML_COLLECTION, NULL);

	collection->item = dom_HTMLFormElement__get_elements_item;
	collection->length = dom_HTMLFormElement__get_elements_length;
	collection->namedItem = dom_HTMLFormElement__get_elements_namedItem;
	
	collection->node = g_object_ref (G_OBJECT (form));
	
	return collection;
}

gchar *
dom_HTMLFormElement__get_encoding (DomHTMLFormElement *form)
{
	GString *encoding = g_string_new ("");
	gint first = TRUE;
	gchar *ptr;
	gint i, length;
	DomHTMLCollection *collection;

	collection = dom_HTMLFormElement__get_elements (form);
	length = dom_HTMLCollection__get_length (collection);

	for (i = 0; i < length; i++) {

		DomNode *node = dom_HTMLCollection__get_item (collection, i);

		ptr = NULL;
		if (DOM_IS_HTML_INPUT_ELEMENT (node))
			ptr = dom_html_input_element_encode (DOM_HTML_INPUT_ELEMENT (node));
		else if (DOM_IS_HTML_TEXT_AREA_ELEMENT (node))
			ptr = dom_html_text_area_element_encode (DOM_HTML_TEXT_AREA_ELEMENT (node));
		else if (DOM_IS_HTML_SELECT_ELEMENT (node))
			ptr = dom_html_select_element_encode (DOM_HTML_SELECT_ELEMENT (node));
		

		if (ptr && strlen (ptr)) {
			if(!first)
				encoding = g_string_append_c (encoding, '&');
			else
				first = FALSE;
			
			encoding = g_string_append (encoding, ptr);
			g_free (ptr);
		}
	}
	ptr = encoding->str;
	g_string_free (encoding, FALSE);
	return ptr;
}

void 
dom_HTMLFormElement_submit (DomHTMLFormElement *form)
{
	dom_Event_invoke (DOM_EVENT_TARGET (form), "submit", TRUE, TRUE);
}

void
dom_HTMLFormElement_reset  (DomHTMLFormElement *form)
{
	gint i, length;
	DomHTMLCollection *collection;

	collection = dom_HTMLFormElement__get_elements (form);
	length = dom_HTMLCollection__get_length (collection);

	for (i = 0; i < length; i++) {

		DomNode *node = dom_HTMLCollection__get_item (collection, i);

		if (DOM_IS_HTML_INPUT_ELEMENT (node))
			dom_html_input_element_reset (DOM_HTML_INPUT_ELEMENT (node));
		else if (DOM_IS_HTML_TEXT_AREA_ELEMENT (node))
			dom_html_text_area_element_reset (DOM_HTML_TEXT_AREA_ELEMENT (node));

	}
	dom_Event_invoke (DOM_EVENT_TARGET (form), "reset", TRUE, FALSE);
}

static void
dom_html_form_element_class_init (GObjectClass *klass)
{
}

static void
dom_html_form_element_init (DomHTMLFormElement *doc)
{
}

GType
dom_html_form_element_get_type (void)
{
	static GType dom_html_form_element_type = 0;

	if (!dom_html_form_element_type) {
		static const GTypeInfo dom_html_form_element_info = {
			sizeof (DomHTMLFormElementClass),
			NULL, /* base_init */
			NULL, /* base_finalize */
			(GClassInitFunc) dom_html_form_element_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (DomHTMLFormElement),
			16,   /* n_preallocs */
			(GInstanceInitFunc) dom_html_form_element_init,
		};

		dom_html_form_element_type = g_type_register_static (DOM_TYPE_HTML_ELEMENT, 
								     "DomHTMLFormElement", &dom_html_form_element_info, 0);
	}

	return dom_html_form_element_type;
}
