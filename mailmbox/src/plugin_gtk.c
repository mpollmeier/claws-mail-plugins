/*
 * mailmbox Plugin -- mbox support for Sylpheed
 * Copyright (C) 2003-2005 Christoph Hohmann, 
 *			   Hoa v. Dinh, 
 *			   Alfons Hoogervorst
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <gtk/gtk.h>

#include <glib.h>
#include <glib/gi18n.h>
#include "plugin.h"
#include "folder.h"
#include "mailmbox_folder.h"
#include "mainwindow.h"
#include "folderview.h"
#include "inputdialog.h"
#include "foldersel.h"
#include "alertpanel.h"
#include "main.h"

#include "pluginconfig.h"

static void new_folder_cb(FolderView *folderview, guint action, GtkWidget *widget);
static void delete_folder_cb(FolderView *folderview, guint action, GtkWidget *widget);
static void rename_folder_cb(FolderView *folderview, guint action, GtkWidget *widget);
static void move_folder_cb(FolderView *folderview, guint action, GtkWidget *widget);
static void update_tree_cb(FolderView *folderview, guint action, GtkWidget *widget);
static void remove_mailbox_cb(FolderView *folderview, guint action, GtkWidget *widget);
static void add_mailbox(gpointer callback_data, guint callback_action, GtkWidget *widget);

static GtkItemFactoryEntry claws_mailmbox_popup_entries[] =
{
	{N_("/Create _new folder..."),	 NULL, new_folder_cb,     0, NULL},
	{N_("/---"),			 NULL, NULL,              0, "<Separator>"},
	{N_("/_Rename folder..."),	 NULL, rename_folder_cb,  0, NULL},
	{N_("/M_ove folder..."), 	 NULL, move_folder_cb,    0, NULL},
	{N_("/Cop_y folder..."),	 NULL, move_folder_cb,    1, NULL},
	{N_("/---"),			 NULL, NULL,              0, "<Separator>"},
	{N_("/_Delete folder"),		 NULL, delete_folder_cb,  0, NULL},
	{N_("/---"),			 NULL, NULL,              0, "<Separator>"},
	{N_("/_Check for new messages"), NULL, update_tree_cb,    0, NULL},
	{N_("/C_heck for new folders"),  NULL, update_tree_cb,    1, NULL},
	{N_("/R_ebuild folder tree"),	 NULL, update_tree_cb,    2, NULL},
	{N_("/---"),			 NULL, NULL, 		  0, "<Separator>"},
	{N_("/Remove _mailbox"),	 NULL, remove_mailbox_cb, 0, NULL},
	{N_("/---"),			 NULL, NULL, 		  0, "<Separator>"},
};

static void set_sensitivity(GtkItemFactory *factory, FolderItem *item);

static FolderViewPopup claws_mailmbox_popup =
{
	"mailmbox",
	"<MailmboxFolder>",
	NULL,
	set_sensitivity
};

static GtkItemFactoryEntry mainwindow_add_mailbox = {
	N_("/File/Add mailbox/mbox (etPan!)..."),
	NULL, 
	add_mailbox, 
	0, 
	NULL
};

gint plugin_gtk_init(gchar **error)
{
	guint i, n_entries;
	GtkItemFactory *ifactory;
	MainWindow *mainwin = mainwindow_get_mainwindow();

	n_entries = sizeof(claws_mailmbox_popup_entries) /
		sizeof(claws_mailmbox_popup_entries[0]);
	for (i = 0; i < n_entries; i++)
		claws_mailmbox_popup.entries = g_slist_append(claws_mailmbox_popup.entries, &claws_mailmbox_popup_entries[i]);

	folderview_register_popup(&claws_mailmbox_popup);

	ifactory = gtk_item_factory_from_widget(mainwin->menubar);
	gtk_item_factory_create_item(ifactory, &mainwindow_add_mailbox, mainwin, 1);

	return 0;
}

void plugin_gtk_done(void)
{
	GtkItemFactory *ifactory;
	MainWindow *mainwin = mainwindow_get_mainwindow();
	GtkWidget *widget;
	
	if (mainwin == NULL || claws_is_exiting())
		return;

	folderview_unregister_popup(&claws_mailmbox_popup);

	ifactory = gtk_item_factory_from_widget(mainwin->menubar);
	widget = gtk_item_factory_get_widget(ifactory, mainwindow_add_mailbox.path);
	gtk_widget_destroy(widget);
	gtk_item_factory_delete_item(ifactory, mainwindow_add_mailbox.path);
}

static void set_sensitivity(GtkItemFactory *factory, FolderItem *item)
{
#define SET_SENS(name, sens) \
	menu_set_sensitive(factory, name, sens)

	SET_SENS("/Create new folder...",   item->stype != F_INBOX);
	SET_SENS("/Rename folder...",       item->stype == F_NORMAL && folder_item_parent(item) != NULL);
	SET_SENS("/Move folder...", 	    item->stype == F_NORMAL && folder_item_parent(item) != NULL);
	SET_SENS("/Delete folder", 	    item->stype == F_NORMAL && folder_item_parent(item) != NULL);
	SET_SENS("/Check for new messages", folder_item_parent(item) == NULL);
	SET_SENS("/Check for new folders",  folder_item_parent(item) == NULL);
	SET_SENS("/Rebuild folder tree",    folder_item_parent(item) == NULL);
	SET_SENS("/Remove mailbox",         folder_item_parent(item) == NULL);

#undef SET_SENS
}

static void update_tree_cb(FolderView *folderview, guint action,
			   GtkWidget *widget)
{
	FolderItem *item;

	item = folderview_get_selected_item(folderview);
	g_return_if_fail(item != NULL);

	summary_show(folderview->summaryview, NULL);

	g_return_if_fail(item->folder != NULL);

	if (action == 0)
		folderview_check_new(item->folder);
	else if (action == 1)
		folderview_rescan_tree(item->folder, FALSE);
	else if (action == 2)
		folderview_rescan_tree(item->folder, TRUE);
}

static void add_mailbox(gpointer callback_data, guint callback_action,
                        GtkWidget *widget)
{
	MainWindow *mainwin = (MainWindow *) callback_data;
	gchar *path, *basename;
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
	basename = g_path_get_basename(path);
	folder = folder_new(folder_get_class_from_string("mailmbox"), 
			    !strcmp(path, "Mail") ? _("Mailbox") : basename,
			    path);
	g_free(basename);			    
	g_free(path);


	if (folder->klass->create_tree(folder) < 0) {
		alertpanel_error(_("Creation of the mailbox failed.\n"
				   "Maybe some files already exist, or you don't have the permission to write there."));
		folder_destroy(folder);
		return;
	}

	folder_add(folder);
	folder_scan_tree(folder, TRUE);

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
	p = g_strconcat(item->path ? item->path : "", ".", new_folder, NULL);
	if (folder_find_child_item_by_name(item, p)) {
		g_free(p);
		alertpanel_error(_("The folder `%s' already exists."), name);
		return;
	}
	g_free(p);

	new_item = folder_create_folder(item, new_folder);
	if (!new_item) {
		alertpanel_error(_("Can't create the folder `%s'."), name);
		return;
	}

	folder_write_list();
}

static void remove_mailbox_cb(FolderView *folderview, guint action, GtkWidget *widget)
{
	GtkCTree *ctree = GTK_CTREE(folderview->ctree);
	GtkCTreeNode *node;
	FolderItem *item;
	gchar *name;
	gchar *message;
	AlertValue avalue;

	item = folderview_get_selected_item(folderview);
	g_return_if_fail(item != NULL);
	g_return_if_fail(item->folder != NULL);
	if (folder_item_parent(item)) return;

	name = trim_string(item->folder->name, 32);
	message = g_strdup_printf
		(_("Really remove the mailbox `%s' ?\n"
		   "(The messages are NOT deleted from the disk)"), name);
	avalue = alertpanel_full(_("Remove mailbox"), message,
				 GTK_STOCK_CANCEL, _("_Remove"), NULL, FALSE,
				 NULL, ALERT_WARNING, G_ALERTDEFAULT);
	g_free(message);
	g_free(name);
	if (avalue != G_ALERTALTERNATE) return;

	folderview_unselect(folderview);
	summary_clear_all(folderview->summaryview);

	folder_destroy(item->folder);
}

static void delete_folder_cb(FolderView *folderview, guint action,
			     GtkWidget *widget)
{
	GtkCTree *ctree = GTK_CTREE(folderview->ctree);
	FolderItem *item;
	gchar *message, *name;
	AlertValue avalue;
	gchar *old_path;
	gchar *old_id;

	item = folderview_get_selected_item(folderview);
	g_return_if_fail(item != NULL);
	g_return_if_fail(item->path != NULL);
	g_return_if_fail(item->folder != NULL);

	name = trim_string(item->name, 32);
	AUTORELEASE_STR(name, {g_free(name); return;});
	message = g_strdup_printf
		(_("All folder(s) and message(s) under `%s' will be deleted.\n"
		   "Do you really want to delete?"), name);
	avalue = alertpanel_full(_("Delete folder"), message,
				 GTK_STOCK_CANCEL, GTK_STOCK_DELETE, NULL, FALSE,
				 NULL, ALERT_NOTICE, G_ALERTDEFAULT);
	g_free(message);
	if (avalue != G_ALERTALTERNATE) return;

	Xstrdup_a(old_path, item->path, return);
	old_id = folder_item_get_identifier(item);

	if (folderview->opened == folderview->selected ||
	    gtk_ctree_is_ancestor(ctree,
				  folderview->selected,
				  folderview->opened)) {
		summary_clear_all(folderview->summaryview);
		folderview->opened = NULL;
	}

	if (item->folder->klass->remove_folder(item->folder, item) < 0) {
		alertpanel_error(_("Can't remove the folder `%s'."), name);
		if (folderview->opened == folderview->selected)
			summary_show(folderview->summaryview,
				     folderview->summaryview->folder_item);
		g_free(old_id);
		return;
	}

	folder_write_list();

	prefs_filtering_delete_path(old_id);
	g_free(old_id);

}

static void move_folder_cb(FolderView *folderview, guint action, GtkWidget *widget)
{
	FolderItem *from_folder = NULL, *to_folder = NULL;

	from_folder = folderview_get_selected_item(folderview);
	if (!from_folder || from_folder->folder->klass != claws_mailmbox_get_class())
		return;

	to_folder = foldersel_folder_sel(from_folder->folder, FOLDER_SEL_MOVE, NULL, FALSE);
	if (!to_folder)
		return;

	folderview_move_folder(folderview, from_folder, to_folder, action);
}

static void rename_folder_cb(FolderView *folderview, guint action,
			     GtkWidget *widget)
{
	FolderItem *item, *parent;
	gchar *new_folder;
	gchar *name;
	gchar *message;
	gchar *old_path;
	gchar *old_id;
	gchar *new_id;
	gchar *p;

	item = folderview_get_selected_item(folderview);
	g_return_if_fail(item != NULL);
	g_return_if_fail(item->path != NULL);
	g_return_if_fail(item->folder != NULL);

	name = trim_string(item->name, 32);
	message = g_strdup_printf(_("Input new name for `%s':"), name);
	new_folder = input_dialog(_("Rename folder"), message, item->name);
	g_free(message);
	g_free(name);
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

	parent = folder_item_parent(item);
	p = g_strconcat(parent->path ? parent->path : "", ".", new_folder, NULL);
	if (folder_find_child_item_by_name(parent, p)) {
		name = trim_string(new_folder, 32);
		alertpanel_error(_("The folder `%s' already exists."), name);
		g_free(name);
		return;
	}

	Xstrdup_a(old_path, item->path, {g_free(new_folder); return;});

	old_id = folder_item_get_identifier(item);

	if (folder_item_rename(item, new_folder) < 0) {
		alertpanel_error(_("The folder could not be renamed.\n"
				   "The new folder name is not allowed."));
		g_free(old_id);
		return;
	}

	new_id = folder_item_get_identifier(item);
	prefs_filtering_rename_path(old_id, new_id);
	account_rename_path(old_id, new_id);

	g_free(old_id);
	g_free(new_id);

	folder_item_prefs_save_config_recursive(item);
	folder_write_list();
}

