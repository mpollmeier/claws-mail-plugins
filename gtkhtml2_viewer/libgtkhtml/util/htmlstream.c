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

#include "htmlstream.h"

static GObjectClass *parent_class = NULL;

static void
html_stream_finalize (GObject *object)
{
	HtmlStream *stream = HTML_STREAM (object);

	if (stream->mime_type)
		g_free (stream->mime_type);

	parent_class->finalize (object);
}

static void
html_stream_class_init (GObjectClass *klass)
{
	klass->finalize = html_stream_finalize;

	parent_class = g_type_class_peek_parent (klass);
}

static void
html_stream_init (HtmlStream *stream)
{
}

GType 
html_stream_get_type (void)
{
	static GType html_type = 0;

	if (!html_type) {
		static GTypeInfo type_info = {
			sizeof (HtmlStreamClass),
			NULL,
			NULL,
			(GClassInitFunc) html_stream_class_init,
			NULL,
			NULL,
			sizeof (HtmlStream),
			16,
			(GInstanceInitFunc) html_stream_init
		};

		html_type = g_type_register_static (G_TYPE_OBJECT, "HtmlStream", &type_info, 0);
	}

	return html_type;
}

void
html_stream_write (HtmlStream *stream,
		   const gchar *buffer,
		   guint size)
{
	g_return_if_fail (stream != NULL);
	g_return_if_fail (buffer != NULL);
	g_return_if_fail (size > 0);

	if (stream->write_func != NULL)
		stream->write_func (stream, buffer, size, stream->user_data);

	stream->written += size;
}

gint
html_stream_get_written (HtmlStream *stream)
{
	g_return_val_if_fail (stream != NULL, 0);

	return stream->written;
}

void
html_stream_destroy (HtmlStream *stream)
{
	g_object_unref (G_OBJECT (stream));
}

void
html_stream_close (HtmlStream *stream)
{
	g_return_if_fail (stream != NULL);
	
	if (stream->close_func != NULL)
		stream->close_func (stream, stream->user_data);
	
	html_stream_destroy (stream);
}

void
html_stream_set_cancel_func (HtmlStream *stream, HtmlStreamCancelFunc cancel_func, gpointer cancel_data)
{
	g_return_if_fail (stream != NULL);
	
	stream->cancel_func = cancel_func;
	stream->cancel_data = cancel_data;
}

void
html_stream_cancel (HtmlStream *stream)
{
	g_return_if_fail (stream != NULL);

	g_return_if_fail (stream->cancel_func != NULL);

	stream->cancel_func (stream, stream->cancel_data, stream->cancel_data);
	html_stream_destroy (stream);
}

/**
 * html_stream_get_mime_type:
 * @stream: a stream
 * 
 * Returns the mimetype of the data or NULL if it is unknown.
 * 
 * Return value: 
 **/
const char *
html_stream_get_mime_type (HtmlStream *stream)
{
	return stream->mime_type;
}

/**
 * html_stream_set_mime_type:
 * @stream: a stream
 * @mime_type: a mime-type
 * 
 * This function can be called if you know the mime-type of the data.
 **/
void
html_stream_set_mime_type (HtmlStream *stream, const char *mime_type)
{
	if (stream->mime_type)
		g_free (stream->mime_type);
	stream->mime_type = g_strdup (mime_type);
}

HtmlStream *
html_stream_new (HtmlStreamWriteFunc write_func, HtmlStreamCloseFunc close_func, gpointer user_data)
{
	HtmlStream *stream;

	stream = (HtmlStream *)g_object_new (HTML_TYPE_STREAM, NULL);

	stream->written = 0;
	stream->write_func = write_func;
	stream->close_func = close_func;
	stream->cancel_func = NULL;
	stream->user_data = user_data;
	
	return stream;
}
