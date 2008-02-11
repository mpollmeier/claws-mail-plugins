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

#include <stdlib.h>
#include "dom-htmloptionelement.h"
#include "dom-htmlselectelement.h"

DomHTMLFormElement *
dom_HTMLOptionElement__get_form (DomHTMLOptionElement *option)
{
	DomNode *form = dom_Node__get_parentNode (DOM_NODE (option));
	
	while (form && !DOM_IS_HTML_FORM_ELEMENT (form))
		form = dom_Node__get_parentNode (form);
	
	return (DomHTMLFormElement *)form;
}

DomString *
dom_HTMLOptionElement__get_label (DomHTMLOptionElement *option)
{
	return dom_Element_getAttribute (DOM_ELEMENT (option), "label");
}

DomString *
dom_HTMLOptionElement__get_text (DomHTMLOptionElement *option)
{
	return dom_Element_getAttribute (DOM_ELEMENT (option), "text");
}

DomString *
dom_HTMLOptionElement__get_value (DomHTMLOptionElement *option)
{
	return dom_Element_getAttribute (DOM_ELEMENT (option), "value");
}

DomBoolean
dom_HTMLOptionElement__get_defaultSelected (DomHTMLOptionElement *option)
{
	return dom_Element_hasAttribute (DOM_ELEMENT (option), "selected");
}

DomBoolean
dom_HTMLOptionElement__get_disabled (DomHTMLOptionElement *option)
{
	return dom_Element_hasAttribute (DOM_ELEMENT (option), "disabled");
}

DomBoolean
dom_HTMLOptionElement__get_selected (DomHTMLOptionElement *option)
{
	/* FIXME: */
	return FALSE;
}

void
dom_HTMLOptionElement__set_label (DomHTMLOptionElement *option, const DomString *label)
{
	dom_Element_setAttribute (DOM_ELEMENT (option), "label", label);
}

void
dom_HTMLOptionElement__set_value (DomHTMLOptionElement *option, const DomString *value)
{
	dom_Element_setAttribute (DOM_ELEMENT (option), "value", value);
}

void
dom_HTMLOptionElement__set_defaultSelected (DomHTMLOptionElement *option, DomBoolean defaultSelected)
{
	if (defaultSelected)
		dom_Element_setAttribute (DOM_ELEMENT (option), "selected", "");
	else
		dom_Element_removeAttribute (DOM_ELEMENT (option), "selected");
}

static DomHTMLSelectElement *
get_select (DomHTMLOptionElement *option)
{
	DomNode *select = dom_Node__get_parentNode (DOM_NODE (option));
	
	while (select && !DOM_IS_HTML_SELECT_ELEMENT (select))
		select = dom_Node__get_parentNode (select);
	
	return (DomHTMLSelectElement *)select;
}

static void 
parse_html_properties (DomHTMLElement *htmlelement, HtmlDocument *document)
{
	DomHTMLOptionElement *option = DOM_HTML_OPTION_ELEMENT (htmlelement);
	DomHTMLSelectElement *select = get_select (option);

	if (select) {
		DomException exc;

		dom_HTMLSelectElement_add (select, DOM_HTML_ELEMENT (option), NULL, &exc);
	}
}

void
dom_html_option_element_new_character_data (DomHTMLOptionElement *option)
{
	DomHTMLSelectElement *select = get_select (option);

	if (select) {
		dom_html_select_element_update_option_data (select, option);
	}
}

static void
dom_html_option_element_init (DomHTMLOptionElement *option)
{
}

static void
dom_html_option_element_class_init (GObjectClass *klass)
{
	DomHTMLElementClass *htmlelement_class = (DomHTMLElementClass *)klass;

	htmlelement_class->parse_html_properties = parse_html_properties;
}

GType
dom_html_option_element_get_type (void)
{
	static GType dom_html_option_element_type = 0;

	if (!dom_html_option_element_type) {
		static const GTypeInfo dom_html_option_element_info = {
			sizeof (DomHTMLOptionElementClass),
			NULL, /* base_init */
			NULL, /* base_finalize */
			(GClassInitFunc) dom_html_option_element_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (DomHTMLOptionElement),
			16,   /* n_preallocs */
			(GInstanceInitFunc) dom_html_option_element_init,
		};

		dom_html_option_element_type = g_type_register_static (DOM_TYPE_HTML_ELEMENT, 
								     "DomHTMLOptionElement", &dom_html_option_element_info, 0);
	}
	return dom_html_option_element_type;
}
