/*
 * Sylpheed -- a GTK+ based, lightweight, and fast e-mail client
 * Copyright (C) 1999-2004 Hiroyuki Yamamoto & the Sylpheed-Claws Team
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

#include <glib.h>

#include <gtk/gtk.h>

#include "intl.h"
#include "utils.h"
#include "folder.h"
#include "folderview.h"
#include "menu.h"
#include "account.h"
#include "alertpanel.h"
#include "inputdialog.h"
#include "mh.h"
#include "foldersel.h"

static void new_folder_cb(FolderView *folderview, guint action, GtkWidget *widget);
/*
static void delete_folder_cb(FolderView *folderview, guint action, GtkWidget *widget);
static void rename_folder_cb(FolderView *folderview, guint action, GtkWidget *widget);
static void move_folder_cb(FolderView *folderview, guint action, GtkWidget *widget);
*/
static void update_tree_cb(FolderView *folderview, guint action, GtkWidget *widget);
/*
static void remove_mailbox_cb(FolderView *folderview, guint action, GtkWidget *widget);
*/
static void add_mailbox(gpointer callback_data, guint callback_action, GtkWidget *widget);

static GtkItemFactoryEntry maildir_popup_entries[] =
{
	{N_("/Create _new folder..."),	 NULL, new_folder_cb,     0, NULL},
/*
	{N_("/_Rename folder..."),	 NULL, rename_folder_cb,  0, NULL},
	{N_("/M_ove folder..."), 	 NULL, move_folder_cb,    0, NULL},
	{N_("/_Delete folder"),		 NULL, delete_folder_cb,  0, NULL},
*/
	{N_("/---"),			 NULL, NULL,              0, "<Separator>"},
	{N_("/_Check for new messages"), NULL, update_tree_cb,    0, NULL},
	{N_("/R_ebuild folder tree"),	 NULL, update_tree_cb,    1, NULL},
	{N_("/---"),			 NULL, NULL, 		  0, "<Separator>"},
/*
	{N_("/Remove _mailbox"),	 NULL, remove_mailbox_cb, 0, NULL},
	{N_("/---"),			 NULL, NULL, 		  0, "<Separator>"},
*/
};

static void set_sensitivity(GtkItemFactory *factory, FolderItem *item);

static FolderViewPopup maildir_popup =
{
	"maildir",
	"<MaildirFolder>",
	NULL,
	set_sensitivity
};

static GtkItemFactoryEntry mainwindow_add_mailbox = {
	N_("/File/Add mailbox/Maildir++..."),
	NULL, 
	add_mailbox, 
	0, 
	NULL
};

void maildir_gtk_init(void)
{
	guint i, n_entries;
	GtkItemFactory *ifactory;
	MainWindow *mainwin = mainwindow_get_mainwindow();

	n_entries = sizeof(maildir_popup_entries) /
		sizeof(maildir_popup_entries[0]);
	for (i = 0; i < n_entries; i++)
		maildir_popup.entries = g_slist_append(maildir_popup.entries, &maildir_popup_entries[i]);

	folderview_register_popup(&maildir_popup);

	ifactory = gtk_item_factory_from_widget(mainwin->menubar);
	gtk_item_factory_create_item(ifactory, &mainwindow_add_mailbox, mainwin, 1);
}

void maildir_gtk_done(void)
{
	GtkItemFactory *ifactory;
	MainWindow *mainwin = mainwindow_get_mainwindow();
	GtkWidget *widget;
	
	folderview_unregister_popup(&maildir_popup);

	ifactory = gtk_item_factory_from_widget(mainwin->menubar);
	widget = gtk_item_factory_get_widget(ifactory, mainwindow_add_mailbox.path);
	gtk_widget_destroy(widget);
	gtk_item_factory_delete_item(ifactory, mainwindow_add_mailbox.path);
}

static void set_sensitivity(GtkItemFactory *factory, FolderItem *item)
{
#define SET_SENS(name, sens) \
	menu_set_sensitive(factory, name, sens)

	SET_SENS("/Create new folder...",   TRUE);
/*
	SET_SENS("/Rename folder...",       item->stype == F_NORMAL && folder_item_parent(item) != NULL);
	SET_SENS("/Move folder...", 	    item->stype == F_NORMAL && folder_item_parent(item) != NULL);
	SET_SENS("/Delete folder", 	    item->stype == F_NORMAL && folder_item_parent(item) != NULL);
*/
	SET_SENS("/Check for new messages", folder_item_parent(item) == NULL);
	SET_SENS("/Rebuild folder tree",    folder_item_parent(item) == NULL);
/*
	SET_SENS("/Remove mailbox",         folder_item_parent(item) == NULL);
*/

#undef SET_SENS
}

static void update_tree_cb(FolderView *folderview, guint action,
			   GtkWidget *widget)
{
	FolderItem *item;

	item = folderview_get_selected(folderview);
	g_return_if_fail(item != NULL);

	summary_show(folderview->summaryview, NULL);

	g_return_if_fail(item->folder != NULL);

	if (action == 0)
		folderview_check_new(item->folder);
	else
		folderview_rescan_tree(item->folder);
}

static void add_mailbox(gpointer callback_data, guint callback_action,
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


	if (folder->klass->create_tree(folder) < 0) {
		alertpanel_error(_("Creation of the mailbox failed.\n"
				   "Maybe some files already exist, or you don't have the permission to write there."));
		folder_destroy(folder);
		return;
	}

	folder_add(folder);
	folder_scan_tree(folder);

	folderview_set(mainwin->folderview);

	return;
}

static void new_folder_cb(FolderView *folderview, guint action,
		          GtkWidget *widget)
{
	GtkCTree *ctree = GTK_CTREE(folderview->ctree);
	FolderItem *item;
	FolderItem *new_item;
	gchar *new_folder;
	gchar *name;
	gchar *p;

	if (!folderview->selected) return;

	item = gtk_ctree_node_get_row_data(ctree, folderview->selected);
	g_return_if_fail(item != NULL);
	g_return_if_fail(item->folder != NULL);

	new_folder = input_dialog(_("New folder"),
				  _("Input the name of new folder:"),
				  _("NewFolder"));
	if (!new_folder) return;
	AUTORELEASE_STR(new_folder, {g_free(new_folder); return;});

	p = strchr(new_folder, G_DIR_SEPARATOR);
	if (p == NULL)
		p = strchr(new_folder, '.');
	if (p) {
		alertpanel_error(_("`%c' can't be included in folder name."),
				 p[0]);
		return;
	}

	name = trim_string(new_folder, 32);
	AUTORELEASE_STR(name, {g_free(name); return;});

	/* find whether the directory already exists */
	if (folder_find_child_item_by_name(item, new_folder)) {
		alertpanel_error(_("The folder `%s' already exists."), name);
		return;
	}

	new_item = folder_create_folder(item, new_folder);
	if (!new_item) {
		alertpanel_error(_("Can't create the folder `%s'."), name);
		return;
	}

	folder_write_list();
}
