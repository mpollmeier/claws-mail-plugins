/*
 * Sylpheed -- a GTK+ based, lightweight, and fast e-mail client
 * Copyright (C) 1999-2004 Hiroyuki Yamamoto
 * This file (C) 2005 Andrej Kacian <andrej@kacian.sk>
 *
 * - callback handler functions for folderview rssyl context menu items
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

#include <gtk/gtk.h>

#include "gettext.h"

#include "folderview.h"
#include "alertpanel.h"
#include "gtk/inputdialog.h"
#include "prefs_common.h"
#include "inc.h"

#include "feed.h"
#include "feedprops.h"
#include "rssyl.h"
#include "rssyl_gtk.h"

void rssyl_new_feed_cb(FolderView *folderview, guint action,
		GtkWidget *widget)
{
	GtkCTree *ctree = GTK_CTREE(folderview->ctree);
	FolderItem *item;
	gchar *new_feed;

	debug_print("RSSyl: new_feed_cb\n");

	g_return_if_fail(folderview->selected != NULL);

	item = gtk_ctree_node_get_row_data(ctree, folderview->selected);
	g_return_if_fail(item != NULL);
	g_return_if_fail(item->folder != NULL);

	new_feed = input_dialog(_("Subscribe feed"),
					 _("Input the URL of the news feed you wish to subscribe:"),
					 "");
	g_return_if_fail(new_feed != NULL);

	rssyl_subscribe_new_feed(item, new_feed, TRUE);

	g_free(new_feed);
}

void rssyl_remove_rss_cb(FolderView *folderview, guint action,
		GtkWidget *widget)
{
	FolderItem *item;
	gchar *name, *message;
	AlertValue aval;

	debug_print("RSSyl: remove_rss_cb\n");

	item = folderview_get_selected_item(folderview);
	g_return_if_fail(item != NULL);
	g_return_if_fail(item->folder != NULL);
	g_return_if_fail( !folder_item_parent(item) );

	name = trim_string(item->folder->name, 32);
	message = g_strdup_printf(_("Really remove the folder tree '%s' ?\n"), name);
	aval = alertpanel_full(_("Remove folder tree"), message,
			 GTK_STOCK_CANCEL, _("_Remove"), NULL, FALSE, NULL,
			  ALERT_WARNING, G_ALERTDEFAULT);
	g_free(message);
	g_free(name);

	if (aval != G_ALERTALTERNATE)
		return;

	folderview_unselect(folderview);
	summary_clear_all(folderview->summaryview);

	folder_destroy(item->folder);
}

void rssyl_remove_feed_cb(FolderView *folderview, guint action,
		GtkWidget *widget)
{
	GtkCTree *ctree = GTK_CTREE(folderview->ctree);
	FolderItem *item;
	gint response;
	GtkWidget *dialog, *rmcache_widget = NULL;
	gboolean rmcache = FALSE;

	debug_print("RSSyl: remove_feed_cb\n");

	item = folderview_get_selected_item(folderview);
	g_return_if_fail(item != NULL);
	g_return_if_fail(item->path != NULL);
	g_return_if_fail(item->folder != NULL);

	dialog = rssyl_feed_removal_dialog(item->name, &rmcache_widget);

	g_return_if_fail(dialog != NULL);

	gtk_widget_show_all(dialog);

	response = gtk_dialog_run(GTK_DIALOG(dialog));

	if( response != GTK_RESPONSE_YES ) {
		debug_print("'No' clicked, returning\n");
		gtk_widget_destroy(dialog);
		return;
	}

	g_return_if_fail(rmcache_widget != NULL);

	rmcache = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(rmcache_widget) );

	gtk_widget_destroy(dialog);

	if( folderview->opened == folderview->selected ||
			gtk_ctree_is_ancestor(ctree,
					folderview->selected,
					folderview->opened) ) {
		summary_clear_all(folderview->summaryview);
		folderview->opened = NULL;
	}

	rssyl_remove_feed_props((RSSylFolderItem *)item);

	if( rmcache == TRUE )
		rssyl_remove_feed_cache(item);

	if( item->folder->klass->remove_folder(item->folder, item) < 0 ) {
		alertpanel_error(_("Can't remove feed '%s'."), item->name);
		if( folderview->opened == folderview->selected )
			summary_show(folderview->summaryview,
					folderview->summaryview->folder_item);
		return;
	}

	folder_write_list();
}

void rssyl_refresh_cb(FolderView *folderview, guint action,
		GtkWidget *widget)
{
	FolderItem *item;
	RSSylFolderItem *ritem;

	item = folderview_get_selected_item(folderview);
	g_return_if_fail(item != NULL);
	g_return_if_fail(item->folder != NULL);

	ritem = (RSSylFolderItem *)item;

	if (prefs_common.work_offline && 
	   !inc_offline_should_override(
	   	_("Sylpheed-Claws needs network access in order "
		  "to update the feed."))) {
			return;
	}

	rssyl_update_feed(ritem);
}

static void rssyl_refresh_all_func(FolderItem *item, gpointer data)
{
	/* Only try to refresh our feed folders */
	if( !IS_RSSYL_FOLDER_ITEM(item) )
		return;

	/* Don't try to refresh the root folder */
	if( folder_item_parent(item) == NULL )
		return;

	rssyl_update_feed((RSSylFolderItem *)item);
}

void rssyl_refresh_all_cb(FolderView *folderview, guint action,
		GtkWidget *widget)
{
	if (prefs_common.work_offline && 
	    !inc_offline_should_override(
		_("Sylpheed-Claws needs network access in order "
		  "to update the feeds."))) {
			return;
	}

	folder_func_to_all_folders((FolderItemFunc)rssyl_refresh_all_func, NULL);
}

void rssyl_prop_cb(FolderView *folderview, guint action, GtkWidget *widget)
{
	FolderItem *item;
	RSSylFolderItem *ritem;

	item = folderview_get_selected_item(folderview);
	g_return_if_fail(item != NULL);
	g_return_if_fail(item->folder != NULL);

	debug_print("RSSyl: rssyl_prop_cb() for '%s'\n", item->name);

	ritem = (RSSylFolderItem *)item;
	rssyl_get_feed_props(ritem);
	rssyl_gtk_prop(ritem);
}

void rssyl_rename_cb(FolderView *folderview, guint action,
			     GtkWidget *widget)
{
	FolderItem *item;
	gchar *new_folder;
	gchar *name;
	gchar *message;

	item = folderview_get_selected_item(folderview);
	g_return_if_fail(item != NULL);
	g_return_if_fail(item->path != NULL);
	g_return_if_fail(item->folder != NULL);

	name = trim_string(item->name, 32);
	message = g_strdup_printf(_("Input new name for '%s':"), name);
	new_folder = input_dialog(_("Rename folder"), message, name);
	g_free(message);
	g_free(name);
	if (!new_folder) return;
	AUTORELEASE_STR(new_folder, {g_free(new_folder); return;});

	if (strchr(new_folder, G_DIR_SEPARATOR) != NULL) {
		alertpanel_error(_("'%c' can't be included in folder name."),
				 G_DIR_SEPARATOR);
		return;
	}

	if (folder_find_child_item_by_name(folder_item_parent(item), new_folder)) {
		name = trim_string(new_folder, 32);
		alertpanel_error(_("The folder '%s' already exists."), name);
		g_free(name);
		return;
	}

	if (folder_item_rename(item, new_folder) < 0) {
		alertpanel_error(_("The folder could not be renamed.\n"
				   "The new folder name is not allowed."));
		return;
	}

	folder_item_prefs_save_config(item);
	folder_write_list();
}
