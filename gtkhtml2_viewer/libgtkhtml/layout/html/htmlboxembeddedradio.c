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
#include "layout/html/htmlboxembeddedradio.h"

static HtmlBoxClass *parent_class = NULL;

static void 
html_box_embedded_radio_set_group (HtmlBoxEmbeddedRadio *radio)
{
	HtmlBoxEmbedded *embedded = HTML_BOX_EMBEDDED (radio);
	gchar *name = dom_HTMLInputElement__get_name (DOM_HTML_INPUT_ELEMENT (HTML_BOX (embedded)->dom_node));
	g_return_if_fail (embedded->form != NULL);

	if (name == NULL)
		name = g_strdup ("gtkhtml2defaultradiogroup");

	gtk_radio_button_set_group (GTK_RADIO_BUTTON (embedded->widget), 
				    html_box_form_get_radio_group (embedded->form, name));
	html_box_form_set_radio_group (embedded->form, name, 
				       gtk_radio_button_group (GTK_RADIO_BUTTON (embedded->widget)));
	
	/*Ok, this is ugly but you can't unselect a single radiobutton with gtk_toggle_button_set_active */
	GTK_TOGGLE_BUTTON (embedded->widget)->active = dom_HTMLInputElement__get_checked (DOM_HTML_INPUT_ELEMENT (HTML_BOX (embedded)->dom_node));

	xmlFree (name);
}

static void
html_box_embedded_radio_relayout (HtmlBox *self, HtmlRelayout *relayout) 
{
	HtmlBoxEmbedded *embedded = HTML_BOX_EMBEDDED (self);
	HtmlBoxEmbeddedRadio *radio = HTML_BOX_EMBEDDED_RADIO (self);

	HTML_BOX_CLASS (parent_class)->relayout (self, relayout);

	if (radio->added_to_group == FALSE) {
		html_box_embedded_radio_set_group (radio);
		radio->added_to_group = TRUE;

		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (embedded->widget), 
					      dom_HTMLInputElement__get_checked (DOM_HTML_INPUT_ELEMENT (HTML_BOX (embedded)->dom_node)));
	}
}

static void 
widget_toggled (DomHTMLInputElement *input, gboolean checked, gpointer user_data)
{
	HtmlBoxEmbedded *embedded = HTML_BOX_EMBEDDED (user_data);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (embedded->widget), checked);
}

static void
toggled (GtkToggleButton *togglebutton, gpointer user_data)
{
	HtmlBox *box = HTML_BOX (user_data);
	HtmlBoxEmbedded *embedded = HTML_BOX_EMBEDDED (user_data);
	
	dom_html_input_element_widget_toggled (DOM_HTML_INPUT_ELEMENT (box->dom_node), 
					       gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (embedded->widget)));
}

static void
html_box_embedded_radio_handle_html_properties (HtmlBox *self, xmlNode *n) 
{
	HtmlBoxEmbedded *embedded = HTML_BOX_EMBEDDED (self);

	if (parent_class->handle_html_properties)
		parent_class->handle_html_properties (self, n);

	g_signal_connect (G_OBJECT (self->dom_node), "widget_toggled", G_CALLBACK (widget_toggled), self);
	g_signal_connect (G_OBJECT (embedded->widget), "toggled", G_CALLBACK (toggled), self);
}

static void
html_box_embedded_radio_finalize (GObject *object)
{
	HtmlBox *box = HTML_BOX (object);
	
	if (box->dom_node) {
		g_signal_handlers_disconnect_matched (G_OBJECT (box->dom_node),
						      G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA,
						      0, 0, NULL, widget_toggled, box);
	}

	G_OBJECT_CLASS(parent_class)->finalize (object);
}

static void
html_box_embedded_radio_class_init (HtmlBoxClass *klass)
{
	GObjectClass *object_class = (GObjectClass *)klass;

	klass->handle_html_properties = html_box_embedded_radio_handle_html_properties;
	klass->relayout = html_box_embedded_radio_relayout;
	object_class->finalize = html_box_embedded_radio_finalize;

	parent_class = g_type_class_peek_parent (klass);
}

static void
html_box_embedded_radio_init (HtmlBoxEmbeddedRadio *radio)
{
}

GType
html_box_embedded_radio_get_type (void)
{
	static GType html_type = 0;
	
	if (!html_type) {
		static GTypeInfo type_info = {
			sizeof (HtmlBoxEmbeddedRadioClass),
			NULL,
			NULL,
			(GClassInitFunc) html_box_embedded_radio_class_init,		       
			NULL,
			NULL,
			sizeof (HtmlBoxEmbeddedRadio),
			16, 
			(GInstanceInitFunc) html_box_embedded_radio_init
		};
		
		html_type = g_type_register_static (HTML_TYPE_BOX_EMBEDDED, "HtmlBoxEmbeddedRadio", &type_info, 0);
	}
	return html_type;
}

HtmlBox *
html_box_embedded_radio_new (HtmlView *view)
{
	HtmlBoxEmbeddedRadio *result;

	result = HTML_BOX_EMBEDDED_RADIO (g_type_create_instance (HTML_TYPE_BOX_EMBEDDED_RADIO));

	html_box_embedded_set_descent (HTML_BOX_EMBEDDED (result), 4);
	html_box_embedded_set_view (HTML_BOX_EMBEDDED (result), view);
	html_box_embedded_set_widget (HTML_BOX_EMBEDDED (result), gtk_radio_button_new (NULL));

	return HTML_BOX (result);
}


