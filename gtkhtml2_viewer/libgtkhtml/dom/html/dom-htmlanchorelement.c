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

#include "dom-htmlanchorelement.h"
#include "dom-htmlinputelement.h"

static GObjectClass *parent_class = NULL;

static void
dom_html_anchor_element_finalize (GObject *object)
{
  /*	DomHTMLAnchorElement *anchor = DOM_HTML_ANCHOR_ELEMENT (object); */

	parent_class->finalize (object);
}

static void 
parse_html_properties (DomHTMLElement *htmlelement, HtmlDocument *document)
{
  /*	DomHTMLAnchorElement *anchorelement = DOM_HTML_ANCHOR_ELEMENT (htmlelement); */
	DomElement *element = DOM_ELEMENT (htmlelement);
	gchar *str;
	gint tabindex;

	if ((str = dom_Element_getAttribute (element, "tabindex"))) {
		tabindex = atoi (str);

		if (tabindex > 0)
			element->tabindex = tabindex;

		g_free (str);
	}
}

static gboolean
is_focusable (DomElement *element)
{
	if (!dom_Element_hasAttribute (element, "href"))
		return FALSE;
	return (dom_Element_hasAttribute (element, "disabled") == FALSE);
}

static void
dom_html_anchor_element_class_init (GObjectClass *klass)
{
	DomHTMLElementClass *html_element_class = (DomHTMLElementClass *)klass;
	DomElementClass *element_class = (DomElementClass *)klass;
	
	element_class->is_focusable = is_focusable;
	html_element_class->parse_html_properties = parse_html_properties;

	klass->finalize = dom_html_anchor_element_finalize;
	
	parent_class = g_type_class_peek_parent (klass);
}

static void
dom_html_anchor_element_init (DomHTMLAnchorElement *doc)
{
}

GType
dom_html_anchor_element_get_type (void)
{
	static GType dom_html_anchor_element_type = 0;

	if (!dom_html_anchor_element_type) {
		static const GTypeInfo dom_html_anchor_element_info = {
			sizeof (DomHTMLAnchorElementClass),
			NULL, /* base_init */
			NULL, /* base_finalize */
			(GClassInitFunc) dom_html_anchor_element_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (DomHTMLAnchorElement),
			16,   /* n_preallocs */
			(GInstanceInitFunc) dom_html_anchor_element_init,
		};

		dom_html_anchor_element_type = g_type_register_static (DOM_TYPE_HTML_ELEMENT, 
								       "DomHTMLAnchorElement", &dom_html_anchor_element_info, 0);
	}

	return dom_html_anchor_element_type;
}
