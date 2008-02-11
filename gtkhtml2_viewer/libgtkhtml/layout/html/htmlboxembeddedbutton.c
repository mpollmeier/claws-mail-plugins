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
#include "layout/html/htmlboxembeddedbutton.h"

static HtmlBoxClass *parent_class = NULL;

static void
html_box_embedded_button_clicked (GtkWidget *widget, HtmlBoxEmbedded *embedded)
{
	g_return_if_fail (embedded->form != NULL);

	DOM_HTML_INPUT_ELEMENT (HTML_BOX (embedded)->dom_node)->active = TRUE;

	switch (HTML_BOX_EMBEDDED_BUTTON (embedded)->type) {
	case HTML_BOX_EMBEDDED_BUTTON_TYPE_SUBMIT:
		if (embedded->form)
			dom_HTMLFormElement_submit (DOM_HTML_FORM_ELEMENT (HTML_BOX (embedded->form)->dom_node));
		break;
	case HTML_BOX_EMBEDDED_BUTTON_TYPE_RESET:
		if (embedded->form)
			dom_HTMLFormElement_reset (DOM_HTML_FORM_ELEMENT (HTML_BOX (embedded->form)->dom_node));
		break;
	default:
		g_warning("html_box_embedded_button_clicked: impossible!\n");
		return;
	}
	DOM_HTML_INPUT_ELEMENT (HTML_BOX (embedded)->dom_node)->active = FALSE;
}

static void
html_box_embedded_button_set_label (HtmlBoxEmbeddedButton *button)
{
	HtmlBoxEmbedded *embedded = HTML_BOX_EMBEDDED (button);
	gchar *value = dom_HTMLInputElement__get_value (DOM_HTML_INPUT_ELEMENT (HTML_BOX (embedded)->dom_node));

	if (value && strlen (value))
		gtk_label_set_text (GTK_LABEL (GTK_BIN (embedded->widget)->child), value);
	else {
		switch (button->type) {
		case HTML_BOX_EMBEDDED_BUTTON_TYPE_SUBMIT:
			gtk_label_set_text (GTK_LABEL (GTK_BIN (embedded->widget)->child), "Submit");
			break;
		case HTML_BOX_EMBEDDED_BUTTON_TYPE_RESET:
			gtk_label_set_text (GTK_LABEL (GTK_BIN (embedded->widget)->child), "Reset");
			break;
		}
	}
	g_free (value);
}

static void
html_box_embedded_button_handle_html_properties (HtmlBox *self, xmlNode *n) 
{
	HtmlBoxEmbeddedButton *button = HTML_BOX_EMBEDDED_BUTTON (self);

	if (parent_class->handle_html_properties)
		parent_class->handle_html_properties (self, n);

	html_box_embedded_button_set_label (button);
}

static void
html_box_embedded_button_class_init (HtmlBoxClass *klass)
{
	klass->handle_html_properties = html_box_embedded_button_handle_html_properties;

	parent_class = g_type_class_peek_parent (klass);
}

static void
html_box_embedded_button_init (HtmlBoxEmbeddedButton *button)
{
}

GType
html_box_embedded_button_get_type (void)
{
	static GType html_type = 0;
	
	if (!html_type) {
		static GTypeInfo type_info = {
			sizeof (HtmlBoxEmbeddedButtonClass),
			NULL,
			NULL,
			(GClassInitFunc) html_box_embedded_button_class_init,		       
			NULL,
			NULL,
			sizeof (HtmlBoxEmbeddedButton),
			16, 
			(GInstanceInitFunc) html_box_embedded_button_init
		};
		
		html_type = g_type_register_static (HTML_TYPE_BOX_EMBEDDED, "HtmlBoxEmbeddedButton", &type_info, 0);
	}
       
	return html_type;
}

HtmlBox *
html_box_embedded_button_new (HtmlView *view, HtmlBoxEmbeddedButtonType type)
{
	HtmlBoxEmbeddedButton *result;

	result = g_object_new (HTML_TYPE_BOX_EMBEDDED_BUTTON, NULL);

	html_box_embedded_set_view (HTML_BOX_EMBEDDED (result), view);
	html_box_embedded_set_widget (HTML_BOX_EMBEDDED (result), gtk_button_new_with_label ("button"));
	result->type = type;

	g_signal_connect (HTML_BOX_EMBEDDED (result)->widget, "clicked", 
			  G_CALLBACK (html_box_embedded_button_clicked), result);

	html_box_embedded_set_descent (HTML_BOX_EMBEDDED (result), 4);

	return HTML_BOX (result);
}


