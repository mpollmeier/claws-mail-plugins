/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
   Copyright (C) 2000-2001 CodeFactory AB
   Copyright (C) 2000-2001 Jonas Borgström <jonas@codefactory.se>
   Copyright (C) 2000-2001 Anders Carlsson <andersca@codefactory.se>
   
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
#include "layout/html/htmlboxembeddedselect.h"
#include "dom/html/dom-htmlinputelement.h"
#include "dom/html/dom-htmlselectelement.h"
#include "dom/html/dom-htmloptionelement.h"

static HtmlBoxClass *parent_class = NULL;

static void
html_box_embedded_select_relayout (HtmlBox *box, HtmlRelayout *relayout) 
{
	DomHTMLSelectElement *select_node = DOM_HTML_SELECT_ELEMENT (box->dom_node);
	HtmlStyle *style = HTML_BOX_GET_STYLE (box);
	GtkWidget *widget = HTML_BOX_EMBEDDED (box)->widget;

	if (dom_HTMLSelectElement__get_multiple (select_node) == FALSE &&
	    dom_HTMLSelectElement__get_size (select_node) == 1) {
	}
	else {
		/* FIXME: This is not correct... */
		gtk_widget_set_usize (widget, -1, (1 + style->inherited->font_spec->size) * 
				      dom_HTMLSelectElement__get_size (select_node) + 5);
	}
}

static gboolean 
combo_select_default (GtkTreeModel *model, GtkTreePath *path,
		      GtkTreeIter *iter, gpointer data)
{
	DomHTMLOptionElement *element;
	GtkComboBox *widget = GTK_COMBO_BOX (data);

	gtk_tree_model_get (model, iter, 2, &element, -1);

	if (dom_HTMLOptionElement__get_defaultSelected (element)) {
		gtk_combo_box_set_active_iter (widget, iter);
	}

	return FALSE;
}

static gboolean 
treeview_select_default (GtkTreeModel *model, GtkTreePath *path,
			 GtkTreeIter *iter, gpointer data)
{
	DomHTMLOptionElement *element;
	GtkTreeView *treeview = GTK_TREE_VIEW (data);

	gtk_tree_model_get (model, iter, 2, &element, -1);

	if (dom_HTMLOptionElement__get_defaultSelected (element)) {
		gtk_tree_selection_select_iter (gtk_tree_view_get_selection (treeview), iter);
		gtk_tree_view_scroll_to_cell (treeview, path, NULL, TRUE,
					      0.5, 0.0);
	}

	return FALSE;
}

static void
update_treeview_selection (GtkWidget *widget, GtkRequisition *req,
			   gpointer data)
{
	GtkTreeModel *model;

	model = GTK_TREE_MODEL (data);

	if (GTK_IS_COMBO_BOX (widget)) {
		gtk_tree_model_foreach (model, combo_select_default, widget);
		if (gtk_combo_box_get_active (GTK_COMBO_BOX (widget)) < 0)
		    gtk_combo_box_set_active (GTK_COMBO_BOX (widget), 0);
	} else {
		gtk_tree_model_foreach (model, treeview_select_default, widget);
	}
}

static void
create_treeview_widget (HtmlBoxEmbedded *embedded, DomHTMLSelectElement *select_node)
{
	GtkCellRenderer *cell;
	GtkTreeViewColumn *column;
	GtkTreeModel *model;
	GtkWidget *treeview;

	html_box_embedded_set_widget (embedded, gtk_scrolled_window_new (NULL, NULL));

	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (embedded->widget),
					GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (embedded->widget),
					     GTK_SHADOW_IN);

	model = dom_html_select_element_get_tree_model (select_node);
	treeview = gtk_tree_view_new_with_model (model);

	if (dom_HTMLSelectElement__get_multiple (select_node))
		gtk_tree_selection_set_mode (gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview)), GTK_SELECTION_MULTIPLE);

	gtk_container_add (GTK_CONTAINER (embedded->widget), treeview);
	gtk_widget_show (treeview);

	cell = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes ("Select", cell, "text", 0, NULL);

	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (treeview), FALSE);

	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), GTK_TREE_VIEW_COLUMN (column));

	g_signal_connect (G_OBJECT (treeview), "size_request",
			  G_CALLBACK (update_treeview_selection), model);
}

static void
combo_selection_changed (GtkComboBox *widget, gpointer data)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	gchar *value;
	HtmlBox *box = HTML_BOX (data);

	model = gtk_combo_box_get_model (widget);
	gtk_combo_box_get_active_iter (widget, &iter);

	gtk_tree_model_get (model, &iter, 1, &value, -1);

	/* This is wrong, it is an invalid cast, need to fix this soon */
	dom_HTMLSelectElement__set_value (DOM_HTML_SELECT_ELEMENT (box->dom_node), value);

	g_free (value);
}

static void
row_changed_callback (GtkTreeModel *model, GtkTreePath *path,
		      GtkTreeIter *iter, gpointer data)
{
	GtkWidget *widget = GTK_WIDGET (data);

	update_treeview_selection (widget, NULL, model);
}

static void
create_combo_widget (HtmlBoxEmbedded *embedded, DomHTMLSelectElement *select_node)
{
	GtkWidget *widget;
	GtkTreeModel *model;
	GtkCellRenderer *renderer;

	model = dom_html_select_element_get_tree_model (select_node);
	widget = gtk_combo_box_new ();

	renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (widget), renderer, FALSE);
	gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (widget), renderer, "text", 0);

	gtk_combo_box_set_model (GTK_COMBO_BOX (widget), model);

	html_box_embedded_set_widget (embedded, widget);

	g_signal_connect (G_OBJECT (model), "row_changed",
			  (GCallback) row_changed_callback, widget);
	g_signal_connect (G_OBJECT (widget), "changed",
			  G_CALLBACK (combo_selection_changed), embedded);
}

static void
html_box_embedded_select_finalize (GObject *object)
{
	HtmlBox *box = HTML_BOX (object);
	DomHTMLSelectElement *select_node;

	if (box->dom_node) {
		GtkTreeModel *model;
		select_node = DOM_HTML_SELECT_ELEMENT (box->dom_node);
		model = dom_html_select_element_get_tree_model (select_node);

		if (dom_HTMLSelectElement__get_multiple (select_node) == FALSE &&
		    dom_HTMLSelectElement__get_size (select_node) == 1) {

			g_signal_handlers_disconnect_matched (G_OBJECT (model),
							      G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA,
							      0, 0, NULL, row_changed_callback, box);
		}
	}
	G_OBJECT_CLASS(parent_class)->finalize (object);
}

static void
html_box_embedded_select_class_init (HtmlBoxClass *klass)
{
	GObjectClass *object_class = (GObjectClass *)klass;

	klass->relayout = html_box_embedded_select_relayout;
	object_class->finalize = html_box_embedded_select_finalize;

	parent_class = g_type_class_peek_parent (klass);
}

static void
html_box_embedded_select_init (HtmlBoxEmbeddedSelect *select)
{
}

GType
html_box_embedded_select_get_type (void)
{
	static GType html_type = 0;
	
	if (!html_type) {
		static GTypeInfo type_info = {
			sizeof (HtmlBoxEmbeddedSelectClass),
			NULL,
			NULL,
			(GClassInitFunc) html_box_embedded_select_class_init,		       
			NULL,
			NULL,
			sizeof (HtmlBoxEmbeddedSelect),
			16, 
			(GInstanceInitFunc) html_box_embedded_select_init
		};
		html_type = g_type_register_static (HTML_TYPE_BOX_EMBEDDED, "HtmlBoxEmbeddedSelect", &type_info, 0);
	}
	return html_type;
}

HtmlBox *
html_box_embedded_select_new (HtmlView *view, DomNode *node)
{
	HtmlBoxEmbeddedSelect *result;
	HtmlBoxEmbedded *embedded;
	DomHTMLSelectElement *select_node = DOM_HTML_SELECT_ELEMENT (node);

	result = g_object_new (HTML_TYPE_BOX_EMBEDDED_SELECT, NULL);
	embedded = HTML_BOX_EMBEDDED (result);
	
	html_box_embedded_set_view (embedded, view);

	if (dom_HTMLSelectElement__get_multiple (select_node) ||
	    dom_HTMLSelectElement__get_size (select_node) > 1)
		create_treeview_widget (embedded, select_node);
	else
		create_combo_widget (embedded, select_node);

	html_box_embedded_set_descent (HTML_BOX_EMBEDDED (result), 4);

	return HTML_BOX (result);
}


