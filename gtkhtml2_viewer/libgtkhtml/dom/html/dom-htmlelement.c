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

#include "dom-htmlelement.h"

/**
 * dom_HTMLElement__get_id:
 * @element: a DomHTMLElement
 * 
 * Returns the id of the HTMLElement.
 * 
 * Return value: Returns the id of the HTMLElement. This value must be freed.
 **/
DomString *
dom_HTMLElement__get_id (DomHTMLElement *element)
{
	return dom_Element_getAttribute (DOM_ELEMENT (element), "id");
}

/**
 * dom_HTMLElement__get_title:
 * @element: a DomHTMLElement
 * 
 * Returns the title of the HTMLElement.
 * 
 * Return value: Returns the title of the HTMLElement. This value must be freed.
 **/
DomString *
dom_HTMLElement__get_title (DomHTMLElement *element)
{
	return dom_Element_getAttribute (DOM_ELEMENT (element), "title");
}

/**
 * dom_HTMLElement__get_lang:
 * @element: a DomHTMLElement
 * 
 * Returns the id of the HTMLElement.
 * 
 * Return value: Returns the lang of the HTMLElement. This value must be freed.
 **/
DomString *
dom_HTMLElement__get_lang (DomHTMLElement *element)
{
	return dom_Element_getAttribute (DOM_ELEMENT (element), "lang");
}

/**
 * dom_HTMLElement__get_dir:
 * @element: a DomHTMLElement
 * 
 * Returns the id of the HTMLElement.
 * 
 * Return value: Returns the dir of the HTMLElement. This value must be freed.
 **/
DomString *
dom_HTMLElement__get_dir (DomHTMLElement *element)
{
	return dom_Element_getAttribute (DOM_ELEMENT (element), "dir");
}

/**
 * dom_HTMLElement__get_className:
 * @element: a DomHTMLElement
 * 
 * Returns the className of the HTMLElement.
 * 
 * Return value: Returns The className of the HTMLElement. This value must be freed.
 **/
DomString *
dom_HTMLElement__get_className (DomHTMLElement *element)
{
	return dom_Element_getAttribute (DOM_ELEMENT (element), "class");
}

void
dom_HTMLElement__set_id (DomHTMLElement *element, const DomString *id)
{
	dom_Element_setAttribute (DOM_ELEMENT (element), "id", id);
}

void
dom_HTMLElement__set_title (DomHTMLElement *element, const DomString *title)
{
	dom_Element_setAttribute (DOM_ELEMENT (element), "title", title);
}

void
dom_HTMLElement__set_lang (DomHTMLElement *element, const DomString *lang)
{
	dom_Element_setAttribute (DOM_ELEMENT (element), "lang", lang);
}

void
dom_HTMLElement__set_dir (DomHTMLElement *element, const DomString *dir)
{
	dom_Element_setAttribute (DOM_ELEMENT (element), "dir", dir);
}

void
dom_HTMLElement__set_className (DomHTMLElement *element, const DomString *className)
{
	dom_Element_setAttribute (DOM_ELEMENT (element), "class", className);
}

/**
 * dom_html_element_parse_html_properties:
 * @element: 
 * @document: The corresponding HtmlDocument
 * 
 * This function parses HTML element specific properties.
 **/
void 
dom_html_element_parse_html_properties (DomHTMLElement *element, HtmlDocument *document)
{
	if (DOM_HTML_ELEMENT_GET_CLASS(element)->parse_html_properties != NULL)
		DOM_HTML_ELEMENT_GET_CLASS(element)->parse_html_properties (element, document);	
}

static void
dom_html_element_class_init (DomHTMLElementClass *klass)
{
}

static void
dom_html_element_init (DomHTMLElement *element)
{
}

GType
dom_html_element_get_type (void)
{
	static GType dom_html_element_type = 0;

	if (!dom_html_element_type) {
		static const GTypeInfo dom_html_element_info = {
			sizeof (DomHTMLElementClass),
			NULL, /* base_init */
			NULL, /* base_finalize */
			(GClassInitFunc) dom_html_element_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (DomHTMLElement),
			16,   /* n_preallocs */
			(GInstanceInitFunc) dom_html_element_init,
		};

		dom_html_element_type = g_type_register_static (DOM_TYPE_ELEMENT, "DomHTMLElement", &dom_html_element_info, 0);
	}

	return dom_html_element_type;
}
