/*
 * Sylpheed -- a GTK+ based, lightweight, and fast e-mail client
 * Copyright (C) 1999-2006 Hiroyuki Yamamoto and the Claws Mail Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

/*
 * The structure of this file has been borrowed from the structure of
 * the image_viewer plugin file. I also used it as an example of how to
 * build the preferences for the gtkhtml2 plugin.
 */

#ifndef GTKHTML2PREFS_H
#define GTKHTML2PREFS_H

#include <glib.h>

typedef struct _GtkHtmlBrowserPrefs	GtkHtmlBrowserPrefs;

struct _GtkHtmlBrowserPrefs
{
	gboolean local;   /**< local browsing */
	gboolean whitelist_ab;
	gchar *whitelist_ab_folder;
	gboolean cache_images;
	gboolean clear_cache;
};

extern GtkHtmlBrowserPrefs gtkhtml_prefs;

void gtkhtml_prefs_init(void);
void gtkhtml_prefs_done(void);

#endif
