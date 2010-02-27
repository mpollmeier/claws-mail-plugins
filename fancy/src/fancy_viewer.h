/*
 * Claws Mail -- a GTK+ based, lightweight, and fast e-mail client
 * == Fancy Plugin ==
 * Copyright (C) 1999-2006 Hiroyuki Yamamoto and the Claws Mail Team
 * This file Copyright (C) 2009 Salvatore De Paolis <iwkse@claws-mail.org>
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

#ifndef FANCY_VIEWER_H
#define FANCY_VIEWER_H

#include <unistd.h>
#include <stdio.h>

#include <glib.h>
#include "gettext.h"
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <webkit/webkitwebview.h>
#include <webkit/webkitversion.h>
#include <webkit/webkitwebframe.h>
#include <webkit/webkitnetworkrequest.h>
#include <prefs_common.h>
#include "common/claws.h"
#include "common/version.h"
#include "common/utils.h"
#include "mimeview.h"
#include "gdk/gdkkeysyms.h"
#include "main.h"
#include "plugin.h"
#include "menu.h"
#include "defs.h"
#include "utils.h"

#ifdef USE_PTHREAD
#include <pthread.h>
#endif

#ifdef HAVE_LIBCURL
#include <curl/curl.h>
#include <curl/curlver.h>
#endif

#ifdef HAVE_LIBSOUP_GNOME
#include <libsoup/soup-gnome.h>
#endif

typedef struct _FancyViewer FancyViewer;
struct _FancyViewer
{
    MimeViewer        mimeviewer;
    GtkWidget         *scrollwin;
    WebKitWebView     *view;
    GtkWidget         *vbox;  
    GtkWidget         *l_link;
    /* Zoom Widgets  */
    GtkWidget         *zoom_100;
    GtkWidget         *ev_zoom_100;
    GtkWidget         *zoom_in;
    GtkWidget         *ev_zoom_in;
    GtkWidget         *zoom_out;
    GtkWidget         *ev_zoom_out;
    GtkWidget         *stop_loading;
    GtkWidget         *ev_stop_loading;
    /* Preferences Widgets  */
    GtkWidget         *fancy_prefs;
    GtkWidget         *ev_fancy_prefs;
    GtkWidget         *fancy_prefs_menu;
    /* Menu Items */
    GtkWidget         *auto_load_images;
    GtkWidget         *enable_scripts;
    GtkWidget         *enable_plugins;
    GtkWidget         *block_extern_content;
    GtkWidget         *open_external;

    GtkWidget         *progress;
    WebKitWebSettings *settings;    
    gboolean          load_page;
    gboolean          printing;
    gboolean          override_prefs_images;
    gboolean          override_prefs_block_extern_content;
    gboolean          override_prefs_scripts;
    gboolean          override_prefs_plugins;
    gboolean          override_prefs_external;

    gboolean          cache_prefs_auto_load_images;
    gboolean          cache_prefs_enable_scripts;
    gboolean          cache_prefs_enable_plugins;

    gchar             *cur_link;
    gchar             *filename;
    MimeInfo          *to_load;
    gulong            doc_handle;
    gint              tag;
    gint              loading;
    gint              stop_previous;
};
#define OPEN_INTERNAL FALSE
#define OPEN_EXTERNAL TRUE
#define CTRL_KEY 4
#define GOOGLE_SEARCH "http://www.google.com/search?hl=en&q="

#endif

