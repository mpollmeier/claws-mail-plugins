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

#include "dom-domimplementation.h"

/**
 * dom_DOMImplementation_hasFeature:
 * @feature: The name of the feature to test (case-insensitive
 * @version: The version number of the feature to test.
 * 
 * Test if the DOM implementation implements a specific feature.
 * 
 * Return value: TRUE if the feature is implemented, FALSE otherwise.
 **/
DomBoolean
dom_DOMImplementation_hasFeature (DomString *feature, DomString *version)
{
	return FALSE;
}

static void
dom_dom_implementation_class_init (DomDOMImplementationClass *klass)
{
}

static void
dom_dom_implementation_init (DomDOMImplementation *doc)
{
}

GType
dom_dom_implementation_get_type (void)
{
	static GType dom_dom_implementation_type = 0;

	if (!dom_dom_implementation_type) {
		static const GTypeInfo dom_dom_implementation_info = {
			sizeof (DomDOMImplementationClass),
			NULL, /* base_init */
			NULL, /* base_finalize */
			(GClassInitFunc) dom_dom_implementation_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (DomDOMImplementation),
			16,   /* n_preallocs */
			(GInstanceInitFunc) dom_dom_implementation_init,
		};

		dom_dom_implementation_type = g_type_register_static (DOM_TYPE_NODE, "DomDOMImplementation", &dom_dom_implementation_info, 0);
	}

	return dom_dom_implementation_type;
}

DomDOMImplementation *
dom_DOMImplementation_new (void)
{
	return g_object_new (DOM_TYPE_DOM_IMPLEMENTATION, NULL);
}
