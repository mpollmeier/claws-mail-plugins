/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
   Copyright (C) 2000 CodeFactory AB
   Copyright (C) 2000 Jonas Borgström <jonas@codefactory.se>
   Copyright (C) 2000 Anders Carlsson <andersca@codefactory.se>
   
   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.
   
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.
   
   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include <gtk/gtk.h>
#include "util/htmlglobalatoms.h"
#include "gtkhtmlcontext.h"
#include "document/htmldocument.h"
#include "graphics/htmlfontspecification.h"

enum {
	PROP_ZERO,
	PROP_DEBUG_PAINTING
};

static void
gtk_html_context_set_property (GObject *object, guint param_id, const GValue *value, GParamSpec *pspec)
{
	GtkHtmlContext *context = GTK_HTML_CONTEXT (object);
	GSList *doc_list;
	
	switch (param_id) {
	case PROP_DEBUG_PAINTING:
		context->debug_painting = g_value_get_boolean (value);
		for (doc_list = context->documents; doc_list; doc_list = doc_list->next) {
			g_signal_emit_by_name (G_OBJECT (doc_list->data), "style_updated", HTML_DOCUMENT (doc_list->data)->dom_document,
					       HTML_STYLE_CHANGE_REPAINT);
		}
		
		g_object_notify (object, "debug_painting");
		
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
	}
}

static void
gtk_html_context_get_property (GObject *object, guint param_id, GValue *value, GParamSpec *pspec)
{
	GtkHtmlContext *context = GTK_HTML_CONTEXT (object);

	switch (param_id) {
	case PROP_DEBUG_PAINTING:
		g_value_set_boolean (value, context->debug_painting);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
	}
}

static void
gtk_html_context_class_init (GtkHtmlContextClass *klass)
{
	GObjectClass *object_class = (GObjectClass *)klass;
	
	object_class->set_property = gtk_html_context_set_property;
	object_class->get_property = gtk_html_context_get_property;
	
	g_object_class_install_property (object_class,
					 PROP_DEBUG_PAINTING,
					 g_param_spec_boolean ("debug_painting",
							       NULL, NULL, FALSE,
							       G_PARAM_READABLE | G_PARAM_WRITABLE));
}

static void
gtk_html_context_init (GtkHtmlContext *html)
{
	html->documents = NULL;

	html_atom_list_initialize ();
	
}

GType
gtk_html_context_get_type (void)
{
	static GtkType html_type = 0;

	if (!html_type) {
		static const GTypeInfo html_info = {
			sizeof (GtkHtmlContextClass), 
			NULL,           /* base_init */
			NULL,           /* base_finalize */
			(GClassInitFunc) gtk_html_context_class_init,
			NULL,           /* class_finalize */
			NULL,           /* class_data */
			sizeof (GtkHtmlContext),
			1,              /* n_preallocs */
			(GInstanceInitFunc) gtk_html_context_init,
		};

		html_type = g_type_register_static (G_TYPE_OBJECT, "GtkHtmlContext", &html_info, 0);
	}

	return html_type;
}


GtkHtmlContext *
gtk_html_context_get (void)
{
	static GtkHtmlContext *context = NULL;

	if (context == NULL) {
		context = g_object_new (GTK_HTML_CONTEXT_TYPE, NULL);
	}

	return context;
}



