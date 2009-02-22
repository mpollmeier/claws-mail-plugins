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
#include <stdio.h>
#include <glib.h>
#include "libgtkhtml/util/rfc1738.h"

gchar *
rfc1738_encode_string (const gchar *str)
{
        static gchar *safe = "$-._!*(),"; /* RFC 1738 */
        unsigned pos = 0;
        GString *encoded = g_string_new ("");
        gchar buffer[5], *ptr;
	guchar c;
	
        while ( pos < strlen(str) ) {

		c = (unsigned char) str[pos];
			
		if ( (( c >= 'A') && ( c <= 'Z')) ||
		     (( c >= 'a') && ( c <= 'z')) ||
		     (( c >= '0') && ( c <= '9')) ||
		     (strchr(safe, c)) )
			encoded = g_string_append_c (encoded, c);
		else if ( c == ' ' )
			encoded = g_string_append_c (encoded, '+');
		else if ( c == '\n' )
			encoded = g_string_append (encoded, "%0D%0A");
		else if ( c != '\r' ) {
			sprintf( buffer, "%%%02X", (int)c );
			encoded = g_string_append (encoded, buffer);
		}
		pos++;
	}
	
	ptr = encoded->str;

	g_string_free (encoded, FALSE);

        return ptr;
}

gchar *
rfc1738_make_full_url (const gchar *base, const gchar *rel)
{
	GString *full = g_string_new ("");
	gint pos;
	gchar *ptr;

	g_return_val_if_fail (base || rel, NULL);

	if (base == NULL && rel)
		return g_strdup (rel);

	if (rel == NULL && base)
		return g_strdup (base);

	/* Looks like rel is a full url, lets use it */
	if (rel && strchr (rel, ':'))
		return g_strdup (rel);

	pos = strlen (base) - 1;

	while (base[pos] && base[pos] != '/') 
		pos--;

	if (base[pos]) {

		g_string_append_len (full, base, pos + 1);
	}
	g_string_append (full, rel);

	ptr = full->str;

	g_string_free (full, FALSE);

	/* FIXME: shorten url's with "/../" /jb */

	return ptr;
}

#if 0
gint
main (gint argc, gchar **argv)
{
	g_print ("url = %s\n", rfc1738_make_full_url ("http://www.gnome.org/", "foo/bar.html"));
	g_print ("url = %s\n", rfc1738_make_full_url ("http://www.gnome.org/index.html", "foo/bar.html"));
	g_print ("url = %s\n", rfc1738_make_full_url ("http://www.gnome.org/gtkhtml2/", "foo/bar.html"));
	g_print ("url = %s\n", rfc1738_make_full_url ("http://www.gnome.org/gtkhtml2/index.html", "bar.html"));
}
#endif
