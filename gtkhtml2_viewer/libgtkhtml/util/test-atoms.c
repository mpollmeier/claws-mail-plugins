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

#include "htmlglobalatoms.h"

gint
main (gint argc, gchar **argv)
{
	HtmlAtomList *al = html_atom_list_new ();
	
	html_global_atoms_initialize (al);

	g_print ("Display is: %d %d\n", HTML_ATOM_DISPLAY, html_atom_list_get_atom (al, "display"));

	g_print ("Embed is: %d %d\n", HTML_ATOM_EMBED, html_atom_list_get_atom (al, "embed"));
	
	g_print ("Foo is: %d\n", html_atom_list_get_atom (al, "foo"));
	g_print ("Bar is: %d\n", html_atom_list_get_atom (al, "bar"));
	g_print ("Foo is: %d\n", html_atom_list_get_atom (al, "foo"));
	return 0;
}
