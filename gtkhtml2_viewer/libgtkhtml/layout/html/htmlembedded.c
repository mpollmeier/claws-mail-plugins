/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
   Copyright (C) 2001 CodeFactory AB
   Copyright (C) 2001 Jonas Borgström <jonas@codefactory.se>
   Copyright (C) 2001 Anders Carlsson <andersca@codefactory.se>
   
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

#include "gtk/gtkutils.h"
#include "layout/html/htmlembedded.h"

struct _HtmlEmbeddedPrivate {
	GHashTable * properties;
};

static void html_embedded_destroy (GtkObject *object);

static void
html_embedded_size_request (GtkWidget *widget, GtkRequisition *requisition)
{
	GtkBin *bin;
	
	g_return_if_fail (widget != NULL);
	g_return_if_fail (requisition != NULL);
	
	bin = GTK_BIN (widget);

	if (bin->child) {
		gtk_widget_size_request (bin->child, requisition);
	} else {
		requisition->width = widget->requisition.width;
		requisition->height = widget->requisition.height;
	}
}

static void
html_embedded_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
	GtkBin *bin;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (allocation != NULL);
	
	bin = GTK_BIN (widget);

	if (bin->child && gtkut_widget_get_visible (bin->child)) {
		gtk_widget_size_allocate(bin->child, allocation);
	}
	widget->allocation = *allocation;
}

void 
html_embedded_set_descent (HtmlEmbedded *embedded, gint descent)
{
	g_return_if_fail (embedded != NULL);
	html_box_embedded_set_descent (embedded->box_embedded, descent);
}

gint
html_embedded_get_descent (HtmlEmbedded *embedded)
{
	g_return_val_if_fail (embedded != NULL, 0);
	return html_box_embedded_get_descent (embedded->box_embedded);
}

DomNode *
html_embedded_get_dom_node (HtmlEmbedded *embedded)
{
	g_return_val_if_fail (embedded != NULL, NULL);
	return embedded->node;
}

static void
html_embedded_class_init (GtkWidgetClass *klass)
{
	GtkObjectClass *object_class;

	object_class = GTK_OBJECT_CLASS (klass);

	object_class->destroy = html_embedded_destroy;

	klass->size_request = html_embedded_size_request;
	klass->size_allocate = html_embedded_size_allocate;
}

static void
html_embedded_init (HtmlEmbedded *object)
{
	HtmlEmbeddedPrivate *priv;

	priv = g_new0 (HtmlEmbeddedPrivate, 1);

	priv->properties = g_hash_table_new_full (g_str_hash,
						  g_str_equal,
						  g_free,
						  NULL);

	object->priv = priv;
}

GType
html_embedded_get_type (void)
{
	static GType html_type = 0;
	
	if (!html_type) {
		static GTypeInfo type_info = {
			sizeof (HtmlEmbeddedClass),
			NULL,
			NULL,
			(GClassInitFunc) html_embedded_class_init,
			NULL,
			NULL,
			sizeof (HtmlEmbedded),
			16, 
			(GInstanceInitFunc) html_embedded_init
		};
		
		html_type = g_type_register_static (GTK_TYPE_BIN, "HtmlEmbedded", &type_info, 0);
	}
       
	return html_type;
}

static gboolean
html_embedded_prop_remove (gpointer key, gpointer value, gpointer data)
{
	g_free (value);
	return TRUE;
}

static void
html_embedded_destroy (GtkObject *object)
{
	HtmlEmbeddedPrivate * priv;

	priv = HTML_EMBEDDED (object)->priv;

	g_hash_table_foreach_remove (priv->properties, (GHRFunc)html_embedded_prop_remove, NULL);
}

static void
html_embedded_parse_properties (HtmlEmbedded *embedded)
{
	HtmlEmbeddedPrivate *priv;
	DomNamedNodeMap *nmap;
	DomNodeList *children;
	gchar *name, *value;
	gint i;

	priv = embedded->priv;
	nmap = dom_Node__get_attributes (embedded->node);

	for (i = 0; i < dom_NamedNodeMap__get_length (nmap); i++) {
		DomNode *node;

		node = dom_NamedNodeMap__get_item (nmap, i);

		name = dom_Node__get_localName (node);
		value = xmlGetProp (embedded->node->xmlnode, name);

		if (name && value)
			g_hash_table_insert (priv->properties, g_ascii_strdown (name, strlen (name)), g_strdup (value));

		g_free (name);
		g_free (value);
	}

	children = dom_Node__get_childNodes (embedded->node);
	for (i = 0; i < dom_NodeList__get_length (children); i++) {
		DomNode *param;

		param = dom_NodeList__get_item (children, i);

		if (strcmp (param->xmlnode->name, "param") != 0)
			continue;

		name = xmlGetProp (param->xmlnode, "name");
		value = xmlGetProp (param->xmlnode, "value");

		if (name && value)
			g_hash_table_insert (priv->properties, g_ascii_strdown (name, strlen (name)), g_strdup (value));

		g_free (name);
		g_free (value);
	}
}

GtkWidget *
html_embedded_new (DomNode *node, HtmlBoxEmbedded *box_embedded)
{
	HtmlEmbedded *embedded = g_object_new (HTML_TYPE_EMBEDDED, NULL);

	embedded->node = node;
	embedded->box_embedded = box_embedded;

	html_embedded_parse_properties (embedded);

	return GTK_WIDGET (embedded);
}

gchar *
html_embedded_get_prop (HtmlEmbedded *embedded, const gchar *name)
{
	HtmlEmbeddedPrivate *priv;
	gchar *prop;

	priv = embedded->priv;

	if (!strcmp (name, "src") || !strcmp (name, "movie")) {
		prop = g_hash_table_lookup (priv->properties, "movie");
		if (!prop)
			prop = g_hash_table_lookup (priv->properties, "src");
	} else {
		prop = g_hash_table_lookup (priv->properties, name);
	}

	return g_strdup (prop);
}
