/*
 * Maildir Plugin -- Maildir++ support for Sylpheed
 * Copyright (C) 2003 Christoph Hohmann
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

#include <gtk/gtk.h>

#include "intl.h"
#include "plugin.h"
#include "folder.h"
#include "maildir.h"
#include "mainwindow.h"
#include "folderview.h"
#include "inputdialog.h"

static void maildir_add_mailbox(gpointer callback_data, guint callback_action,
                                GtkWidget *widget)
{
	MainWindow *mainwin = (MainWindow *) callback_data;
	gchar *path;
	Folder *folder;

	path = input_dialog(_("Add mailbox"),
			    _("Input the location of mailbox.\n"
			      "If the existing mailbox is specified, it will be\n"
			      "scanned automatically."),
			    "Mail");
	if (!path) return;
	if (folder_find_from_path(path)) {
		alertpanel_error(_("The mailbox `%s' already exists."), path);
		g_free(path);
		return;
	}
	folder = folder_new(folder_get_class_from_string("maildir"), 
			    !strcmp(path, "Mail") ? _("Mailbox") : g_basename(path),
			    path);
	g_free(path);

/*
	if (folder->klass->create_tree(folder) < 0) {
		alertpanel_error(_("Creation of the mailbox failed.\n"
				   "Maybe some files already exist, or you don't have the permission to write there."));
		folder_destroy(folder);
		return;
	}
*/
	folder_add(folder);
	folder_scan_tree(folder);

	folderview_set(mainwin->folderview);

	return;
}

static gchar *item_path = N_("/File/Add mailbox/Maildir++...");

gint plugin_init(gchar **error)
{
	GtkItemFactoryEntry ifentry = {
		item_path,
		NULL, 
		maildir_add_mailbox, 
		0, 
		NULL
	};
	GtkItemFactory *ifactory;
	MainWindow *mainwin = mainwindow_get_mainwindow();
	
	ifactory = gtk_item_factory_from_widget(mainwin->menubar);
	gtk_item_factory_create_item(ifactory, &ifentry, mainwin, 1);

	return 0;
}

void plugin_done()
{
	GtkItemFactory *ifactory;
	MainWindow *mainwin = mainwindow_get_mainwindow();
	GtkWidget *widget;
	
	ifactory = gtk_item_factory_from_widget(mainwin->menubar);

	widget = gtk_item_factory_get_widget(ifactory, item_path);
	gtk_widget_destroy(widget);

	gtk_item_factory_delete_item(ifactory, item_path);
}

const gchar *plugin_name()
{
	return "Maildir++ GTK";
}

const gchar *plugin_desc()
{
	return "";
}

const gchar *plugin_type()
{
	return "GTK";
}
