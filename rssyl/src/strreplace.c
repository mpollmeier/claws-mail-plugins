/*
 * Sylpheed -- a GTK+ based, lightweight, and fast e-mail client
 * Copyright (C) 2005 Andrej Kacian <andrej@kacian.sk>
 *
 * - a strreplace function (something like sed's s/foo/bar/g)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glib.h>
#include <stdlib.h>

#include "common/utils.h"

gchar *rssyl_strreplace(gchar *source, gchar *pattern,
		gchar *replacement)
{
	gchar *new, *c;
	guint count = 0, final_length;

/*
	debug_print("RSSyl: ======= strreplace: '%s': '%s'->'%s'\n", source, pattern,
			replacement);
*/

	if( source == NULL || pattern == NULL ) {
		debug_print("RSSyl: source or pattern is NULL!!!\n");
		return NULL;
	}

	if( !g_utf8_validate(source, -1, NULL) ) {
		debug_print("RSSyl: source is not an UTF-8 encoded text\n");
		return NULL;
	}

	if( !g_utf8_validate(pattern, -1, NULL) ) {
		debug_print("RSSyl: pattern is not an UTF-8 encoded text\n");
		return NULL;
	}

	c = source;
	while( ( c = g_strstr_len(c, strlen(c), pattern) ) ) {
		count++;
		c++;
	}

/*
	debug_print("RSSyl: ==== count = %d\n", count);
*/

	final_length = strlen(source)
		- ( count * strlen(pattern) )
		+ ( count * strlen(replacement) );

	new = malloc(final_length + 1);
	memset(new, '\0', final_length + 1);

	c = source;

	while( *c != '\0' ) {
		if( !memcmp(c, pattern, strlen(pattern)) ) {
			strncat(new, replacement, strlen(replacement));
			c = c + strlen(pattern);
		} else {
			strncat(new, c, 1);
			c++;
		}
	}

	return new;
}

