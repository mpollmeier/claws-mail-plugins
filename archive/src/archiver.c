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

#include "gettext.h"
#include <gtk/gtk.h>

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
	NULL,
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

	bindtextdomain(TEXTDOMAIN, LOCALEDIR);
	bind_textdomain_codeset(TEXTDOMAIN, "UTF-8");

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
	return _("This plugin adds archiving features to Claws Mail.\n"
			"\n"
			"It enables you to select a mail folder that you want "
			"to be archived, and then choose a name, format and "
			"location for the archive. Subfolders can be included "
			"and MD5 checksums can be added for each file in the "
			"archive. Several archiving options are also available.\n"
			"\n"
			"The archive can be stored as:\n"
			"\tTAR\n\tPAX\n\tSHAR\n\tCPIO\n"
			"\n"
			"The archive can be compressed using:\n"
			"\tGZIP/ZIP\n\tBZIP2\n"
			"\n"
			"The archives can be restored with any standard tool "
			"that supports the chosen format and compression.\n"
			"\n"
			"The supported folder types are MH, IMAP, RSSyl and "
			"vCalendar.\n"
			"\n"
			"To activate the archiving feature go to /Tools/Create Archive\n"
			"\n"
			"Default options can be set in /Configuration/Preferences/Plugins"
			"/Mail Archiver"
			);
}

struct PluginFeature* plugin_provides(void) {
	static struct PluginFeature features[] =
	{ {PLUGIN_UTILITY, N_("Archiver")},
	  {PLUGIN_NOTHING, NULL} };
	return features;
}
