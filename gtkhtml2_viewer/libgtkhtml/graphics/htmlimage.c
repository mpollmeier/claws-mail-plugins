/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
   Copyright (C) 2000 CodeFactory AB
   Copyright (C) 2000 Jonas Borgstr\366m <jonas@codefactory.se>
   Copyright (C) 2000 Anders Carlsson <andersca@codefactory.se>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include <string.h>
#include "htmlimage.h"
#include "util/htmlmarshal.h"

enum {
	REPAINT_IMAGE,
	RESIZE_IMAGE,
	LAST_UNREF,
	
	LAST_SIGNAL
};

static guint image_signals [LAST_SIGNAL] = { 0 };

static GObjectClass *image_parent_class = NULL;

static void
html_image_dispose (GObject *image)
{
	g_signal_emit (G_OBJECT (image), image_signals [LAST_UNREF], FALSE);

	G_OBJECT_CLASS (image_parent_class)->dispose (G_OBJECT (image));
}

static void
html_image_area_updated (GdkPixbufLoader *loader, gint x, gint y, gint width, gint height, HtmlImage *image)
{
	g_signal_emit (G_OBJECT (image), image_signals [REPAINT_IMAGE], 0, x, y, width, height);
}

static void
html_image_closed (GdkPixbufLoader *loader, HtmlImage *image)
{
}

static void
html_image_area_prepared (GdkPixbufLoader *loader, HtmlImage *image)
{
	GdkPixbufAnimation *animation;

	animation = gdk_pixbuf_loader_get_animation (loader);

	if (gdk_pixbuf_animation_is_static_image (animation)) {
		image->pixbuf = gdk_pixbuf_ref (gdk_pixbuf_loader_get_pixbuf (loader));
	}

	g_signal_emit (G_OBJECT (image), image_signals [RESIZE_IMAGE], 0);
}

static void
html_image_finalize (GObject *object)
{
	HtmlImage *image = HTML_IMAGE (object);

	g_free (image->uri);
	
	if (image->pixbuf)
		gdk_pixbuf_unref (image->pixbuf);

	if (image->loader) {
		g_signal_handlers_disconnect_by_func (image->loader, 
			   (void *) html_image_area_prepared, image);
		g_signal_handlers_disconnect_by_func (image->loader, 
			   (void *) html_image_area_updated, image);
		gdk_pixbuf_loader_close (image->loader, NULL);
		g_object_unref (G_OBJECT (image->loader));
	}
	
	if (image->stream)
		html_stream_cancel (image->stream);

	G_OBJECT_CLASS (image_parent_class)->finalize (object);
}

static void
html_image_class_init (HtmlImageClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	image_parent_class = g_type_class_peek_parent (klass);

	object_class->finalize = html_image_finalize;
	object_class->dispose = html_image_dispose;
	
	image_signals [REPAINT_IMAGE] =
		g_signal_new ("repaint_image",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (HtmlImageClass, repaint_image),
			      NULL, NULL,
			      html_marshal_VOID__INT_INT_INT_INT,
			      G_TYPE_NONE,
			      4,
			      G_TYPE_INT, G_TYPE_INT,
			      G_TYPE_INT, G_TYPE_INT);

	image_signals [RESIZE_IMAGE] =
		g_signal_new ("resize_image",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (HtmlImageClass, resize_image),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
	
	image_signals [LAST_UNREF] =
		g_signal_new ("last_unref",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (HtmlImageClass, last_unref),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
}


static void
html_image_init (HtmlImage *image)
{
	image->pixbuf = NULL;
	image->stream = NULL;
	image->broken = FALSE;
	image->loading = FALSE;
	image->loader = gdk_pixbuf_loader_new ();

	g_signal_connect (G_OBJECT (image->loader), "area_prepared",
			   G_CALLBACK (html_image_area_prepared), image);
	g_signal_connect (G_OBJECT (image->loader), "area_updated",
			   G_CALLBACK (html_image_area_updated), image);
	g_signal_connect (G_OBJECT (image->loader), "closed",
			   G_CALLBACK (html_image_closed), image);

	
}

GType
html_image_get_type (void)
{
	static GType html_image_type = 0;

	if (!html_image_type) {
		GTypeInfo html_image_info = {
			sizeof (HtmlImageClass),
			NULL,
			NULL,
			(GClassInitFunc) html_image_class_init,
			NULL,
			NULL,
			sizeof (HtmlImage),
			1,
			(GInstanceInitFunc) html_image_init,
		};

		html_image_type = g_type_register_static (G_TYPE_OBJECT, "HtmlImage", &html_image_info, 0);
	}
	return html_image_type;
}

gint
html_image_get_width (HtmlImage *image)
{
	if (image->pixbuf)
		return gdk_pixbuf_get_width (image->pixbuf);
	else 
		return 0;
}

gint
html_image_get_height (HtmlImage *image)
{
	if (image->pixbuf)
		return gdk_pixbuf_get_height (image->pixbuf);
	else
		return 0;

}

