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
#include <stdlib.h>
#include "dom-htmltextareaelement.h"
#include "util/rfc1738.h"

static GObjectClass *parent_class = NULL;

DomString *
dom_HTMLTextAreaElement__get_type (DomHTMLTextAreaElement *select)
{
	return g_strdup ("textarea");
}

DomString *
dom_HTMLTextAreaElement__get_name (DomHTMLTextAreaElement *text_area)
{
	return dom_Element_getAttribute (DOM_ELEMENT (text_area), "name");
}

DomString *
dom_HTMLTextAreaElement__get_defaultValue (DomHTMLTextAreaElement *text_area)
{
	if (text_area->defaultValue)
		return g_strdup (text_area->defaultValue);
	else
		return g_strdup ("");
}

DomString *
dom_HTMLTextAreaElement__get_value (DomHTMLTextAreaElement *text_area)
{
	GtkTextIter start, end;

	gtk_text_buffer_get_iter_at_offset (text_area->buffer, &start, 0);
	gtk_text_buffer_get_end_iter (text_area->buffer, &end);

	return gtk_text_buffer_get_text (text_area->buffer, &start, &end, FALSE);
}

DomBoolean
dom_HTMLTextAreaElement__get_readOnly (DomHTMLTextAreaElement *text_area)
{
	return dom_Element_hasAttribute (DOM_ELEMENT (text_area), "readonly");
}

DomBoolean
dom_HTMLTextAreaElement__get_disabled (DomHTMLTextAreaElement *text_area)
{
	return dom_Element_hasAttribute (DOM_ELEMENT (text_area), "disabled");
}

glong
dom_HTMLTextAreaElement__get_rows (DomHTMLTextAreaElement *text_area)
{
	glong rows = 1;

	gchar *str = dom_Element_getAttribute (DOM_ELEMENT (text_area), "rows");

	if (str) {
		str = g_strchug (str);
		rows = atoi (str);
		xmlFree (str);
	}
	return rows;
}

glong
dom_HTMLTextAreaElement__get_cols (DomHTMLTextAreaElement *text_area)
{
	glong cols = 1;

	gchar *str = dom_Element_getAttribute (DOM_ELEMENT (text_area), "cols");

	if (str) {
		str = g_strchug (str);
		cols = atoi (str);
		xmlFree (str);
	}
	return cols;
}

DomHTMLFormElement *
dom_HTMLTextAreaElement__get_form (DomHTMLTextAreaElement *text_area)
{
	DomNode *form = dom_Node__get_parentNode (DOM_NODE (text_area));
	
	while (form && !DOM_IS_HTML_FORM_ELEMENT (form))
		form = dom_Node__get_parentNode (form);
	
	return (DomHTMLFormElement *)form;
}

void
dom_HTMLTextAreaElement__set_name (DomHTMLTextAreaElement *text_area, const DomString *name)
{
	dom_Element_setAttribute (DOM_ELEMENT (text_area), "name", name);
}

void
dom_HTMLTextAreaElement__set_defaultValue (DomHTMLTextAreaElement *text_area, const DomString *value)
{
	if (text_area->defaultValue)
		g_free (text_area->defaultValue);
	
	text_area->defaultValue = g_strdup (value);

	dom_HTMLTextAreaElement__set_value (text_area, text_area->defaultValue);
}

void
dom_HTMLTextAreaElement__set_value (DomHTMLTextAreaElement *text_area, const DomString *value)
{
	gtk_text_buffer_set_text (text_area->buffer, value, strlen (value));
}

void
dom_HTMLTextAreaElement__set_rows (DomHTMLTextAreaElement *text_area, glong rows)
{
	gchar *str = g_strdup_printf ("%ld", rows);
	dom_Element_setAttribute (DOM_ELEMENT (text_area), "rows", str);
	g_free (str);
}

void
dom_HTMLTextAreaElement__set_cols (DomHTMLTextAreaElement *text_area, glong cols)
{
	gchar *str = g_strdup_printf ("%ld", cols);
	dom_Element_setAttribute (DOM_ELEMENT (text_area), "cols", str);
	g_free (str);
}

void
dom_HTMLTextAreaElement__set_readOnly (DomHTMLTextAreaElement *text_area, DomBoolean readonly)
{
	if (readonly)
		dom_Element_setAttribute (DOM_ELEMENT (text_area), "readonly", NULL);
	else
		dom_Element_removeAttribute (DOM_ELEMENT (text_area), "readonly");
}

void
dom_HTMLTextAreaElement__set_disabled (DomHTMLTextAreaElement *text_area, DomBoolean disabled)
{
	if (disabled)
		dom_Element_setAttribute (DOM_ELEMENT (text_area), "disabled", NULL);
	else
		dom_Element_removeAttribute (DOM_ELEMENT (text_area), "disabled");
}

static gboolean
is_focusable (DomElement *element)
{
	return TRUE;
}

void
dom_html_text_area_element_reset (DomHTMLTextAreaElement *text_area)
{
	dom_HTMLTextAreaElement__set_value (text_area, dom_HTMLTextAreaElement__get_defaultValue (text_area));
}

DomString *
dom_html_text_area_element_encode (DomHTMLTextAreaElement *text_area)
{
	GString *encoding = g_string_new ("");
	gchar *ptr, *value;
	gchar *name = dom_HTMLTextAreaElement__get_name (text_area);

	if (name == NULL)
		return g_strdup ("");

	value = dom_HTMLTextAreaElement__get_value (text_area);

	if (value) {
		
		ptr = rfc1738_encode_string (name);
		encoding = g_string_append (encoding, ptr);
		g_free (ptr);
		
		encoding = g_string_append_c (encoding, '=');
		
		ptr = rfc1738_encode_string (value);
		encoding = g_string_append (encoding, ptr);
		
		g_free (ptr);			
		xmlFree (value);
	}

	xmlFree (name);

	ptr = encoding->str;
	g_string_free(encoding, FALSE);

	return ptr;
}

static void
finalize (GObject *object)
{
	DomHTMLTextAreaElement *text_area = DOM_HTML_TEXT_AREA_ELEMENT (object);

	if (text_area->defaultValue)
		g_free (text_area->defaultValue);

	g_object_unref (G_OBJECT (text_area->buffer));

	parent_class->finalize (object);
}

GtkTextBuffer *
dom_html_text_area_element_get_text_buffer (DomHTMLTextAreaElement *textarea)
{
	return textarea->buffer;
}

static void
dom_html_text_area_element_class_init (GObjectClass *klass)
{
	DomElementClass *element_class = (DomElementClass *)klass;

	klass->finalize = finalize;
	element_class->is_focusable = is_focusable;
	
	parent_class = g_type_class_peek_parent (klass);
}

static void
dom_html_text_area_element_init (DomHTMLTextAreaElement *text_area)
{
	text_area->buffer = gtk_text_buffer_new (NULL);
}

GType
dom_html_text_area_element_get_type (void)
{
	static GType dom_html_text_area_element_type = 0;

	if (!dom_html_text_area_element_type) {
		static const GTypeInfo dom_html_text_area_element_info = {
			sizeof (DomHTMLTextAreaElementClass),
			NULL, /* base_init */
			NULL, /* base_finalize */
			(GClassInitFunc) dom_html_text_area_element_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (DomHTMLTextAreaElement),
			16,   /* n_preallocs */
			(GInstanceInitFunc) dom_html_text_area_element_init,
		};

		dom_html_text_area_element_type = g_type_register_static (DOM_TYPE_HTML_ELEMENT, 
								     "DomHTMLTextAreaElement", &dom_html_text_area_element_info, 0);
	}
	return dom_html_text_area_element_type;
}
