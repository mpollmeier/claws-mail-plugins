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

#include <glib.h>
#include "htmlglobalatoms.h"
#include "htmlatomlist.h"

HtmlAtomList *html_atom_list = NULL;

HtmlAtomList *
html_atom_list_new (void)
{
	HtmlAtomList *al;

	al = g_new (HtmlAtomList, 1);
	al->len = 0;
	al->table = g_hash_table_new (g_str_hash, g_str_equal);
	al->data = NULL;
	
	return al;
}

gchar *
html_atom_list_get_string (HtmlAtomList *al, gint atom)
{
	gchar *str = NULL;
	
	if (atom >= 0 && atom <= al->len)
		str = al->data [atom];
	return str;
}

HtmlAtom
html_atom_list_get_atom_length (HtmlAtomList *al, const gchar *str, gint len)
{
	HtmlAtom atom;
	gchar *s = g_strndup (str, len);

	atom= html_atom_list_get_atom (al, s);

	g_free (s);

	return atom;
}

HtmlAtom
html_atom_list_get_atom (HtmlAtomList *al, const gchar *str)
{
	HtmlAtom atom;
	gchar *ptr;
	gboolean found;
	gpointer old_atom;

	ptr = g_ascii_strdown (str, strlen (str));

	found = g_hash_table_lookup_extended (al->table, ptr, NULL, &old_atom);
	
	if (!found) {
		if (al->len % 512 == 0)
			al->data = g_renew (gchar *, al->data, al->len + 512);

		al->data[al->len] = g_strdup (ptr);
		atom = al->len;
		g_hash_table_insert (al->table, al->data[al->len], GUINT_TO_POINTER (atom));
		al->len++;
	}
	else {
		atom = GPOINTER_TO_UINT (old_atom);
	}

	g_free (ptr);
	
	return atom;
}

void
html_atom_list_initialize (void)
{
	if (html_atom_list == NULL)
		html_atom_list = html_atom_list_new ();
	
	html_global_atoms_initialize (html_atom_list);
}




