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
#include <string.h>
#include <stdlib.h>
#include "layout/htmlbox.h"
#include "layout/html/htmlboxembedded.h"
#include "view/htmlview.h"
#include "gtk/gtkutils.h"

static HtmlBoxClass *parent_class = NULL;

/**
 * html_box_embedded_find_form:
 * @embedded: 
 * 
 * This function searches for the form box that this element belongs to.
 * It will update embedded->form width the value found. If it cant
 * find any form, then it will set the variable to NULL
 *
 **/
static void
html_box_embedded_find_form (HtmlBoxEmbedded *embedded)
{
	HtmlBox *form = HTML_BOX (embedded)->parent;
	
	if (embedded->form == NULL) {

		while (form && !HTML_IS_BOX_FORM (form))
			form = form->parent;
		
		if (form)
			embedded->form = HTML_BOX_FORM (form);
	}
}

static void
html_box_embedded_finalize (GObject *object)
{
	HtmlBoxEmbedded *embedded = HTML_BOX_EMBEDDED (object);

	if (embedded->widget)
		gtk_widget_destroy (embedded->widget);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
html_box_embedded_relayout (HtmlBox *self, HtmlRelayout *relayout)
{
	html_box_embedded_find_form (HTML_BOX_EMBEDDED (self));
}

static void
html_box_embedded_paint (HtmlBox *self, HtmlPainter *painter, GdkRectangle *area, gint tx, gint ty)
{
	HtmlBoxEmbedded *embedded = HTML_BOX_EMBEDDED (self);

	if (embedded->widget) {

		gint new_x, new_y;

		g_return_if_fail (embedded->view != NULL);

		new_x = tx + self->x + html_box_left_mbp_sum (self, -1);
		new_y = ty + self->y + html_box_top_mbp_sum (self, -1);
	
		if(new_x != embedded->abs_x || new_y != embedded->abs_y) {
			
			gtk_layout_move (GTK_LAYOUT (embedded->view), embedded->widget, new_x, new_y);
			if (!gtkut_widget_get_visible(embedded->widget))
				gtk_widget_show (embedded->widget);
			embedded->abs_x = new_x;
			embedded->abs_y = new_y;
		}
	}
}

static void
allocate (GtkWidget *w, GtkAllocation  *allocation, HtmlBox *self)
{
	if (self->width  != allocation->width || 
	    self->height != allocation->height) {

		HtmlBoxEmbedded *embedded = HTML_BOX_EMBEDDED (self);

		self->width  = allocation->width;
		self->height = allocation->height;

		if (embedded->view)
			g_signal_emit_by_name (G_OBJECT (embedded->view->document), 
				       "relayout_node", self->dom_node);
	}
}


/**
 * html_box_embedded_set_widget:
 * @embedded: The layout box
 * @widget: The widget to embed
 * 
 *  This function sets which widget the layout box should embed
 **/
void
html_box_embedded_set_widget (HtmlBoxEmbedded *embedded, GtkWidget *widget)
{
	embedded->widget = widget;

	g_object_set_data (G_OBJECT (widget), "box", embedded);
			   
	g_signal_connect (G_OBJECT (widget), "size_allocate",
			  G_CALLBACK (allocate), embedded);
	
	if (embedded->view)
		gtk_layout_put (GTK_LAYOUT (embedded->view), embedded->widget,
				embedded->abs_x, embedded->abs_y);
}

/**
 * html_box_embedded_set_view:
 * @embedded: The layout box
 * @view: The htmlview that the embedded box will be embedded in.
 * 
 * The embedded box need to know which gtklayout the gtkwidget should be embedded in.
 **/
void
html_box_embedded_set_view (HtmlBoxEmbedded *embedded, HtmlView *view)
{
	embedded->view = view;

	if (embedded->widget)
		gtk_layout_put (GTK_LAYOUT (embedded->view), embedded->widget,
				embedded->abs_x, embedded->abs_y);
	
}

static gint 
get_ascent (HtmlBox *box)
{
	HtmlBoxEmbedded *embedded = HTML_BOX_EMBEDDED (box);
	return box->height - embedded->descent;
}

static gint 
get_descent (HtmlBox *box)
{
	HtmlBoxEmbedded *embedded = HTML_BOX_EMBEDDED (box);
	return embedded->descent;
}

gint 
html_box_embedded_get_descent (HtmlBoxEmbedded *embedded)
{
	g_return_val_if_fail (embedded != NULL, 0);
	return embedded->descent;
}

void
html_box_embedded_set_descent (HtmlBoxEmbedded *embedded, gint descent)
{
	g_return_if_fail (embedded != NULL);
	embedded->descent = descent;
}

static void
html_box_embedded_class_init (HtmlBoxEmbeddedClass *klass)
{
	HtmlBoxClass *box_class = (HtmlBoxClass *)klass;
        GObjectClass *object_class = (GObjectClass *)klass;

	box_class->paint = html_box_embedded_paint;
	box_class->relayout = html_box_embedded_relayout;
	box_class->get_ascent = get_ascent;
	box_class->get_descent = get_descent;

	object_class->finalize = html_box_embedded_finalize;

	parent_class = g_type_class_peek_parent (klass);
}

static void
html_box_embedded_init (HtmlBoxEmbedded *embedded)
{
}

GType
html_box_embedded_get_type (void)
{
	static GType html_type = 0;
	
	if (!html_type) {
		static GTypeInfo type_info = {
			sizeof (HtmlBoxEmbeddedClass),
			NULL,
			NULL,
			(GClassInitFunc) html_box_embedded_class_init,		       
			NULL,
			NULL,
			sizeof (HtmlBoxEmbedded),
			16, 
			(GInstanceInitFunc) html_box_embedded_init
		};
		
		html_type = g_type_register_static (HTML_TYPE_BOX, "HtmlBoxEmbedded", &type_info, 0);
	}
       
	return html_type;
}

