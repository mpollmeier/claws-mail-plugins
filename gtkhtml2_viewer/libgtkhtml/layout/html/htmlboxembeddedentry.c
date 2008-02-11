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

#include <string.h>
#include <stdlib.h>
#include "dom/html/dom-htmlinputelement.h"
#include "layout/html/htmlboxembeddedentry.h"

static HtmlBoxClass *parent_class = NULL;

static void 
widget_text_changed (DomHTMLInputElement *input, gpointer user_data)
{
	HtmlBox *box = HTML_BOX (user_data);
	HtmlBoxEmbedded *embedded = HTML_BOX_EMBEDDED (user_data);
	gchar *value = dom_HTMLInputElement__get_value (DOM_HTML_INPUT_ELEMENT (box->dom_node));
	
	HTML_BOX_EMBEDDED_ENTRY (user_data)->in_text_changed = TRUE;

	if (strcmp (value, gtk_entry_get_text (GTK_ENTRY (embedded->widget))) != 0) {
		
		gtk_entry_set_text (GTK_ENTRY (embedded->widget), value);
	}
	g_free (value);
	
	HTML_BOX_EMBEDDED_ENTRY (user_data)->in_text_changed = FALSE;
}

static void
changed (GtkEntry *entry, gpointer user_data)
{
	HtmlBox *box = HTML_BOX (user_data);

	if (HTML_BOX_EMBEDDED_ENTRY (user_data)->in_text_changed == FALSE) {

		dom_HTMLInputElement__set_value (DOM_HTML_INPUT_ELEMENT (box->dom_node), gtk_entry_get_text (entry));
		dom_html_input_element_widget_text_changed (DOM_HTML_INPUT_ELEMENT (box->dom_node));
	}
}

static void
html_box_embedded_entry_handle_html_properties (HtmlBox *self, xmlNode *n) 
{
	HtmlBoxEmbedded *embedded = HTML_BOX_EMBEDDED (self);
	HtmlStyle *style = HTML_BOX_GET_STYLE (self);
	gchar *value = dom_HTMLInputElement__get_value (DOM_HTML_INPUT_ELEMENT (HTML_BOX (embedded)->dom_node));
	gint maxlength = dom_HTMLInputElement__get_maxLength (DOM_HTML_INPUT_ELEMENT (HTML_BOX (embedded)->dom_node));
	gchar *size_str;

	if (parent_class->handle_html_properties)
		parent_class->handle_html_properties (self, n);

	gtk_entry_set_max_length (GTK_ENTRY (embedded->widget), maxlength);

	if ((size_str = dom_HTMLInputElement__get_size (DOM_HTML_INPUT_ELEMENT (HTML_BOX (embedded)->dom_node)))) {

		gtk_widget_set_usize (embedded->widget, style->inherited->font_spec->size * atoi (size_str) * 0.66, -1);

		xmlFree (size_str);
	}

	if (dom_HTMLInputElement__get_readOnly (DOM_HTML_INPUT_ELEMENT (self->dom_node)))
		gtk_entry_set_editable (GTK_ENTRY (embedded->widget), FALSE);

	if (value)
		gtk_entry_set_text (GTK_ENTRY (embedded->widget), value);

	g_free (value);

	g_signal_connect (G_OBJECT (self->dom_node), "widget_text_changed", G_CALLBACK (widget_text_changed), self);
	g_signal_connect (G_OBJECT (embedded->widget), "changed", G_CALLBACK (changed), self);
}

static void
html_box_embedded_entry_finalize (GObject *object)
{
	HtmlBox *box = HTML_BOX (object);
	
	if (box->dom_node) {
		g_signal_handlers_disconnect_matched (G_OBJECT (box->dom_node),
						      G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA,
						      0, 0, NULL, widget_text_changed, box);
	}

	G_OBJECT_CLASS(parent_class)->finalize (object);
}

static void
html_box_embedded_entry_class_init (HtmlBoxClass *klass)
{
	GObjectClass *object_class = (GObjectClass *)klass;

	klass->handle_html_properties = html_box_embedded_entry_handle_html_properties;
	object_class->finalize = html_box_embedded_entry_finalize;

	parent_class = g_type_class_peek_parent (klass);
}

static void
html_box_embedded_entry_init (HtmlBoxEmbeddedEntry *entry)
{
}

GType
html_box_embedded_entry_get_type (void)
{
	static GType html_type = 0;
	
	if (!html_type) {
		static GTypeInfo type_info = {
			sizeof (HtmlBoxEmbeddedEntryClass),
			NULL,
			NULL,
			(GClassInitFunc) html_box_embedded_entry_class_init,		       
			NULL,
			NULL,
			sizeof (HtmlBoxEmbeddedEntry),
			16, 
			(GInstanceInitFunc) html_box_embedded_entry_init
		};
		html_type = g_type_register_static (HTML_TYPE_BOX_EMBEDDED, "HtmlBoxEmbeddedEntry", &type_info, 0);
	}
	return html_type;
}

HtmlBox *
html_box_embedded_entry_new (HtmlView *view, HtmlBoxEmbeddedEntryType type)
{
	HtmlBoxEmbeddedEntry *result;

	result = g_object_new (HTML_TYPE_BOX_EMBEDDED_ENTRY, NULL);

	html_box_embedded_set_view (HTML_BOX_EMBEDDED (result), view);
	html_box_embedded_set_descent (HTML_BOX_EMBEDDED (result), 4);
	html_box_embedded_set_widget (HTML_BOX_EMBEDDED (result), gtk_entry_new ());
	GTK_WIDGET_SET_FLAGS(HTML_BOX_EMBEDDED (result)->widget, GTK_CAN_FOCUS);

	if (type == HTML_BOX_EMBEDDED_ENTRY_TYPE_PASSWORD)
		gtk_entry_set_visibility (GTK_ENTRY(HTML_BOX_EMBEDDED (result)->widget), FALSE);

	return HTML_BOX (result);
}


