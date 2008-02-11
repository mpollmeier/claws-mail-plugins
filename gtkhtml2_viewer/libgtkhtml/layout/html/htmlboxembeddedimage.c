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
#include "layout/html/htmlboxembeddedimage.h"

static HtmlBoxClass *parent_class = NULL;

static void html_box_embedded_image_resize_image (HtmlImage *image,
						  HtmlBoxEmbeddedImage *box);
static void html_box_embedded_image_repaint_image (HtmlImage *image,
						   gint x, gint y,
						   gint width, gint height,
						   HtmlBoxEmbeddedImage *box);

static void
html_box_embedded_image_paint (HtmlBox *box, HtmlPainter *painter, GdkRectangle *area, gint tx, gint ty)
{
	HtmlBoxEmbeddedImage *image = HTML_BOX_EMBEDDED_IMAGE (box);
	
	if (image->image->pixbuf) {

		GdkPixbuf *pixbuf = image->image->pixbuf;
		gint width = gdk_pixbuf_get_width (pixbuf);
		gint height = gdk_pixbuf_get_height (pixbuf);
		gint x, y;
		
		x = box->x + tx + (box->width - width) / 2;
		y = box->y + ty + (box->height - height) / 2;

		html_painter_draw_pixbuf (painter, area, pixbuf,
					  0, 0, x, y,
					  width, height);
	}
}

static void
html_box_embedded_image_relayout (HtmlBox *box, HtmlRelayout *relayout)
{
	HtmlBoxEmbeddedImage *image = HTML_BOX_EMBEDDED_IMAGE (box);
	gint width = 4, height = 4;

	if (image->image && image->image->pixbuf) {

		width  = gdk_pixbuf_get_width  (image->image->pixbuf);
		height = gdk_pixbuf_get_height (image->image->pixbuf);
	}

	box->width  = width  + html_box_horizontal_mbp_sum (box);
	box->height = height + html_box_vertical_mbp_sum (box);
}

static void
html_box_embedded_image_finalize (GObject *object)
{
	HtmlBoxEmbeddedImage *image = HTML_BOX_EMBEDDED_IMAGE (object);

	g_signal_handlers_disconnect_by_func (G_OBJECT (image->image),
					      G_CALLBACK (html_box_embedded_image_resize_image),
					      image);
	g_signal_handlers_disconnect_by_func (G_OBJECT (image->image),
					      G_CALLBACK (html_box_embedded_image_repaint_image),
					      image);
}

static void
html_box_embedded_image_class_init (HtmlBoxClass *klass)
{
	GObjectClass *object_class = (GObjectClass *)klass;

	klass->paint    = html_box_embedded_image_paint;
	klass->relayout = html_box_embedded_image_relayout;

	object_class->finalize = html_box_embedded_image_finalize;

	parent_class = g_type_class_peek_parent (klass);
}

static void
html_box_embedded_image_resize_image (HtmlImage *image, HtmlBoxEmbeddedImage *box)
{
	g_signal_emit_by_name (G_OBJECT (box->view->document), "relayout_node", HTML_BOX (box)->dom_node);
}

static void
html_box_embedded_image_repaint_image (HtmlImage *image, gint x, gint y, gint width, gint height, HtmlBoxEmbeddedImage *box)
{
	g_signal_emit_by_name (G_OBJECT (box->view->document), "repaint_node", HTML_BOX (box)->dom_node);
}

void
html_box_embedded_image_set_image (HtmlBoxEmbeddedImage *box, HtmlImage *image)
{
	if (box->image)
		g_error ("support image replacing");

	g_signal_connect (G_OBJECT (image), "resize_image",
			  G_CALLBACK (html_box_embedded_image_resize_image), box);
	g_signal_connect (G_OBJECT (image), "repaint_image",
			   G_CALLBACK (html_box_embedded_image_repaint_image), box);
	box->image = image;
}

static void
html_box_embedded_image_init (HtmlBoxEmbeddedImage *image)
{
}

GType
html_box_embedded_image_get_type (void)
{
	static GType html_type = 0;
	
	if (!html_type) {
		static GTypeInfo type_info = {
			sizeof (HtmlBoxEmbeddedImageClass),
			NULL,
			NULL,
			(GClassInitFunc) html_box_embedded_image_class_init,		       
			NULL,
			NULL,
			sizeof (HtmlBoxEmbeddedImage),
			16, 
			(GInstanceInitFunc) html_box_embedded_image_init
		};
		
		html_type = g_type_register_static (HTML_TYPE_BOX_EMBEDDED, "HtmlBoxEmbeddedImage", &type_info, 0);
	}
       
	return html_type;
}

HtmlBox *
html_box_embedded_image_new (HtmlView *view)
{
	HtmlBoxEmbeddedImage *result;

	result = g_object_new (HTML_TYPE_BOX_EMBEDDED_IMAGE, NULL);

	html_box_embedded_set_view (HTML_BOX_EMBEDDED (result), view);

	result->view = view;

	return HTML_BOX (result);
}


