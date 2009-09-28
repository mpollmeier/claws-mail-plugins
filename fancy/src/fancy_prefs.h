/* 
 * Claws Mail -- A GTK+ based, lightweight, and fast e-mail client
 * Copyright(C) 1999-2009 the Claws Mail Team
 * == Fancy Plugin ==
 * This file Copyright (C) 2009 Salvatore De Paolis
 * <iwkse@claws-mail.org>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write tothe Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef FANCY_PREFS_H
#define FANCY_PREFS_H

#include <glib.h>
#include <webkit/webkitwebsettings.h>
typedef struct _FancyPrefs FancyPrefs;

struct _FancyPrefs
{
    WebKitWebSettings *web_settings;
    
    gboolean auto_load_images;
    gboolean block_links;
    gboolean enable_scripts;
    gboolean enable_plugins;
    gboolean open_external;
};

extern FancyPrefs fancy_prefs;

void fancy_prefs_init(void);
void fancy_prefs_done(void);

#endif
