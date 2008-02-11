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

#include "htmlstreambuffer.h"

typedef struct _HtmlStreamBuffer HtmlStreamBuffer;

struct _HtmlStreamBuffer {
	GString *buffer;
	gpointer user_data;
	HtmlStreamBufferCloseFunc close_func;
};


static void
html_stream_buffer_write (HtmlStream *stream, const gchar *buffer, guint size, gpointer user_data)
{
	HtmlStreamBuffer *stream_buffer = (HtmlStreamBuffer *)user_data;

	if (!stream_buffer->buffer)
		stream_buffer->buffer = g_string_new_len (buffer, size);
	else
		g_string_append_len (stream_buffer->buffer, buffer, size);
}

static void
html_stream_buffer_close (HtmlStream *stream, gpointer user_data)
{
	HtmlStreamBuffer *stream_buffer = (HtmlStreamBuffer *)user_data;

	if (stream_buffer->buffer) {
		stream_buffer->close_func (stream_buffer->buffer->str, stream_buffer->buffer->len, stream_buffer->user_data);

		g_string_free (stream_buffer->buffer, TRUE);
	}
	else
		stream_buffer->close_func (NULL, -1, stream_buffer->user_data);
	g_free (user_data);
}

HtmlStream *
html_stream_buffer_new (HtmlStreamBufferCloseFunc close_func, gpointer user_data)
{
	HtmlStreamBuffer *buffer = g_new (HtmlStreamBuffer, 1);
	HtmlStream *stream = html_stream_new (html_stream_buffer_write, html_stream_buffer_close, buffer);

	buffer->buffer = NULL;
	buffer->user_data = user_data;
	buffer->close_func = close_func;

	return stream;
}


