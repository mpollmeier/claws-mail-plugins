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

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "htmlimagefactory.h"
#include "document/htmldocument.h"
#include "layout/html/htmlboximage.h"
#include "util/htmlstream.h"
#include "util/htmlmarshal.h"

static GObjectClass *factory_parent_class = NULL;

enum {
	REQUEST_IMAGE,
	LAST_SIGNAL
};

static guint image_factory_signals [LAST_SIGNAL] = { 0 };

static void
html_image_factory_class_init (HtmlImageFactoryClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	factory_parent_class = g_type_class_peek_parent (klass);

	image_factory_signals [REQUEST_IMAGE] =
		g_signal_new ("request_image",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (HtmlImageFactoryClass, request_image),
			      NULL, NULL,
			      html_marshal_VOID__STRING_POINTER,
			      G_TYPE_NONE,
			      2,
			      G_TYPE_STRING,
			      G_TYPE_POINTER);
}

static void
html_image_factory_init (HtmlImageFactory *image_factory)
{
	image_factory->image_hash = g_hash_table_new (g_str_hash, g_str_equal);
}

GType
html_image_factory_get_type (void)
{
	static GType html_image_factory_type = 0;

	if (!html_image_factory_type) {
		GTypeInfo html_image_factory_info = {
			sizeof (HtmlImageFactoryClass),
			NULL,
			NULL,
			(GClassInitFunc) html_image_factory_class_init,
			NULL,
			NULL,
			sizeof (HtmlImageFactory),
			1,
			(GInstanceInitFunc) html_image_factory_init,
		};

		html_image_factory_type = g_type_register_static (G_TYPE_OBJECT, "HtmlImageFactory", &html_image_factory_info, 0);
	}

	return html_image_factory_type;
}

static void
write_pixbuf (HtmlStream *stream, const gchar *data, guint size, HtmlImage *image)
{
	GError *error = NULL;

	if (!image)
		return;

	gdk_pixbuf_loader_write (image->loader, data, size, &error);

	if (error) {

		g_warning ("gdk_pixbuf_loader_write error: %s\n", error->message);
		g_error_free (error);
	}
}

static void 
close_pixbuf (HtmlStream *stream, HtmlImage *image)
{
	if (!image)
		return;

	image->loading = FALSE;
	
	if (html_stream_get_written (stream) == 0) {
		image->broken = TRUE;
		g_signal_emit_by_name (G_OBJECT (image), "repaint_image", 0, 0,
				       html_image_get_width (image), html_image_get_height (image));

	}
	
	gdk_pixbuf_loader_close (image->loader, NULL);
	g_object_unref (G_OBJECT (image->loader));
	image->loader = NULL;
	image->stream = NULL;
}

static void
html_image_shutdown (HtmlImageFactory *image_factory, HtmlImage *image)
{
	g_hash_table_remove (image_factory->image_hash, image->uri);
}

HtmlImage *
html_image_factory_get_image (HtmlImageFactory *image_factory, const gchar *uri)
{
	HtmlImage *image;

	image = g_hash_table_lookup (image_factory->image_hash, uri);
	
	if (image)
		image = (HtmlImage *)g_object_ref (G_OBJECT (image));
	else {
		HtmlStream *stream;
		
		image = HTML_IMAGE (g_object_new (HTML_IMAGE_TYPE, NULL));

		g_signal_connect_swapped (G_OBJECT (image), "last_unref", G_CALLBACK (html_image_shutdown), image_factory);
		
		image->loading = TRUE;
		
		stream = html_stream_new ((HtmlStreamWriteFunc)write_pixbuf, (HtmlStreamCloseFunc)close_pixbuf,
					  image);

		image->stream = stream;
		g_object_add_weak_pointer (G_OBJECT (stream), (gpointer *) &(image->stream));
		g_object_add_weak_pointer (G_OBJECT (image), (gpointer *) &(stream->user_data));

		g_signal_emit (G_OBJECT (image_factory), image_factory_signals [REQUEST_IMAGE], 0, uri, stream);
		
		image->uri = g_strdup (uri);
		g_hash_table_insert (image_factory->image_hash, image->uri, image);
	}

	return image;
}

HtmlImageFactory *
html_image_factory_new (void)
{
	HtmlImageFactory *image_factory = HTML_IMAGE_FACTORY (g_object_new (HTML_TYPE_IMAGE_FACTORY, NULL));

	return image_factory;
}
