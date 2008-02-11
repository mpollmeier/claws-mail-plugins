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

#include <stdio.h>
#include <glib.h>
#include "gtkhtml.h"
#include "htmlparser.h"
#include "htmldocument.h"

gint
main (gint argc, gchar **argv)
{
	HtmlDocument *doc;
	GtkHtml *html;
	HtmlParser *parser;
	FILE *file;
	size_t bytes_read;
	gchar chars[10];
	
	if (argc != 2) {
		g_print ("Usage: %s file.html\n\n", argv[0]);
		return 0;
	}

	gtk_type_init ();

	html = gtk_html_new ();
	doc = gtk_html_create_document (html);

	if (strlen (argv[1]) > 5 &&
	    strcmp (argv[1] + strlen (argv[1]) - 5, ".html") == 0) {
		g_print ("Html!\n");
		parser = html_parser_new (doc, HTML_PARSER_TYPE_HTML);
	}
	else {
		parser = html_parser_new (doc, HTML_PARSER_TYPE_XML);
	}
	
	file = fopen (argv[1], "r");

	
	if (!file) {
		g_print ("Could not open file %s\n", argv[1]);
		return 0;
	}

	while ((bytes_read = fread (chars, 1, 3, file)) > 0) {
		html_stream_write (parser->stream, chars, bytes_read);
	}

	html_stream_close (parser->stream);

	return 0;
}
