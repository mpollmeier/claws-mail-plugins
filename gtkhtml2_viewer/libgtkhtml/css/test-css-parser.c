/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
   Copyright (C) 2000 CodeFactory AB
   Copyright (C) 2000 Jonas Borgstr�m <jonas@codefactory.se>
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
#include <glib-object.h>
#include <string.h>
#include <libxml/parser.h>
#include "cssdebug.h"
#include "cssparser.h"
#include "util/htmlglobalatoms.h"
#include "html.css.h"

#define STYLESHEET html_css

gint
main (gint argc, gchar **argv)
{
	CssStylesheet *stylesheet;

	g_print ("length is: %d\n", strlen (html_css));
		
	html_atom_list = html_atom_list_new ();
	html_global_atoms_initialize (html_atom_list);

	stylesheet = css_parser_parse_stylesheet (STYLESHEET, strlen (STYLESHEET));

	css_debug_print_stylesheet (stylesheet);
}
