/*
 * Sylpheed -- a GTK+ based, lightweight, and fast e-mail client
 * Copyright (C) 1999-2003 Hiroyuki Yamamoto and the Sylpheed-Claws Team
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

#include "defs.h"
#include "version.h"
#include "sylpheed.h"

#include <glib.h>
#include <gtk/gtk.h>

#include "intl.h"
#include "plugin.h"
#include "utils.h"
#include "prefs.h"
#include "prefs_gtk.h"
#include "fetchinfo_plugin.h"

struct FetchinfoPage
{
	PrefsPage page;
	
	GtkWidget *fetchinfo_enable;
	GtkWidget *fetchinfo_uidl;
	GtkWidget *fetchinfo_account;
	GtkWidget *fetchinfo_server;
	GtkWidget *fetchinfo_userid;
	GtkWidget *fetchinfo_time;
};

static void fetchinfo_set_sensitive(struct FetchinfoPage *page, gboolean enable)
{
	gtk_widget_set_sensitive(GTK_WIDGET(page->fetchinfo_uidl), enable);
	gtk_widget_set_sensitive(GTK_WIDGET(page->fetchinfo_account), enable);
	gtk_widget_set_sensitive(GTK_WIDGET(page->fetchinfo_server), enable);
	gtk_widget_set_sensitive(GTK_WIDGET(page->fetchinfo_userid), enable);
	gtk_widget_set_sensitive(GTK_WIDGET(page->fetchinfo_time), enable);
}

static void fetchinfo_enable_cb(GtkWidget *widget, gpointer data)
{
	struct FetchinfoPage *page = (struct FetchinfoPage *) data;

	fetchinfo_set_sensitive(page, gtk_toggle_button_get_active(
		GTK_TOGGLE_BUTTON(page->fetchinfo_enable)));
}

#define ADD_NEW_CHECKBOX(line, button, text) \
	button = gtk_check_button_new_with_label (_(text)); \
	gtk_widget_show (button); \
	gtk_table_attach (GTK_TABLE (table), button, 1, 2, line, line+1, \
		       	  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), \
               		  (GtkAttachOptions) (0), 0, 0);

static void fetchinfo_create_widget_func(PrefsPage * _page, GtkWindow *window, gpointer data)
{
	struct FetchinfoPage *page = (struct FetchinfoPage *) _page;
	FetchinfoConfig *config;
	GtkWidget *table;
	GtkWidget *fetchinfo_enable;
  	GtkWidget *fetchinfo_uidl;
  	GtkWidget *fetchinfo_account;
  	GtkWidget *fetchinfo_server;
  	GtkWidget *fetchinfo_userid;
  	GtkWidget *fetchinfo_time;

  	table = gtk_table_new (6, 3, FALSE);
	gtk_widget_show(table);
	gtk_table_set_row_spacings(GTK_TABLE(table), 4);
	gtk_table_set_col_spacings(GTK_TABLE(table), 8);

	fetchinfo_enable = gtk_check_button_new_with_label (_("Add fetchinfo headers"));
	gtk_widget_show (fetchinfo_enable);
	gtk_table_attach (GTK_TABLE (table), fetchinfo_enable, 0, 2, 0, 1,
		       	  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
               		  (GtkAttachOptions) (0), 0, 0);

	ADD_NEW_CHECKBOX(1, fetchinfo_uidl,	"UIDL");
	ADD_NEW_CHECKBOX(2, fetchinfo_account,	"Account name");
	ADD_NEW_CHECKBOX(3, fetchinfo_server,	"Receive server");
	ADD_NEW_CHECKBOX(4, fetchinfo_userid,	"UserID");
	ADD_NEW_CHECKBOX(5, fetchinfo_time,	"Fetch time");

	config = fetchinfo_get_config();

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(fetchinfo_enable),
				     config->fetchinfo_enable);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(fetchinfo_uidl),
				     config->fetchinfo_uidl);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(fetchinfo_account),
				     config->fetchinfo_account);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(fetchinfo_server),
				     config->fetchinfo_server);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(fetchinfo_userid),
				     config->fetchinfo_userid);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(fetchinfo_time),
				     config->fetchinfo_time);

	gtk_signal_connect(GTK_OBJECT(fetchinfo_enable), "released", 
			   GTK_SIGNAL_FUNC(fetchinfo_enable_cb), page);

	page->fetchinfo_enable	= fetchinfo_enable;
	page->fetchinfo_uidl	= fetchinfo_uidl;
	page->fetchinfo_account	= fetchinfo_account;
	page->fetchinfo_server	= fetchinfo_server;
	page->fetchinfo_userid	= fetchinfo_userid;
	page->fetchinfo_time	= fetchinfo_time;

	page->page.widget = table;

	fetchinfo_set_sensitive(page, config->fetchinfo_enable);
}
#undef ADD_NEW_CHECKBOX

static void fetchinfo_destroy_widget_func(PrefsPage *_page)
{
	debug_print("Destroying Fetchinfo widget\n");
}

static void fetchinfo_save_func(PrefsPage *_page)
{
	struct FetchinfoPage *page = (struct FetchinfoPage *) _page;
	FetchinfoConfig *config;

	debug_print("Saving Fetchinfo Page\n");

	config = fetchinfo_get_config();

	config->fetchinfo_enable  = gtk_toggle_button_get_active(
		GTK_TOGGLE_BUTTON(page->fetchinfo_enable) );
	config->fetchinfo_uidl	  = gtk_toggle_button_get_active(
		GTK_TOGGLE_BUTTON(page->fetchinfo_uidl)   );
	config->fetchinfo_account = gtk_toggle_button_get_active(
		GTK_TOGGLE_BUTTON(page->fetchinfo_account));
	config->fetchinfo_server  = gtk_toggle_button_get_active(
		GTK_TOGGLE_BUTTON(page->fetchinfo_server) );
	config->fetchinfo_userid  = gtk_toggle_button_get_active(
		GTK_TOGGLE_BUTTON(page->fetchinfo_userid) );
	config->fetchinfo_time	  = gtk_toggle_button_get_active(
		GTK_TOGGLE_BUTTON(page->fetchinfo_time)   );

	fetchinfo_save_config();
}

static struct FetchinfoPage fetchinfo_page;

gint plugin_init(gchar **error)
{
	static gchar *path[3];

	if ((sylpheed_get_version() > VERSION_NUMERIC)) {
		*error = g_strdup("Your sylpheed version is newer than the version the plugin was built with");
		return -1;
	}

	if ((sylpheed_get_version() < MAKE_NUMERIC_VERSION(0, 9, 12, 0))) {
		*error = g_strdup("Your sylpheed version is too old");
		return -1;
	}

	path[0] = _("Plugins");
	path[1] = _("Fetchinfo");
	path[2] = NULL;

	fetchinfo_page.page.path = path;
	fetchinfo_page.page.create_widget = fetchinfo_create_widget_func;
	fetchinfo_page.page.destroy_widget = fetchinfo_destroy_widget_func;
	fetchinfo_page.page.save_page = fetchinfo_save_func;
	
	prefs_gtk_register_page((PrefsPage *) &fetchinfo_page);

	debug_print("Fetchinfo GTK plugin loaded\n");
	return 0;	
}

void plugin_done(void)
{
	prefs_gtk_unregister_page((PrefsPage *) &fetchinfo_page);

	debug_print("Fetchinfo GTK plugin unloaded\n");
}

const gchar *plugin_name(void)
{
	return _("Fetchinfo GTK");
}

const gchar *plugin_desc(void)
{
	return _("This plugin adds a configuration page for"
	         "the Fetchinfo plugin.\n");
}

const gchar *plugin_type(void)
{
	return "GTK";
}
