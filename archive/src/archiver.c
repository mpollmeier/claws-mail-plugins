/* vim:et:ts=4:sw=4:et:sts=4:ai:set list listchars=tab\:»·,trail\:·: */

/*
 * Claws Mail -- a GTK+ based, lightweight, and fast e-mail client
 * Copyright (C) 1999-2008 Michael Rasmussen and the Claws Mail Team
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 * 
 */

#ifdef HAVE_CONFIG_H
#	include <config.h>
#endif

#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gi18n.h>

#include "common/claws.h"
#include "common/version.h"
#include "plugin.h"
#include "mimeview.h"
#include "utils.h"
#include "alertpanel.h"
#include "statusbar.h"
#include "archiver.h"
#include "archiver_prefs.h"

#define PLUGIN_NAME (_("Mail Archiver"))

static void create_archive_cb(
		gpointer callback_data, guint callback_action, GtkWidget *widget) {

	debug_print("Call-back function called\n");
	
	archiver_gtk_show();
}

static GtkItemFactoryEntry archiver_main_menu = {
	N_("/Tools/Create Archive..."),
	"<alt>A",
	create_archive_cb,
	0,
	NULL
};

static GtkItemFactoryEntry archiver_separator = {
	N_("/Tools/----"),
	NULL,
	NULL,
	0,
	"<Separator>"
};

gint plugin_init(gchar** error)
{
	GtkItemFactory *ifactory;
	MainWindow *mainwin = mainwindow_get_mainwindow();

	if (!check_plugin_version(MAKE_NUMERIC_VERSION(3,4,0,65),
				VERSION_NUMERIC, PLUGIN_NAME, error))
		return -1;

	ifactory = gtk_item_factory_from_widget(mainwin->menubar);
	gtk_item_factory_create_item(ifactory, &archiver_separator, mainwin, 1);
	gtk_item_factory_create_item(ifactory, &archiver_main_menu, mainwin, 1);

	archiver_prefs_init();

	debug_print("archive plugin loaded\n");

	return 0;
}

gboolean plugin_done(void)
{
	MainWindow *mainwin = mainwindow_get_mainwindow();
	GtkItemFactory *ifactory;
	GtkWidget *widget;

	if (mainwin == NULL)
		return FALSE;
	ifactory = gtk_item_factory_from_widget(mainwin->menubar);
	widget = gtk_item_factory_get_widget(ifactory, archiver_separator.path);
	gtk_widget_destroy(widget);
	gtk_item_factory_delete_item(ifactory, archiver_separator.path);
	widget = gtk_item_factory_get_widget(ifactory, archiver_main_menu.path);
	gtk_widget_destroy(widget);
	gtk_item_factory_delete_item(ifactory, archiver_main_menu.path);

	archiver_prefs_done();
	debug_print("archive plugin unloaded\n");

	return TRUE;
}

const gchar* plugin_licence(void) {
	return "GPL3+";
}

const gchar* plugin_version(void) {
	return PLUGINVERSION;
}

const gchar* plugin_type(void) {
	return "GTK2";
}

const gchar* plugin_name(void) {
	return PLUGIN_NAME;
}

const gchar* plugin_desc(void) {
	return _("This plugin adds archiving features to claws-mail.\n"
			 "\n"
			 "The basic functionality is that you choose a folder "
			 "in the folder view, decides the name of the archive, "
			 "and choose a local folder to store the archive.\n"
			 "\n"
			 "Supported folders are IMAP, POP, LOCAL and vCalendar.\n"
			 "\n"
			 "The archive can be stored as:\n"
			 "\t-\tTAR\n\t-\tPAX\n\t-\tSHAR\n\t-\tCPIO\n"
			 "\n"
			 "The archive can be compressed using:\n"
			 "\t-\tGZIP/ZIP\n\t-\tBZIP2\n\t-\tNONE\n"
			 "\n"
			 "An option for adding folders recursively is available\n"
			 "\n"
			 "The archive can be restored with any standard tool "
			 "supporting the chosen format and compression.\n"
			 "\n"
			 "To activate the archiving feature either go to "
			 "Tools->Create Archive or use Alt+A."
			);
}

struct PluginFeature* plugin_provides(void) {
	static struct PluginFeature features[] =
	{ {PLUGIN_UTILITY, N_("Archiver")},
	  {PLUGIN_NOTHING, NULL} };
	return features;
}
