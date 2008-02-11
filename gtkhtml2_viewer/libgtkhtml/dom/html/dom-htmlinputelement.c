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

#include "dom-htmlinputelement.h"
#include "util/htmlmarshal.h"
#include "util/rfc1738.h"

static GObjectClass *parent_class = NULL;

enum {
	WIDGET_TOGGLED,
	WIDGET_TEXT_CHANGED,
	LAST_SIGNAL
};

static guint input_signals [LAST_SIGNAL] = { 0 };
DomString *
dom_HTMLInputElement__get_name (DomHTMLInputElement *input)
{
	return dom_Element_getAttribute (DOM_ELEMENT (input), "name");
}

DomString *
dom_HTMLInputElement__get_defaultValue (DomHTMLInputElement *input)
{
	return dom_Element_getAttribute (DOM_ELEMENT (input), "value");
}

DomString *
dom_HTMLInputElement__get_value (DomHTMLInputElement *input)
{
	if (input->str_value == NULL) {
		gchar *value = dom_HTMLInputElement__get_defaultValue (input);
		input->str_value = g_strdup (value);
		xmlFree (value);
	}
	return g_strdup (input->str_value);
}

DomBoolean
dom_HTMLInputElement__get_readOnly (DomHTMLInputElement *input)
{
	return dom_Element_hasAttribute (DOM_ELEMENT (input), "readonly");
}

DomBoolean
dom_HTMLInputElement__get_checked (DomHTMLInputElement *input)
{
	if (input->checked == -1)
		input->checked = dom_HTMLInputElement__get_defaultChecked (input);

	return input->checked;
}

DomBoolean
dom_HTMLInputElement__get_defaultChecked (DomHTMLInputElement *input)
{
	return dom_Element_hasAttribute (DOM_ELEMENT(input), "checked");
}

DomBoolean
dom_HTMLInputElement__get_disabled (DomHTMLInputElement *input)
{
	return dom_Element_hasAttribute (DOM_ELEMENT (input), "disabled");
}

DomBoolean
dom_HTMLInputElement__get_maxLength (DomHTMLInputElement *input)
{
	gint maxLength = G_MAXINT;
	gchar *str;

	if ((str = dom_Element_getAttribute (DOM_ELEMENT (input), "maxlength"))) {
		
		g_strchug (str);
		maxLength = atoi (str);
		xmlFree (str);
	}
	
	return maxLength;
}

DomString *
dom_HTMLInputElement__get_size (DomHTMLInputElement *input)
{
	return dom_Element_getAttribute (DOM_ELEMENT (input), "size");
}

DomHTMLFormElement *
dom_HTMLInputElement__get_form (DomHTMLInputElement *input)
{
	DomNode *form = dom_Node__get_parentNode (DOM_NODE (input));
	
	while (form && !DOM_IS_HTML_FORM_ELEMENT (form))
		form = dom_Node__get_parentNode (form);
	
	return (DomHTMLFormElement *)form;
}

void
dom_HTMLInputElement__set_name (DomHTMLInputElement *input, const DomString *name)
{
	dom_Element_setAttribute (DOM_ELEMENT (input), "name", name);
}

void
dom_HTMLInputElement__set_defaultValue (DomHTMLInputElement *input, const DomString *value)
{
	dom_Element_setAttribute (DOM_ELEMENT (input), "value", value);

	dom_HTMLInputElement__set_value (input, value);
}

void
dom_HTMLInputElement__set_value (DomHTMLInputElement *input, const DomString *value)
{
	if (input->str_value)
		g_free (input->str_value);
	
	if (value)
		input->str_value = g_strdup (value);
	else
		input->str_value = g_strdup ("");

	dom_html_input_element_widget_text_changed (input);
}
void
dom_HTMLInputElement__set_size (DomHTMLInputElement *input, const DomString *size)
{
	dom_Element_setAttribute (DOM_ELEMENT (input), "size", size);
}

void
dom_HTMLInputElement__set_readOnly (DomHTMLInputElement *input, DomBoolean readonly)
{
	if (readonly)
		dom_Element_setAttribute (DOM_ELEMENT (input), "readonly", NULL);
	else
		dom_Element_removeAttribute (DOM_ELEMENT (input), "readonly");
}

void
dom_HTMLInputElement__set_checked (DomHTMLInputElement *input, DomBoolean checked)
{
	input->checked = checked;
	dom_html_input_element_widget_toggled (input, input->checked);
}

void
dom_HTMLInputElement__set_defaultChecked (DomHTMLInputElement *input, DomBoolean checked)
{
	if (checked)
		dom_Element_setAttribute (DOM_ELEMENT (input), "checked", NULL);
	else
		dom_Element_removeAttribute (DOM_ELEMENT (input), "disabled");

	dom_HTMLInputElement__set_checked (input, checked);
}

void
dom_HTMLInputElement__set_disabled (DomHTMLInputElement *input, DomBoolean disabled)
{
	if (disabled)
		dom_Element_setAttribute (DOM_ELEMENT (input), "disabled", "");
	else
		dom_Element_removeAttribute (DOM_ELEMENT (input), "disabled");
}

void
dom_HTMLInputElement__set_maxLength (DomHTMLInputElement *input, gint maxLength)
{
	gchar *str = g_strdup_printf ("%d", maxLength);
	dom_Element_setAttribute (DOM_ELEMENT (input), "maxlength", str);
	g_free (str);
}

static void 
parse_html_properties (DomHTMLElement *htmlelement, HtmlDocument *document)
{
	DomElement *element = DOM_ELEMENT (htmlelement);
	gchar *src, *type;
	
	if ((type = dom_Element_getAttribute (element, "type"))) {
		
		if ((src = dom_Element_getAttribute (element, "src"))) {
			
			HtmlImage *image;
			
			image = html_image_factory_get_image (document->image_factory, src);
			
			/* FIXME: What about freeing the data? */
			g_object_set_data_full (G_OBJECT (element), "image", image, g_object_unref);
			
			xmlFree (src);
		}
		xmlFree (type);
	}
}

void
dom_html_input_element_widget_toggled (DomHTMLInputElement *input, gboolean checked)
{
	input->checked = checked;
	g_signal_emit (G_OBJECT (input), input_signals [WIDGET_TOGGLED], 0, checked);
}

void
dom_html_input_element_widget_text_changed (DomHTMLInputElement *input)
{
	g_signal_emit (G_OBJECT (input), input_signals [WIDGET_TEXT_CHANGED], 0);
}

static gboolean
is_focusable (DomElement *element)
{
	gchar *str;
	
	if (dom_Element_hasAttribute (element, "disabled"))
		return FALSE;

	if ((str = dom_Element_getAttribute (element, "type")) &&
	    strcasecmp (str, "hidden") == 0) {
		g_free (str);
		return FALSE;
	}
	
	return TRUE;
}

void
dom_html_input_element_reset (DomHTMLInputElement *input)
{
	DomElement *element = DOM_ELEMENT (input);
	gchar *type;

	if ((type = dom_Element_getAttribute (element, "type"))) {

		if (strcasecmp ("radio", type) == 0 || strcasecmp ("checkbox", type) == 0) {
			
			dom_HTMLInputElement__set_checked (input, dom_HTMLInputElement__get_defaultChecked (input));
		}
		else {
			dom_HTMLInputElement__set_value (input, dom_HTMLInputElement__get_defaultValue (input));
		}

		xmlFree (type);
	}	
}

DomString *
dom_html_input_element_encode (DomHTMLInputElement *input)
{
	DomElement *element = DOM_ELEMENT (input);
	GString *encoding = g_string_new ("");
	gchar *type, *ptr;
	gchar *name = dom_HTMLInputElement__get_name (input);

	if (name == NULL)
		return g_strdup ("");

	type = dom_Element_getAttribute (element, "type");
	
	if (type && (strcasecmp ("radio", type) == 0 || 
		     strcasecmp ("checkbox", type) == 0)) {
		
		if (input->checked) {
			
			gchar *value = dom_HTMLInputElement__get_value (input);
			
			if (value == NULL)
				value = g_strdup ("on");
			
			ptr = rfc1738_encode_string (name);
			encoding = g_string_append (encoding, ptr);
			g_free (ptr);
			
			encoding = g_string_append_c (encoding, '=');
			
			ptr = rfc1738_encode_string (value);
			encoding = g_string_append (encoding, ptr);
			
			g_free (ptr);			
			xmlFree (value);
		}
	}
	else {
		gchar *value = dom_HTMLInputElement__get_value (input);
		
		if (value && 
		    !(type && !strcasecmp ("submit", type) && 
		      input->active == FALSE)) {
			
			ptr = rfc1738_encode_string (name);
			encoding = g_string_append (encoding, ptr);
			g_free (ptr);
			
			encoding = g_string_append_c (encoding, '=');
			
			ptr = rfc1738_encode_string (value);
			encoding = g_string_append (encoding, ptr);
			
			g_free (ptr);			
		}
		xmlFree (value);
	}
	
	xmlFree (type);
	xmlFree (name);

	ptr = encoding->str;
	g_string_free(encoding, FALSE);

	return ptr;
}

static void
finalize (GObject *object)
{
	DomHTMLInputElement *input = DOM_HTML_INPUT_ELEMENT (object);

	if (input->str_value)
		g_free (input->str_value);

	parent_class->finalize (object);
}

static void
dom_html_input_element_class_init (GObjectClass *klass)
{
	DomHTMLElementClass *html_element_class = (DomHTMLElementClass *)klass;
	DomElementClass *element_class = (DomElementClass *)klass;

	klass->finalize = finalize;
	html_element_class->parse_html_properties = parse_html_properties;
	element_class->is_focusable = is_focusable;
	
	parent_class = g_type_class_peek_parent (klass);

	input_signals [WIDGET_TOGGLED] =
		g_signal_new ("widget_toggled",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (DomHTMLInputElementClass, widget_toggled),
			      NULL, NULL,
			      html_marshal_VOID__BOOLEAN,
			      G_TYPE_NONE,
			      1,
			      G_TYPE_BOOLEAN);

	input_signals [WIDGET_TEXT_CHANGED] =
		g_signal_new ("widget_text_changed",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (DomHTMLInputElementClass, widget_text_changed),
			      NULL, NULL,
			      html_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
}

static void
dom_html_input_element_init (DomHTMLInputElement *input)
{
	input->checked = -1;
}

GType
dom_html_input_element_get_type (void)
{
	static GType dom_html_input_element_type = 0;

	if (!dom_html_input_element_type) {
		static const GTypeInfo dom_html_input_element_info = {
			sizeof (DomHTMLInputElementClass),
			NULL, /* base_init */
			NULL, /* base_finalize */
			(GClassInitFunc) dom_html_input_element_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (DomHTMLInputElement),
			16,   /* n_preallocs */
			(GInstanceInitFunc) dom_html_input_element_init,
		};

		dom_html_input_element_type = g_type_register_static (DOM_TYPE_HTML_ELEMENT, 
								     "DomHTMLInputElement", &dom_html_input_element_info, 0);
	}
	return dom_html_input_element_type;
}
