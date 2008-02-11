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
#include "dom/html/dom-htmltextareaelement.h"
#include "layout/html/htmlboxembeddedtextarea.h"

static HtmlBoxClass *parent_class = NULL;

static void
html_box_embedded_textarea_handle_html_properties (HtmlBox *self, xmlNode *n) 
{
	HtmlBoxEmbedded *embedded = HTML_BOX_EMBEDDED (self);
	HtmlStyle *style = HTML_BOX_GET_STYLE (self);
	GtkWidget *textview;
	gchar *str;
	gint rows = -1, cols = -1;

	textview = gtk_text_view_new ();
	gtk_container_add (GTK_CONTAINER (embedded->widget), textview);
	gtk_widget_show (textview);

	if (parent_class->handle_html_properties)
		parent_class->handle_html_properties (self, n);


	gtk_text_view_set_buffer (GTK_TEXT_VIEW (textview), dom_html_text_area_element_get_text_buffer (DOM_HTML_TEXT_AREA_ELEMENT (HTML_BOX (embedded)->dom_node)));

	gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (textview), GTK_WRAP_WORD);


	if ((str = xmlGetProp (n, "rows"))) {
		/* FIXME */
		rows = atoi (str);
		xmlFree (str);
	}
	if ((str = xmlGetProp (n, "cols"))) {
		/* FIXME */
		cols = atoi (str) / 2;
		xmlFree (str);
	}

	if (rows != -1 && cols != -1) {
		
		gtk_widget_set_usize (embedded->widget, 
				      cols * style->inherited->font_spec->size,
				      rows * style->inherited->font_spec->size + 6);
	}

	if ((str = xmlGetProp (n, "readonly"))) {
		
		gtk_text_view_set_editable (GTK_TEXT_VIEW (textview), FALSE);
		xmlFree (str);
	}
}

static void
html_box_embedded_textarea_class_init (HtmlBoxClass *klass)
{
	klass->handle_html_properties = html_box_embedded_textarea_handle_html_properties;

	parent_class = g_type_class_peek_parent (klass);
}

static void
html_box_embedded_textarea_init (HtmlBoxEmbeddedTextarea *textarea)
{
}

GType
html_box_embedded_textarea_get_type (void)
{
	static GType html_type = 0;
	
	if (!html_type) {
		static GTypeInfo type_info = {
			sizeof (HtmlBoxEmbeddedTextareaClass),
			NULL,
			NULL,
			(GClassInitFunc) html_box_embedded_textarea_class_init,		       
			NULL,
			NULL,
			sizeof (HtmlBoxEmbeddedTextarea),
			16, 
			(GInstanceInitFunc) html_box_embedded_textarea_init
		};
		html_type = g_type_register_static (HTML_TYPE_BOX_EMBEDDED, "HtmlBoxEmbeddedTextarea", &type_info, 0);
	}
	return html_type;
}

HtmlBox *
html_box_embedded_textarea_new (HtmlView *view, DomNode *node)
{
	HtmlBoxEmbeddedTextarea *result;
	HtmlBoxEmbedded *embedded;
	GtkWidget *window;

	result = g_object_new (HTML_TYPE_BOX_EMBEDDED_TEXTAREA, NULL);
	embedded = HTML_BOX_EMBEDDED (result);

	window = gtk_scrolled_window_new (NULL, NULL);

	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (window),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (window),
					     GTK_SHADOW_IN);

	html_box_embedded_set_view (embedded, view);
	html_box_embedded_set_widget (embedded, window);

	return HTML_BOX (result);
}

