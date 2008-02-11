/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2000-2001 CodeFactory AB
 * Copyright (C) 2000-2001 Anders Carlsson <andersca@codefactory.se>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "dom-documentview.h"

/**
 * dom_DocumentView__get_defaultView:
 * @view: a DomDocumentView
 * 
 * Returns the default abstract view for this DomDocument or NULL if none is available.
 * 
 * Return value: The default abstract view for this document.
 **/
DomAbstractView *
dom_DocumentView__get_defaultView (DomDocumentView *view)
{
	return DOM_DOCUMENT_VIEW_GET_IFACE (view)->_get_defaultView (view);
}

GType
dom_document_view_get_type (void)
{
	static GType dom_document_view_type = 0;

	if (!dom_document_view_type) {
		static const GTypeInfo dom_document_view_info = {
			sizeof (DomDocumentViewIface), /* class_size */
			NULL, /* base_init */
			NULL, /* base_finalize */
			NULL, /* class_init */
			NULL, /* class_finalize */
			NULL, /* class_data */
			0,
			0,   /* n_preallocs */
			NULL
		};

		dom_document_view_type = g_type_register_static (G_TYPE_OBJECT, "DomDocumentView", &dom_document_view_info, 0);
		g_type_interface_add_prerequisite (dom_document_view_type, G_TYPE_OBJECT);
	}

	return dom_document_view_type;
}
